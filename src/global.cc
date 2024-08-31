#include "global.h"

// global
#include <signal.h>
#include <stdlib.h>

namespace global {

static volatile sig_atomic_t gRunning = 1;

static void SigHandler(int _) {
  (void)_;
  gRunning = 0;
}

void CatchSignals() {
  // Ctrl + C
  signal(SIGINT, SigHandler);
  // Ctrl + '\'
  signal(SIGQUIT, SigHandler);
  // Ctrl + Z
  signal(SIGTSTP, SigHandler);
}

int IsRunning() { return gRunning; }

}  // namespace global