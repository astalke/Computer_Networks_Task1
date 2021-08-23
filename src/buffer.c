/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis pliku: Moduł obsługi żądania HTTP.
 */

#include "buffer.h"

bool buffer_init(struct buffer *buf) {
  assert(buf != NULL);
  buf->data = malloc(64 * sizeof(unsigned char));
  if (buf->data == NULL) {
    return false;
  }
  buf->size = 64;
  buf->length = 0;
  return true;
}

void buffer_destroy(struct buffer *buf) {
  assert(buf != NULL);
  free(buf->data);
  buf->data = NULL;
  buf->length = 0;
  buf->size = 0;
}

bool buffer_push(struct buffer *buf, int code) {
  assert(buf != NULL);
  // Czy wymagana jest realokacja?
  if (buf->length >= buf->size) {
    void *temp = realloc(buf->data, 2 * buf->size);
    if (temp == NULL) {
      return false;
    }
    buf->data = temp;
    buf->size *= 2;
  }
  buf->data[buf->length++] = (unsigned char)code;
  return true;
}

uint64_t buffer_hash(struct buffer const *buf) {
  uint64_t result = 0;
  for (size_t i = 0; i < buf->length; ++i) {
    result *= HASH_MUTIPLIER;
    result += buf->data[i];
    result %= HASH_MODULO;
  }
  return result;
}

uint64_t asciiz_hash(unsigned char const *s) {
  uint64_t result = 0;
  for (size_t i = 0; s[i]; ++i) {
    result *= HASH_MUTIPLIER;
    result += s[i];
    result %= HASH_MODULO;
  }
  return result;
}



