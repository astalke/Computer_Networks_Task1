/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 */
#include "common.h"

int opened_descriptors[] = {-1, -1, -1};

// Czy ignorujemy SIGPIPE?
static bool sigpipe_ignored = false;

void ignore_sigpipe(void) {
  if (!sigpipe_ignored) {
    sigset_t blockmask;
    sigemptyset(&blockmask);
    struct sigaction action;
    action.sa_handler = SIG_IGN;  // Ignorowanie.
    action.sa_flags = 0;
    action.sa_mask = blockmask;
    if (sigaction(SIGPIPE, &action, NULL) != 0) {
      fatal_err("sigaction");
    }
    sigpipe_ignored = true;
  }
}

void exit_cleanup(void) {
  // Gniazdo zostało otwarte - musimy je zamknąć.
  for (unsigned i = 0; i < sizeof(opened_descriptors) / sizeof(int); ++i) {
    if (opened_descriptors[i] > 0) {
      // Program i tak się zamyka, nic nie zrobię z błędem.
      (void)close(opened_descriptors[i]);
      opened_descriptors[i] = -1;
    }
  }
}

bool is_correct_request_char(int code) {
  if ('a' <= code && code <= 'z') return true;
  if ('A' <= code && code <= 'Z') return true;
  if ('0' <= code && code <= '9') return true;
  if (code == '.') return true;
  if (code == '-') return true;
  if (code == '/') return true;
  return false;
}

