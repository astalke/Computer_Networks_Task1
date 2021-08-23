#include "correlated.h"


// Tablice załadowane do pamięci.
static struct corr_res_arr correlated_loaded;

// Destruktor załadowanej tablicy przy zamykaniu programu.
static void correlated_atexit(void) {
  for (ssize_t i = 0; i < correlated_loaded.size; ++i) {
    buffer_destroy(&correlated_loaded.arr[i].buf);
  }
  free(correlated_loaded.arr);
}

// Przetwarza bufor i umieszcza dane w pozostałych polach.
static void correlated_process(struct corr_res *ptr) {
  assert(ptr != NULL);
  ptr->res = (unsigned char*)ptr->buf.data;
  // Numer przetwarzanego słowa. 0 - zasób, 1 - serwer, 2 - port
  int stage = 0;
  for (size_t i = 0; i < ptr->buf.length; ++i) {
    // getline umieszcza czasem na końcu \n
    if (ptr->buf.data[i] == '\t' || ptr->buf.data[i] == '\n') {
      // Rozdzielamy słowa '\0'
      ptr->buf.data[i] = '\0';
      switch (stage) {
        case 0:
          ptr->server = &ptr->buf.data[i + 1];
          break;
        case 1:
          ptr->port = &ptr->buf.data[i + 1];
          break;
        default:
          break;
      }
      ++stage;
    }
  }
  // Obliczamy hasz nazwy zasobu.
  ptr->hash = asciiz_hash(ptr->res);
}

struct corr_res_arr correlated_load(char const *filename) {
  assert(filename != NULL);
  // Deklaracja tablicy wynikowej. Będzie wypełniona dopiero na końcu.
  struct corr_res_arr result_arr;
  result_arr.size = -1; // -1 oznacza błąd
  result_arr.arr = NULL;
  FILE *restrict file = fopen(filename, "r");
  if (file == NULL) {
    return result_arr;
  }
  size_t allocated = 64;
  // Dzięki calloc result[i].buf == NULL, co jest niezbędne dla getline.
  struct corr_res *result = calloc(allocated, sizeof(struct corr_res));
  size_t i = 0;
  ssize_t re;
  // True, jeśli doszło do błędu i trzeba zwolnić pamięć.
  bool failed = false;
  while ((re = getline((char **restrict)&result[i].buf.data, 
          (size_t *restrict)&result[i].buf.size, file)) > 0) {
    result[i++].buf.length = re;
    // Wypełniliśmy tablicę, więc realokacja.
    if (i >= allocated) {
      struct corr_res *temp = realloc(result, 2 * allocated * sizeof(struct corr_res));
      if (temp == NULL) {
        failed = true;
        break;
      }
      allocated *= 2;
      result = temp;
    }
  }
  // Błąd - zwolnij pamięć i zwróć NULL.
  if (failed) {
    for (size_t j = 0; j < i; ++j) {
      buffer_destroy(&result[i].buf);
    }
    fclose(file);
    free(result);
    return result_arr;
  }
  // getline się nie powiodło, ale to wciąż trzeba zwolnić.
  free(result[i].buf.data);
  // Dane załadowane do buforów, teraz trzeba je tylko przetworzyć.
  for (size_t j = 0; j < i; ++j) {
    correlated_process(&result[j]);
  }
  result_arr.arr = result;
  result_arr.size = i;
  correlated_loaded = result_arr;
  atexit(correlated_atexit);
  fclose(file);
  return result_arr;
}

ssize_t correlated_find(struct corr_res_arr const *arr, struct buffer *res) {
  uint64_t hash = buffer_hash(res);
  char const *buf1 = (char const*)res->data;
  for (ssize_t i = 0; i < arr->size; ++i) {
    char const *buf2 = (char const*)arr->arr[i].res;
    // Czy zgadzają się hasze?
    if (hash == arr->arr[i].hash) {
      // Sprawdzamy, czy na pewno są równe.
      if (res->length == strlen(buf2)) {
        if (strncmp(buf1, buf2, res->length) == 0) {
          return i;
        }
      }
    }
  }
  return -1;
}

