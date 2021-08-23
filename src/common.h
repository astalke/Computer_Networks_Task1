/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis: wykorzystywane w całym projekcie biblioteki i definicje.
 */

#ifndef COMMON_H
#define COMMON_H

// Dla getline i sigaction.
#define _POSIX_C_SOURCE (200809L)

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <endian.h>
#include <fcntl.h>
#include <assert.h>
#include "error.h"

/*
 * Tablica gniazd, które trzeba zamknąć.
 */
extern int opened_descriptors[];

/*
 * Pozycja w tablicy opened_descriptors gniazda na którym są akceptowane 
 * połączenia.
 */
#define CONNECTION_ACCEPT_SOCKET  0
// Pozycja w tablicy opened_descriptors gniazda aktualnie połączonego klienta.
#define CONNECTED_CLIENT_SOCKET   1
// Pozycja w tablicy opened_descriptors katalogu.
#define OPENED_DIRECTORY          2

/*
 * Funkcja sprzątająca przed zamknięciem. Zamyka zarejestrowane deskryptory.
 */
void exit_cleanup(void);

// Włącza ignorowanie SIGPIPE.
void ignore_sigpipe(void);

/*
 * Zwraca true, jeśli code jest znakiem, który może występować w zasobie.
 * Czyli czy spełnia [a-zA-Z0-9.-/]
 */
bool is_correct_request_char(int code);


#endif/*COMMON_H*/
