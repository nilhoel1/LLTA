/* Self-recursion with NO recursion_bound: the recursive call edge forms an
 * unbounded cycle, so the analyzer produces NO WCET. Documented expected
 * failure -- finalize() reports the function as UNBOUNDED ("RECURSION: ... has
 * no recursion_bound"). Turns YELLOW the day unbounded self-recursion starts
 * producing a WCET (which would be unsound) -- refresh then. */
volatile int n;

int fac(int x) {
  if (x <= 1)
    return 1;
  return x * fac(x - 1);
}

int main(void) { return fac(n); }
