/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis pliku: Moduł obsługi żądania HTTP.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"

// Liczba przez jaką mnożymy w haszu.
#define HASH_MUTIPLIER  300
// Liczba modulująca hasz.
#define HASH_MODULO     1000696969

// Struktura bufora znaków.
struct buffer {
  size_t size;          // Aktualny rozmiar bufora.
  size_t length;        // Numer pierwszego niezajętego bajtu.
  unsigned char *data;  // Wskaźnik na dane.
};

/* 
 * Inicjowanie bufora znaków. Zwraca true w przypadku powodzenia.
 */
bool buffer_init(struct buffer *buf);
// Dealokacja zalokowanego bufora.
void buffer_destroy(struct buffer *buf);

// Dodaje na koniec bufora nowy znak. Zwraca true w przypadku powodzenia.
bool buffer_push(struct buffer *buf, int code);

// Zwraca hasz zawartości bufora.
uint64_t buffer_hash(struct buffer const *buf);

// Liczenie haszu dla stringa zakończonego znakiem '\0'
uint64_t asciiz_hash(unsigned char const *s);


#endif/*BUFFER_H*/
