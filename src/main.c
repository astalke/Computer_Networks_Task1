/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis pliku: Funkcja main programu.
 */

#include "tcp_handler.h"
#include "http.h"
#include "correlated.h"

/*
 * Funkcja pomocnicza. Konweruje wejściowy ciąg znaków str do unsigned short 
 * int. Wykorzystywana do wczytania numeru portu. Jest to osobna funkcja, gdyż
 * atoi nie wykrywa błędów, jest niebezpieczne i zwraca za duży typ.
 * Maksymalny numer portu to liczba 16-bitowa bez znaku.
 * Przerywa program, jeśli wystąpiły znaki niebędące cyframi w systemie
 * dziesiętnym lub reprezentowana przez nie liczba przekorczyła 65535.
 * Wartość zwracana: liczba reprezentowana przez string str.
 */
static unsigned short int string_to_portnum(char const *str) {
  unsigned short int result = 0;
  int i = 0;
  // Iterujemy się aż napotkamy NULL.
  while (str[i]) {
    // Czy znak jest cyfrą dziesiętną?
    if (str[i] >= '0' && str[i] <= '9') {
      // Tymczasowa wartość do wykrywania przekorczenia zakresu typu.
      unsigned short int temp = result; 
      result *= 10;
      result += str[i] - '0';
      if (result / 10 != temp) {
        fatal("Niepoprawny numer portu.\n");
      }
    } else {
      fatal("Niecyfry w numerze portu.\n");
    }
    ++i;
  }
  return result;
}

/*
 * Próbuje zlokalizować zasób. Jak go znajdzie, to wysyła odpowiedni
 * komunikat do klienta. Jeśli go nie znajdzie lub wystąpi błąd, to wysyła
 * odpowiedni komunikat błędu do klienta.
 * 
 * Zwraca true, jeśli serwer powinien utrzymywać połączenie.
 */
static bool send_target(int sock, struct http_request *req, 
    struct corr_res_arr *corr, char const *dir) {
  // Szukamy teraz pliku.
  size_t dir_len = strlen(dir);
  unsigned char * s = malloc(2 + req->target.length + dir_len);
  if (s == NULL) {
    http_send_errormessage(sock, HTTP_STATUS_SERVERERROR);
    return false;
  }
  // Tworzymy ścieżkę ścieżka...
  memcpy(s, dir, dir_len);
  memcpy(s + dir_len, req->target.data, req->target.length); 
  s[dir_len + req->target.length] = '\0';
  // Struktura zawierająca rozmiar pliku.
  struct stat statbuf;
  // Deskryptor pliku.
  int target_file = -1;

  int statret = stat((char const *restrict)s, &statbuf);
  // Błąd bądź plik to katalog.
  if (statret != -1 && !S_ISDIR(statbuf.st_mode)) {
    // Otwieramy plik do odczytu.
    target_file = open((char const *)s, O_RDONLY);
  }
  free(s); // Nie jest juz potrzebne.
  // Nie znaleziono pliku bądź jest on katalogiem.
  if (target_file == -1) {
    ssize_t ret = correlated_find(corr, &req->target);
    if (ret == -1) {
      // Nie mamy zasobu.
      return http_send_not_ok(sock, HTTP_STATUS_NOTFOUND, req->closing) == 0;
    } else {
      // Zasób jest w serwerach skorelowanych.
      return http_send_found(sock, &corr->arr[ret], req->closing) == 0;
    }
  } else {
    // Plik znaleziony, rzucamy 200.
    uint64_t file_size = statbuf.st_size;
    return http_send_ok(req, sock, target_file, file_size, req->closing) == 0;
  }
}

/*
 * Funkcja główna serwera.
 * Program akceptuje 2 lub 3 argumenty dodatkowe:
 * - Nazwa katalogu z plikami - obowiązkowy. Może być podany jako ścieżka
 *   względna lub bezwzględna. W przypadku ścieżki względnej serwer próbuje 
 *   odnaleźć wskazany katalog w bieżącym katalogu roboczym.
 * - Plik z serwerami skorelowanymi - obowiązkowy.
 * - Numer portu serwera - opcjonalny. Wskazuje numer portu na jakim serwer
 *   powinien nasłuchiwać połączeń od klientów. Domyślny: 8080.
 */
int main(int argc, char *argv[]) {
  // Nieprawidłowa liczba argumentów.
  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Uzycie: %s <nazwa-katalogu-z-plikami> <plik-z-serwerami-"
          "skorelowanymi> [<numer-portu-serwera>].\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  // Rejestrujemy funkcję, która zamyka używane deskryptory.
  atexit(exit_cleanup);
  // Zaczynamy ignorowanie SIGPIPE.
  ignore_sigpipe(); 
  // Port na jakim mam nasłuchiwać.
  unsigned short int port = DEFAULT_PORT;
  // Sprawdzamy, czy korzystamy z domyślnego numeru portu.
  if (argc == 4) {
    port = string_to_portnum(argv[3]);
  }
  // Ładujemy plik z zasobami skorelowanymi do struktury.
  struct corr_res_arr corr = correlated_load(argv[2]);
  if (corr.size == -1) {
    exit(EXIT_FAILURE);
  }

  /*
   * Otwieramy katalog (trzeba sprawdzić, czy istnieje).
   * Nie używam tego do niczego, ale treść mówi, że trzeba otworzyć...
   * Więc mam otwarte :)
   */
  int directory = open(argv[1], O_RDONLY | O_DIRECTORY);
  if (directory == -1) {
    // Nie powiodło się otworzenie katalogu.
    exit(EXIT_FAILURE);
  }
  // Rejestrujemy deskryptor do zamknięcia.
  opened_descriptors[OPENED_DIRECTORY] = directory;

  // Zmianna pomocnicza, gdy potrzebujemy zmiennej na wynik w switchu.
  int wret;
  // Otwieramy i inicjujemy gniazdo serwera.
  int sock = init_tcpipv4_socket(port);
  // Na sock można już wywoływać accept.
  int accepted_socket = -1;
  // Struktura na dane o kliencie.
  struct sockaddr_in cd;
  // Niezbędny parametr funkcji accept.
  socklen_t cdl = sizeof(cd);
  // Główna pętla serwera, która akceptuje klientów.
  while ((accepted_socket = accept(sock, (struct sockaddr *)&cd, &cdl)) > 0) {
    // Rejestrujemy otwarte gniazdo.
    opened_descriptors[CONNECTED_CLIENT_SOCKET] = accepted_socket;
    // Czyścimy bufor danych po poprzednim kliencie.
    tcp_flush();
    bool connected = true;
    while (connected) {    
      struct http_request req;
      http_request_init(&req);
      int errorcode;
      switch ((errorcode = http_read(accepted_socket, &req))) {
        // Nagłówek poprawy, teraz sprawdzanie czy w ogóle mamy ten zasób.
        case HTTP_STATUS_OK:
          if ((wret = is_correct_path(&req.target)) == HTTP_STATUS_OK) {
            connected = send_target(accepted_socket, &req, &corr, argv[1]);
          } else {
            if (wret == HTTP_STATUS_NOTFOUND) {
              wret = http_send_not_ok(accepted_socket, HTTP_STATUS_NOTFOUND,
                                      req.closing);
              if (wret != 0) {
                // Nieważne, czy się rozłączył, czy błąd. Przerywamy połączenie.
                connected = false;
              }
            } else {
              connected = false;
              http_send_errormessage(accepted_socket, HTTP_STATUS_BAD_REQUEST);
            }
          }
          break;
        case HTTP_STATUS_NOT_IMPLEMENTED:
          connected = (http_send_not_ok(accepted_socket, 
                                        HTTP_STATUS_NOT_IMPLEMENTED, 
                                        req.closing) == 0);
          break;
        // Rzucamy błąd i przerywamy połączenie.
        case HTTP_STATUS_BAD_REQUEST:
        case HTTP_STATUS_SERVERERROR:
          http_send_errormessage(accepted_socket, errorcode);
          connected = false;
          break;
        default:
          // Nie powinny wystąpić, oznacza to błąd serwera.
          http_send_errormessage(accepted_socket, HTTP_STATUS_SERVERERROR);
          connected = false;
          break;
      }
      http_request_destroy(&req);
    }
    // Wyrejestrowujemy gniazdo.
    opened_descriptors[CONNECTED_CLIENT_SOCKET] = -1;
    close(accepted_socket);
  }
  return EXIT_SUCCESS;
}
