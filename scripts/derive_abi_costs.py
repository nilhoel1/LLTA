#!/usr/bin/env python3
"""Derive conservative, SOUND worst-case cycle upper bounds for the MSP430(X)
runtime/library routines (`__mspabi_*`, newlib soft-float `__unpack_d`/`__pack_d`,
libm, ...) that LLTA cannot analyze directly because they are body-less in the IR
and encoded in MSP430X 20-bit instructions LLVM's MSP430 backend cannot decode.

The output is a `name -> cycles` table baked into the MSP430 target
(getExternalCallCost). Re-run this when the MSP430 toolchain changes.

Method (deliberately conservative -> sound upper bound, not tight):
  * Disassemble the linked ELF with GNU objdump (which DOES decode MSP430X).
  * For each routine, take its instruction range [entry, next-real-symbol).
  * Build intra-routine control flow from objdump's resolved branch targets
    (the trailing ";abs 0x..."/"; ... 0x...." comment). A back edge is a branch
    whose target address <= its own address.
  * Loop nesting depth of an instruction = number of back-edge address intervals
    [target, source] that contain it (correct for the reducible, address-
    contiguous loops gcc emits for these routines).
  * cost = sum over instructions of  cyc(i) * LOOP_BOUND**depth(i)
           + sum over call sites of  cost(callee) * LOOP_BOUND**depth(callsite)
    LOOP_BOUND bounds every loop's trip count; the routines' loops are bit-width
    bounded (<= 64), so 64 is sound.
  * Per-instruction cycles use a safe upper bound (8 for any single transfer;
    pushm/popm.a #n cost 2+n). This never under-counts a real instruction.

Calls are resolved from the objdump comment target; register-indirect `calla rN`
is resolved by scanning back for the most recent `mova #imm, rN`. A routine with
an unresolvable call or an unbounded structure is reported as UNRESOLVED and left
out of the table (so LLTA keeps reporting it UNSOUND rather than under-counting).

Usage:
  derive_abi_costs.py <objdump> <elf> [routine ...]
If no routines are given, all `__mspabi_*` symbols in the ELF are derived.
"""
import re
import subprocess
import sys

LOOP_BOUND = 64  # conservative max trip count for the bit-width loops
DEFAULT_CYC = 8  # safe upper bound for any single MSP430(X) transfer
MAX_DEPTH = 4    # refuse to cost pathologically deep nesting (-> UNRESOLVED)

HEADER_RE = re.compile(r"^([0-9a-fA-F]+) <([^>]+)>:")
INSN_RE = re.compile(r"^\s+([0-9a-fA-F]+):\s+((?:[0-9a-fA-F]{2} )+)\s*\t?(.*)$")
LAST_HEX_RE = re.compile(r"0x([0-9a-fA-F]+)\s*$")
PUSHPOP_RE = re.compile(r"#(\d+)")


class Insn:
    __slots__ = ("addr", "size", "mnem", "ops", "target", "is_call", "is_jump",
                 "is_ret")

    def __init__(self, addr, size, mnem, ops, comment):
        self.addr = addr
        self.size = size
        self.mnem = mnem
        self.ops = ops
        m = LAST_HEX_RE.search(comment)
        self.target = int(m.group(1), 16) if m else None
        self.is_ret = mnem in ("ret", "reta", "reti")
        self.is_call = mnem.startswith("call")
        self.is_jump = mnem.startswith("j")


def parse(objdump, elf):
    out = subprocess.run([objdump, "-d", elf], capture_output=True, text=True,
                         check=True).stdout
    insns = []          # all instructions, in address order
    syms = {}           # real (non-dot) symbol name -> entry address
    for line in out.splitlines():
        h = HEADER_RE.match(line)
        if h:
            name = h.group(2)
            if not name.startswith("."):
                syms[name] = int(h.group(1), 16)
            continue
        m = INSN_RE.match(line)
        if not m:
            continue
        addr = int(m.group(1), 16)
        nbytes = len(m.group(2).split())
        rest = m.group(3)
        comment = rest.split(";", 1)[1] if ";" in rest else ""
        body = rest.split(";", 1)[0].strip()
        parts = body.split(None, 1)
        mnem = parts[0] if parts else ""
        ops = parts[1] if len(parts) > 1 else ""
        insns.append(Insn(addr, nbytes, mnem, ops, comment))
    insns.sort(key=lambda i: i.addr)
    return insns, syms


def cyc(insn):
    if insn.mnem.startswith(("pushm", "popm")):
        m = PUSHPOP_RE.search(insn.ops)
        n = int(m.group(1)) if m else 16
        return 2 + n
    return DEFAULT_CYC


def routine_range(start, insns, sorted_entries):
    # End at the next real-symbol entry strictly greater than start.
    end = None
    for e in sorted_entries:
        if e > start:
            end = e
            break
    body = [i for i in insns if i.addr >= start and (end is None or i.addr < end)]
    return body


def resolve_call_target(insn, body, idx):
    if insn.target is not None:
        return insn.target
    # Register-indirect `calla rN`: scan back for `mova #imm, rN`.
    reg = insn.ops.strip()
    for j in range(idx - 1, -1, -1):
        p = body[j]
        if p.mnem.startswith("mova") and p.ops.replace(" ", "").endswith("," + reg):
            if p.target is not None:
                return p.target
            m = re.search(r"#(\d+)", p.ops)
            if m:
                return int(m.group(1))
        if p.is_call or p.is_ret:
            break
    return None


def cost(start, insns, sorted_entries, addr_to_sym, memo, stack):
    if start in memo:
        return memo[start]
    if start in stack:           # recursion -> cannot bound here
        return None
    stack = stack | {start}
    body = routine_range(start, insns, sorted_entries)
    if not body:
        return None
    lo, hi = body[0].addr, body[-1].addr
    # Back edges within the routine -> loop intervals [target, source].
    loops = []
    for i in body:
        if i.is_jump and i.target is not None and lo <= i.target <= hi \
                and i.target <= i.addr:
            loops.append((i.target, i.addr))

    def depth(addr):
        return sum(1 for (t, s) in loops if t <= addr <= s)

    total = 0
    for idx, i in enumerate(body):
        d = depth(i.addr)
        if d > MAX_DEPTH:
            return None
        total += cyc(i) * (LOOP_BOUND ** d)
        if i.is_call:
            tgt = resolve_call_target(i, body, idx)
            if tgt is None or tgt not in addr_to_sym:
                return None  # unresolved callee -> cannot soundly cost
            sub = cost(addr_to_sym[tgt], insns, sorted_entries, addr_to_sym,
                       memo, stack)
            if sub is None:
                return None
            total += sub * (LOOP_BOUND ** d)
    memo[start] = total
    return total


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    objdump, elf = sys.argv[1], sys.argv[2]
    roots = sys.argv[3:]
    insns, syms = parse(objdump, elf)
    addr_to_sym = {a: a for a in syms.values()}  # entry addr -> itself (start)
    sorted_entries = sorted(syms.values())
    if not roots:
        roots = sorted(n for n in syms if n.startswith("__mspabi_"))
    memo = {}
    for name in roots:
        if name not in syms:
            print(f"  // {name}: not found in {elf}", file=sys.stderr)
            continue
        c = cost(syms[name], insns, sorted_entries, addr_to_sym, memo, frozenset())
        if c is None:
            print(f"  // {name}: UNRESOLVED (left UNSOUND)", file=sys.stderr)
        else:
            print(f'    {{"{name}", {c}}},')


if __name__ == "__main__":
    main()
