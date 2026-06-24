/* Self-recursion (fib) bounded via #pragma recursion_bound. The recursive call
 * edge closes a cycle back into fib's entry; the pragma turns that entry into a
 * bounded loop header (max total invocations per top-level call), so the IPET
 * loop-bound row caps the recursion and a finite WCET is produced. Multiple
 * recursive call sites (fib(x-1) and fib(x-2)) are both backedges into the
 * entry; their sum is bounded by N-1. */
volatile int n;

#pragma recursion_bound(20)
int fib(int x) {
  if (x < 2)
    return x;
  return fib(x - 1) + fib(x - 2);
}

int main(void) { return fib(n); }
