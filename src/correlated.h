/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis pliku: Moduł obsługi pliku z skorelowanymi zasobami.
 */

#ifndef CORRELATED_H
#define CORRELATED_H

#include "common.h"
#include "buffer.h"

// Struktura wpisu danych w pliku z serwerami skorelowanymi.
struct corr_res {
  // Hasz wpisu danych.
  uint64_t hash;
  // Bufor zawierający wczytane dane z tabluacjami zamienionymi w NULL.
  struct buffer buf;
  // Wskaźnik na pierwszy znak zasobu.
  unsigned char const *res;
  // Wskaźnik na pierwszy znak adresu serwera.
  unsigned char const *server;
  // Wskaźnik na pierwszy znak numeru portu.
  unsigned char const *port;
};

// Tablica struktur corr_res.
struct corr_res_arr {
  ssize_t size;         // Liczba pól tablicy (-1 to błąd);
  struct corr_res *arr; // Tablica.
};

/*
 * Ładuje tablicę struktur plików skorelowanych z pliku.
 * Może być wywołana z powodzeniem tylko raz.
 * Zwraca tablicę z rozmiarem -1 w przypadku błędu.
 */
struct corr_res_arr correlated_load(char const *filename);

/*
 * Zwraca indeks pod jakim znajduje się szukany zasób.
 * -1 jeśli nie ma.
 */
ssize_t correlated_find(struct corr_res_arr const *arr, struct buffer *res);



#endif/*CORRELATED_H*/
