/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis: Moduł obsługi sieci.
 */
#include "tcp_handler.h"

// Bufor odebranych danych. Po zmianie użytkownika powinien być czyszczony.
unsigned char tcp_buffer[TCP_BUFFERSIZE];
// Aktualny rozmiar bufora.
ssize_t tcp_buffer_length;
// Wskazuje na pierwszy nieprzeczytany znak.
ssize_t tcp_buffer_counter;
// Stan: 0 - ok, TCP_CLOSED - EOF, -1 - error
int tcp_buffer_status;

int tcp_getchar(int sock) {
  int result = TCP_ERROR;
  if (tcp_buffer_status != 0) {
    if (tcp_buffer_status == TCP_CLOSED) return TCP_CLOSED;
    return TCP_ERROR;
  }
  // Wciąż są wolne dane w buforze.
  if (tcp_buffer_counter < tcp_buffer_length) {
    result = tcp_buffer[tcp_buffer_counter++];
    return result;
  } 
  // Nie ma już danych w buforze - ciągniemy je readem.
  ssize_t ret = read(sock, tcp_buffer, TCP_BUFFERSIZE);
  // Czy zamknięto połączenie?
  if (ret == 0) {
    tcp_buffer_status = TCP_CLOSED;
    return TCP_CLOSED;
  }
  if (ret < 0) {
    tcp_buffer_status = -1;
    // Zachowujemy errno.
    return TCP_ERROR;
  }
  // Wszystko w porządku - przeczytaliśmy co najmniej 1 bajt.
  tcp_buffer_length = ret;
  tcp_buffer_counter = 0;
  tcp_buffer_status = 0;
  result = tcp_buffer[tcp_buffer_counter++];
  return result;
}

void tcp_flush(void) {
  tcp_buffer_length = 0;
  tcp_buffer_counter = 0;
  tcp_buffer_status = 0;
}

int write_all(int fd, void const *buf, size_t const count) {
  size_t written = 0;
  while (written < count) {
    ssize_t ret = write(fd, buf + written, count - written);
    if (ret < 0) {
      // Czy klient zamknął połączenie w czasie pisania?
      if (errno == EPIPE) {
        errno = 0;
        return 1;
      }
      // Zachowujemy errno.
      return -1;
    }
    written += ret;
  }
  return 0;
}

int init_tcpipv4_socket(unsigned short int port) {
  // Otwieramy gniazdo.
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // Jeśli wystąpił błąd - przerwij program.
  if (sock < 0) {
    fatal_err("socket");
  }
  // Gniazdo wymaga już zamknięcia przy pomocy close.
  opened_descriptors[CONNECTION_ACCEPT_SOCKET] = sock;
  // Inicjowanie struktury potrzebnej w bind.
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;    // IPv4
  addr.sin_port = htons(port);  // Podajemy port.
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // Związujemy lokalny adres z gniazdem.
  if (bind(sock, (struct sockaddr*)&addr, (int)sizeof(addr)) == -1) {
    fatal_err("bind");
  }
  // Zaczynamy nasłuchiwanie.
  if (listen(sock, ACCEPT_QUEUE_SIZE) == -1) {
    fatal_err("listen");
  }
  // Wszystko w porządku, zwracamy zainicjowane gniazdo.
  return sock;
}


