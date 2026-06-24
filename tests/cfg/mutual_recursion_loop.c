/* Mutual recursion invoked from a pragma-bounded loop. ping<->pong is bounded
 * per top-level entry by recursion_bound; main enters the cycle once per loop
 * iteration. The header bound multiplies by the loop's external-entry count
 * (the IPET row caps the header at B * non-backedge in-flow), which a flat
 * x<=B cap would get wrong -> finite WCET that scales with the loop trips. */
volatile int n;

int pong(int x);

#pragma recursion_bound(8)
int ping(int x) {
  if (x <= 0)
    return 0;
  return pong(x - 1);
}

#pragma recursion_bound(8)
int pong(int x) {
  if (x <= 0)
    return 1;
  return ping(x - 1);
}

int main(void) {
  int s = 0;
#pragma loop_bound(0, 5)
  for (int i = 0; i < 5; i++)
    s += ping(n);
  return s;
}
