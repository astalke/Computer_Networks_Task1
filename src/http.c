/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis pliku: Moduł obsługi żądania HTTP.
 */

#include "http.h"

// Dozwolone metody.
static char const *text_meth[] = {"GET ", "HEAD "};
// Liczba metod w tablicy wyżej.
static size_t const num_of_methods = 2;
// Dozwolony protokół.
static char const protocol[] = "HTTP/1.1";


// Tekst opisujący zwracane kody stanu.
static char const error_message_bad_request[] = "Bad Request";
static char const error_message_not_implemented[] = "Not Implemented";
static char const error_message_server_error[] = "Internal Server Error";
static char const error_message_not_found[] = "Not Found";
static char const error_message_found[] = "Found";
static char const error_message_ok[] = "OK";
// Nazwa serwera.
static char const server_name[] = "Andrzej_Stalke_Server";

// Nagłówki i ich długości.
static unsigned char const connection_text[]  = "CONNECTION:";
static size_t const connection_text_len = sizeof(connection_text) / 
                                          sizeof(unsigned char) - 1;
static unsigned char const con_type_text[]    = "CONTENT-TYPE:";
static size_t const con_type_text_len = sizeof(con_type_text) / 
                                        sizeof(unsigned char) - 1;

static unsigned char const con_length_text[]  = "CONTENT-LENGTH:";
static size_t const con_length_text_len = sizeof(con_length_text) / 
                                          sizeof(unsigned char) - 1;
static unsigned char const server_text[]  = "SERVER:";
static size_t const server_text_len = sizeof(server_text) / 
                                      sizeof(unsigned char) - 1;



/*
 * Ładuje całą linię danych do bufora (czyli kończącą się CRLF).
 * Bufor musi być zainicjowany.
 * Zwraca liczbę wczytanych bajtów (bez CRLF) lub -1 w przypadku błędu.
 */
static int http_readline(int sock, struct buffer *buf) {
  assert(buf != NULL);
  int result = 0;
  int code;
  // Czy poprzedni znak to był cr.
  bool cr = false;
  while ((code = tcp_getchar(sock)) >= 0) {
    if (code == '\r') cr = true;
    else {
      if ((code == '\n') && cr) {
        break;
      } else {
        if (cr) {
          if (!buffer_push(buf, '\r')) return -1;
          ++result;
        }
      }
      if(!buffer_push(buf, code)) return -1;
      ++result;
      cr = false;
    }
  }
  if (code == TCP_CLOSED || code == TCP_ERROR) return -1;
  return result;
}


/*
 * Konwertuje numer w tablicy text_meth funkcji http_read_method do
 * odpowiedniego enuma.
 */
static enum http_requests num_to_enum(size_t m) {
  switch (m) {
    case 0:
      return HTTP_GET;
    case 1:
      return HTTP_HEAD;
    default:
      return HTTP_INVALID;
  }
}

/*
 * Wczytuje pierwszą linię żądania. Jeśli została poprawnie wczytana, to
 * zwraca HTTP_STATUS_OK, w przeciwnym przypadku odpowiedni kod błędu.
 * Jeśli po kodzie serwer powinien przerwać połączenie, to bufor połączenia
 * jest w niezdefiniowanym stanie.
 */
static int http_read_start_line(int sock, struct http_request *ret) {
  assert(ret != NULL);
  ret->method = HTTP_INVALID;

  struct buffer buf;
  if (!buffer_init(&buf)) {
    return HTTP_STATUS_SERVERERROR;
  }
  int r = http_readline(sock, &buf);
  if (r > 0) {
    for (size_t m = 0; m < num_of_methods; ++m) {
      size_t const len = strlen(text_meth[m]);      
      if (len < buf.length) {
        if (strncmp(text_meth[m], (char const *)buf.data, len) == 0) {
          // Metoda dopasowana, nie ma już możliwości powrotu do innej metody.
          ret->method = num_to_enum(m);
          // Inicjujemy bufor na target.
          if (!buffer_init(&ret->target)) {
            buffer_destroy(&buf);
            return HTTP_STATUS_SERVERERROR;
          }
          // Jest dopasowanie metody - można zacząć czytać target.
          size_t j;
          for (j = len; j < buf.length; ++j) {
            if (buf.data[j] == ' ') break;
            if (!buffer_push(&ret->target, buf.data[j])) {
              buffer_destroy(&buf);
              buffer_destroy(&ret->target);
              return HTTP_STATUS_SERVERERROR;
            }
          }
          // Metoda skopiowana lub skończyły się dane.
          ++j;
          if (j < buf.length) {
            size_t const prot_len = strlen(protocol);
            if (j + prot_len <= buf.length) {
              if (strncmp(protocol, (char const*)buf.data + j, prot_len) == 0) {
                buffer_destroy(&buf);
                return HTTP_STATUS_OK;
              }
            }
          }
          // Skończyły się dane przed HTTP/1.1 lub inny błąd.
          buffer_destroy(&buf);
          buffer_destroy(&ret->target);
          return HTTP_STATUS_BAD_REQUEST;
        }
      }
    }
    // Nie udało się dopasować żadnej metody.
    buffer_destroy(&buf);
    return HTTP_STATUS_NOT_IMPLEMENTED;
  }
  // Błąd wczytywania linii.
  buffer_destroy(&buf);
  return r;
}

// Zamienia małe litery w wielkie.
static int small_to_big(int code) {
  if ('a' <= code && code <= 'z') {
    code -= 'a';
    code += 'A';
  }
  return code;
}

// Zwraca true, jeśli text jest prefiksem bufora.
static bool http_field_name_passed(struct buffer const *buf, size_t length, 
    unsigned char const *text) {
  size_t i;
  bool passed = true;
  for (i = 0; i < length; ++i) {
    if (i >= buf->length) {
      passed = false;
      break;
    }
    if (text[i] != small_to_big(buf->data[i])) {
      passed = false;
      break;
    }
  }
  return passed;
}

/*
 * Zwraca jaki jest nagłówek. Wiadomości. Przesuwa *itr o jego długość.
 */
static enum http_headers http_read_field_name(struct buffer *buf, size_t *itr) {
  /*
   * Sprawdzanie poprawności, czyli:
   * - Pierwszy znak nie może być dwukropkiem ani spacją.
   * - Musi być jakiś dwukropek.
   */
  if (buf->length > 0) {
    // Pusty nagłówek.
    if (buf->data[0] == ':') return HTTP_HEADER_INVALID;
    // Zabronione przez standard.
    if (buf->data[0] == ' ') return HTTP_HEADER_INVALID;
  } else {
    return HTTP_HEADER_INVALID;
  }
  bool is_ok = false;
  for (size_t i = 0; i < buf->length; ++i) {
    if (buf->data[i] == ':') {
      is_ok = true;
      break;
    }
  }
  if (!is_ok) return HTTP_HEADER_INVALID;

  enum http_headers result = HTTP_HEADER_UNKNOWN;
  if (http_field_name_passed(buf, connection_text_len, connection_text)) {
    result = HTTP_HEADER_CONNECTION;
    *itr += connection_text_len;
  } 
  else if (http_field_name_passed(buf, con_type_text_len, con_type_text)) {
    result = HTTP_HEADER_CONTENT_TYPE;
    *itr += con_type_text_len;
  }
  else if (http_field_name_passed(buf, con_length_text_len, con_type_text)) {
    result = HTTP_HEADER_CONTENT_LENGTH;
    *itr += con_length_text_len;
  }
  else if (http_field_name_passed(buf, server_text_len, server_text)) {
    result = HTTP_HEADER_SERVER;
    *itr += server_text_len;
  }  
  return result;
}

// Zwraca false jeśli parametr Connection nie jest close, true jeśli jest.
static bool http_handle_connection(struct buffer const *buf) {
  unsigned char const close_text[] = "CLOSE";
  size_t i;
  // Zaczynamy tam gdzie skończyło się wykrywanie, pomijamy spacje.
  for(i = sizeof("Connection:") - 1; i < buf->length; ++i) {
    if (buf->data[i] != ' ') break;
  }
  // Albo i trafiło na coś co nie jest spacją, albo skończył się bufor.
  if (i < buf->length) {
    // Do końca linii musi być co najmniej 5 bajtów
    if (i + sizeof(close_text) - 1 <= buf->length) {
      for (size_t j = i; j < i + sizeof(close_text) - 1; ++j) {
        if (small_to_big(buf->data[j]) != close_text[j - i]) {
          return false;
        }
      }
      for (i = i + sizeof(close_text) - 1; i < buf->length; ++i) {
        if (buf->data[i] != ' ') {
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

// Zwraca true, jeśli powinniśmy wysłać 400.
static bool http_handle_content_length(struct buffer const *buf) {
  size_t i;
  // Zaczynamy tam gdzie skończyło się wykrywanie, pomijamy spacje.
  for(i = sizeof("Content-Length:") - 1; i < buf->length; ++i) {
    if (buf->data[i] != ' ') break;
  }
  // Albo i trafiło na coś co nie jest spacją, albo skończył się bufor.
  if (i < buf->length) {
    // 0 - spacje, 1 - zera, 2 - spacje
    int stage = 0;
    // Czy jest tam coś innego niż: spacje, cyfry 0, spacje?
    for (size_t j = i; j < buf->length; ++j) {
      unsigned char c = buf->data[j];
      if (c != ' ' && c != '0') {
        return true;
      }      
      if (stage == 0 && c == '0') stage = 1;
      else if (stage == 1 && c == ' ') stage = 2;
      else if (stage == 2 && c == '0') return true; // Błąd. 
    }
  }
  return false;
}

int http_read(int sock, struct http_request *ret) {
  assert(ret != NULL);
  int estat = http_read_start_line(sock, ret);
  /*
   * Po napotkaniu HTTP_STATUS_NOT_IMPLEMENTED wciąż możemy spotkać poważniejsze
   * błędy z niego wynikające.
   */
  if (estat == HTTP_STATUS_OK || estat == HTTP_STATUS_NOT_IMPLEMENTED) {
    struct buffer buf;
    buffer_init(&buf);
    while (http_readline(sock, &buf) > 0) {
      // Wskaźnik na pierwszy niezbadany znak.
      size_t itr = 0;
      // Jaki otrzymaliśmy nagłówek.
      enum http_headers header = http_read_field_name(&buf, &itr);
      switch (header) {
        case HTTP_HEADER_CONNECTION:
          // Zaznaczamy obecność.
          if (!http_add_field(ret, header, 0)) {
            // Rzucamy HTTP_STATUS_BAD_REQUEST.
            buffer_destroy(&buf);
            return HTTP_STATUS_BAD_REQUEST;
          }
          // Czy w danych jest "close"?
          if (http_handle_connection(&buf)) {
            ret->closing = true;
          }
          break;
        case HTTP_HEADER_CONTENT_TYPE:
        case HTTP_HEADER_SERVER:
          // Ignorujemy linię, ale zaznaczamy jej obecność.
          if (!http_add_field(ret, header, 0)) {
            // Rzucamy HTTP_STATUS_BAD_REQUEST.
            buffer_destroy(&buf);
            return HTTP_STATUS_BAD_REQUEST;
          }
          break;
        case HTTP_HEADER_CONTENT_LENGTH:
          if (!http_add_field(ret, header, 0)) {
            // Rzucamy HTTP_STATUS_BAD_REQUEST.
            buffer_destroy(&buf);
            return HTTP_STATUS_BAD_REQUEST;
          }
          // Jeśli jest tam coś innego niż 0, rzuć błąd.
          if (http_handle_content_length(&buf)) {
            // Rzucamy HTTP_STATUS_BAD_REQUEST.
            buffer_destroy(&buf);
            return HTTP_STATUS_BAD_REQUEST;
          }
          break;
        case HTTP_HEADER_INVALID:
          // Rzucamy HTTP_STATUS_BAD_REQUEST.
          buffer_destroy(&buf);
          return HTTP_STATUS_BAD_REQUEST;
          break;
        default:
          /* 
           * Ignorujemy, gdyż: "Serwer nie musi przejmować się wielokrotnym 
           * występowaniem nagłówków, które i tak ignoruje."
           */
          break;
      }
      buffer_destroy(&buf);
      buffer_init(&buf);
    }
    buffer_destroy(&buf);
  } 
  return estat;
}

bool http_add_field(struct http_request *ret, enum http_headers header,
                    unsigned value) {
  assert(ret != NULL);
  if (!ret->headers[header].is_set) {
    ret->headers[header].value = value;
    ret->headers[header].is_set = true;
    return true;
  }
  return false;
}

void http_request_init(struct http_request *ptr) {
  assert(ptr != NULL);
  memset(ptr, 0, sizeof(*ptr));
}

void http_request_destroy(struct http_request *ptr) {
  if (ptr != NULL) {
    buffer_destroy(&ptr->target);
  }
}

void http_send_errormessage(int sock, int error) {
  static char const mess_format[] = "HTTP/1.1 %d %s\r\nConnection: "\
    "close\r\nServer: %s\r\n\r\n";
  char message[4096]; // Zmieści się w tylu.
  char const *error_message;
  switch (error) {
    case HTTP_STATUS_BAD_REQUEST:
      error_message = error_message_bad_request;
      break;
    default:
      error_message = error_message_server_error;
      // Server_error
      break;
  }
  int length = sprintf(message, mess_format, error, error_message, server_name);
  // I tak nic nie zrobimy, jeśli wystąpił błąd.
  (void)write_all(sock, message, length);
}

int http_send_not_ok(int sock, int error, bool closing) {
  static char const mess_format[] = "HTTP/1.1 %d %s\r\n"\
                                    "Server: %s\r\n"\
                                    "%s\r\n";
  char message[4096]; // Zmieści się w tylu.
  int length;
  char const *error_message;
  switch (error) {
    case HTTP_STATUS_NOT_IMPLEMENTED:
      error_message = error_message_not_implemented;
      break;
    default:
      error_message = error_message_not_found;
      break;
  }
  if (closing) {
    length = sprintf(message, mess_format, error, error_message, server_name, 
                     "Connection: close\r\n");
  } else {
    length = sprintf(message, mess_format, error, error_message, server_name, 
                     "");
  }
  int ret = write_all(sock, message, length);
  return closing ? 1 : ret;
}

int http_send_found(int sock, struct corr_res const* res, bool closing) {
  static char const mess_format[] = "HTTP/1.1 %d %s\r\n"\
                                    "Server: %s\r\n"\
                                    "Location: http://%s:%s%s\r\n"\
                                    "%s\r\n";
  char message[4096]; // Zmieści się w tylu.
  int length;
  if (closing) {
    length = sprintf(message, mess_format, HTTP_STATUS_FOUND,
                     error_message_found, server_name, res->server, res->port,
                       res->res, "Connection: close\r\n");
  } else {
    length = sprintf(message, mess_format, HTTP_STATUS_FOUND,
                     error_message_found, server_name, res->server, res->port,
                     res->res, "");
  }
  int ret = write_all(sock, message, length);
  return closing ? 1 : ret;
}

int http_send_ok(struct http_request const *req, int sock, int file, 
                 uint64_t file_size, bool closing) {
  static char const mess_format[] = "HTTP/1.1 %d %s\r\nContent-Type: "\
                                     "application/octet-stream\r\n"\
                                     "Content-Length: %"PRIu64"\r\n"\
                                     "Server: %s\r\n"\
                                     "%s\r\n";
  char message[4096]; // Zmieści się w tylu.
  int length;
  if (closing) {
    length = sprintf(message, mess_format, HTTP_STATUS_OK, error_message_ok, 
                     file_size, server_name, "Connection: close\r\n");
  } else {
    length = sprintf(message, mess_format, HTTP_STATUS_OK, error_message_ok, 
                     file_size, server_name, "");
  }
  // Wysyłamy nagłówek.
  int ret = write_all(sock, message, length);
  if (req->method == HTTP_GET) {
    if (ret == 0) {
      while ((ret = read(file, message, 4096)) > 0) {
        if ((ret = write_all(sock, message, ret)) != 0) break;
      }    
    }
  }
  // Jak wystątpił błąd, to informujemy funkcję wyżej.
  close(file);
  return closing ? 1 : ret;
}


int is_correct_path(struct buffer const *buf) {
  assert(buf != NULL);
  assert(buf->data != NULL);
  // Aliasy, bo buf->data jest długie.
  unsigned char const *const ptr = buf->data;
  size_t const len = buf->length;

  // Puste? Błąd.
  if (len == 0) {
    return HTTP_STATUS_BAD_REQUEST;
  }
  // Czy pierwszy znak to '/'?
  if (ptr[0] != '/') {
    return HTTP_STATUS_BAD_REQUEST;
  }

  // Czy są poprawne znaki?
  for (size_t i = 0; i < len; ++i) {
    if (!is_correct_request_char(ptr[i])) {
      return HTTP_STATUS_NOTFOUND;
    }
  }
  
  // Czy na dowolnym prefiksie nie ma więcej /../
  int dots = 0; // Liczba ".."
  int not_dots = 0; // Liczba nie ".."
  for (size_t i = 1; i < len; ++i) {
    bool is_dots = false;
    if (dots > not_dots) return HTTP_STATUS_NOTFOUND;
    if (ptr[i] == '/') {
      // Mogły wystąpić 2 kropki.
      if (i >= 3) {
        // Poprzednie 2 znaki to kropki.
        if (ptr[i - 1] == ptr[i - 2] && ptr[i - 2] == '.') {
          // Znak przed nimi to '/'
          if (ptr[i - 3] == '/') {
            ++dots;
            is_dots = true;
          }
        }
      }
      if (!is_dots) {
        bool is_dot = false;
        // Sprawdzamy, czy przypadkiem nie jest to /./ lub //
        if (i >= 2) {
          if (ptr[i - 1] == '.' && ptr[i - 2] == '/') {
            is_dot = true;
          } 
        }
        if (i >= 1) {
          // Jakby nie patrzeć cd ./directory i cd .//////////////directory
          // robią to samo
          if (ptr[i - 1] == '/') {
            is_dot = true;
          }
        }
        if (!is_dot) {
          ++not_dots;
        }
      }
    }
  }
  // Jeśli ostatni znak nie jest / to na końcu wciąż mogą być kropki.
  if (len >= 3 && ptr[len - 1] != '/') {
    if (ptr[len - 1] == ptr[len - 2] && ptr[len - 2] == '.') {
      if (ptr[len - 3] == '/') {
        ++dots;
      }
    }
  }
  if (dots > not_dots) return HTTP_STATUS_NOTFOUND;
  return HTTP_STATUS_OK;
}


