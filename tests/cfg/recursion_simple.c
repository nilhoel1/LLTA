/* GAP (finding #5): self-recursion. The ILP's call/return matching binds each
 * recursive call to its return but imposes no depth limit, so the analyzer
 * cannot bound the recursion. Documented expected-failure: produces NO WCET. */
volatile int n;

int fib(int x) {
  if (x < 2)
    return x;
  return fib(x - 1) + fib(x - 2);
}

int main(void) { return fib(n); }
