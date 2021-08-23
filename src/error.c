/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis modułu: moduł obsługi błędów.
 */

#include "error.h"
#include <stdlib.h>
#include <stdio.h>

void fatal_err(char const *func) {
  perror(func);
  exit(EXIT_FAILURE);
}

void fatal(char const *mess) {
  fprintf(stderr, "ERROR: %s", mess);
  exit(EXIT_FAILURE);
}

