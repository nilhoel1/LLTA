/* GAP (finding #3): a genuine indirect (function-pointer) call. The target is
 * selected through a volatile-indexed table so the optimizer cannot devirtualize
 * it to a direct call. ProgramGraph only wires MO_GlobalAddress / MO_ExternalSymbol
 * calls, so the indirect target is not wired into the graph. Documented
 * expected-failure: whatever the analyzer reports (a WCET omitting the callee,
 * hence under-approximate, or no WCET) is pinned so the day indirect calls are
 * modeled is visible. */
typedef int (*fn)(int);
volatile int sel;

int add(int x) { return x + 1; }
int sub(int x) { return x - 1; }

static fn table[2] = {add, sub};

int main(void) {
  fn fp = table[sel & 1];
  return fp(5);
}
