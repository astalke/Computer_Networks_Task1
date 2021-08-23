/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis: Moduł obsługi sieci.
 */
#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H
#include "common.h"

// Domyślny port na jakim ma nasłuchiwać serwer.
#define DEFAULT_PORT 8080
// Długość kolejki, niesprecyzowana w treści, więc ustawiona na 5.
#define ACCEPT_QUEUE_SIZE 5
// Rozmiar bufora danych z strumienia TCP.
#define TCP_BUFFERSIZE 4096
// Wartość zwracana przez niektóre funkcje, gdy zostało zamknięte połączenie.
#define TCP_CLOSED  (-2)
// Wartość zwracana w przypadku innego błędu.
#define TCP_ERROR   (-1)

/*
 * Pełny write. Czyta count bajtów z buf i pisze je do deskryptora fd.
 * Zwraca 0 w przypadku powodzenia.
 * Zwraca 1, jeśli zamknięto połączenie.
 * -1 w przypadku błędu, errno pozostaje jakie zostało ustawione przez
 *  funkcję write.
 */
int write_all(int fd, void const *buf, size_t const count);

// Czyści bufor TCP z wszystkich danych.
void tcp_flush(void);

/*
 * Pobiera 1 znak z strumienia wejściowego. Jeśli nie ma, to ciągnie więcej
 * z gniazda przekazanego jako argument.
 * W przypadku powodzenia: zwraca numer ASCII uzyskanego znaku (od 0 do 255).
 * W przypadku niepowodzenia:
 *    TCP_CLOSED - połączenie po drugiej stronie jest zamknięte.
 *    -1 - wystąpił błąd, zostaje ustawione stosowne errno.
 */
int tcp_getchar(int sock);

/*
 * Inicjuje gniazdo serwera podanym jako argument portem, aby zaczęło
 * nasłuchiwać i oczekiwać na połączenioa TCP/IPv4.
 * Otwarte gniazdo jest zarejestrowane do zamknięcia przez funkcję 
 */
int init_tcpipv4_socket(unsigned short int port);


#endif/*TCP_HANDLER_H*/
