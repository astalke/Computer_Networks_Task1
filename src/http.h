/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis pliku: Moduł obsługi żądania HTTP.
 */

#include "common.h"
#include "tcp_handler.h"
#include "buffer.h"
#include "correlated.h"

// Kody odpowiedzi serwera.
#define HTTP_STATUS_OK              200
#define HTTP_STATUS_FOUND           302
#define HTTP_STATUS_BAD_REQUEST     400
#define HTTP_STATUS_NOTFOUND        404
#define HTTP_STATUS_SERVERERROR     500
#define HTTP_STATUS_NOT_IMPLEMENTED 501

// Obsługiwane żądania.
enum http_requests {HTTP_INVALID, HTTP_GET, HTTP_HEAD};
// Obsługiwane nagłówki.
enum http_headers {
  // Connection:
  HTTP_HEADER_CONNECTION      = 0,
  // Content-Type:
  HTTP_HEADER_CONTENT_TYPE    = 1,
  // Content-Length:
  HTTP_HEADER_CONTENT_LENGTH  = 2,
  // Server:
  HTTP_HEADER_SERVER          = 3,
  // Wszystkie inne.
  HTTP_HEADER_UNKNOWN         = 4,
  // Zdecydowanie błędne nagłówki.
  HTTP_HEADER_INVALID         = 5
};
// Ile jest zdefiniowanych wyżej nagłówków.
#define HTTP_HEADERS_NUMBER 5

/*
 * Struktura przechowująca informacje o nagłówku.
 */
struct http_field {
  // Czy nagłówek jest ustawiony.
  bool is_set;
  unsigned value;
};

/*
 * Struktura opisująca żądanie HTTP.
 */
struct http_request {
  enum http_requests method;     // Metoda żądania.  
  struct buffer target;         // Szukany zasób.
  struct http_field headers[HTTP_HEADERS_NUMBER]; // Tablica nagłówków.
  bool closing; // Czy klient przesłał: Connection: close
};

// Inicjuje strukturę http_request.
void http_request_init(struct http_request *ptr);
// Destruktor http_request.
void http_request_destroy(struct http_request *ptr);


/*
 * Ustawia wartość value dla nagłówka header.
 * Jeśli jest już coś ustawione to zwraca false. W przeciwnym razie zwraca true.
 */
bool http_add_field(struct http_request *ret, enum http_headers header, 
                    unsigned value);

/*
 * Czyta dane z gniazda przy pomocy tcp_getchar i parsuje je do postaci
 * struktury http_request.
 * Funkcja zakłada, że pierwszy bajt jest pierwszym bajtem zapytania.
 * Jeśli funkcja otrzyma niepoprawne żądanie, wystąpi błąd lub klient
 * zamknie połączenie, to przerwie parsowanie i zwróci -1. 
 */
int http_read(int sock, struct http_request *ret);

/*
 * Wysyła do klienta komunikat o błędzie, który ma zakończyć się zakończeniem
 * połączenia.
 * Zaimplementowane błędy: 400, 500
 */
void http_send_errormessage(int sock, int error);

/*
 * Wysyła do klienta komunikat o błędzie, który niekoniecznie zamyka
 * połączenie. Czyli 404 i 501.
 * Zwraca 0 jeśli połączenie powinno być utrzymane.
 */
int http_send_not_ok(int sock, int error, bool closing);

/*
 * Wysyła klientowi gdzie ma szukać zasobu.
 * Zwraca 0 jeśli połączenie powinno być utrzymane.
 */
int http_send_found(int sock, struct corr_res const* res, bool closing);

/*
 * Wysyła klientowi zawartość pliku file.
 * Zwraca 0 jeśli połączenie powinno być utrzymane.
 * Zamyka przy okazji plik file.
 */
int http_send_ok(struct http_request const *req, int sock, int file, 
                 uint64_t file_size, bool closing);

/*
 * Stwierdza, czy podana w buforze ścieżka jest w porządku.
 * Czyli, czy zaczyna się znakiem / i nie wychodzi poza katalog przy
 * pomocy ..
 * Zwraca HTTP_STATUS_OK jeśli wszystko w porządku, HTTP_STATUS_NOTFOUND jeśli
 * ścieżka jest niepoprawna, ale nie na tyle by dać HTTP_STATUS_BAD_REQUEST.
 * No i oczywiście w kilku przypadkach rzuci HTTP_STATUS_BAD_REQUEST.
 */
int is_correct_path(struct buffer const *buf);


