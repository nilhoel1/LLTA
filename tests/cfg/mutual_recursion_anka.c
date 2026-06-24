/* The Maelardalen anka/kalle mutual-recursion pattern, made reachable: main
 * calls kalle(n) so the kalle<->anka cycle is on the analysis path (in the
 * Maelardalen recursion.c these are dead code). Header = kalle (external
 * entry); the call edge anka->kalle is the cycle backedge into kalle's entry,
 * capped at kalle's recursion_bound, anka bounded transitively -> finite WCET.
 */
volatile int n;

int anka(int);

#pragma recursion_bound(20)
int kalle(int i) {
  if (i <= 0)
    return 0;
  return anka(i - 1);
}

#pragma recursion_bound(20)
int anka(int i) {
  if (i <= 0)
    return 1;
  return kalle(i - 1);
}

int main(void) { return kalle(n); }
