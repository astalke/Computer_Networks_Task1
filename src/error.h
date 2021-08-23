/*
 * Sieci komputerowe, Zadanie 1 (prosty serwer HTTP).
 * Autor: Andrzej Stalke
 * Opis modułu: moduł obsługi błędów.
 */

#ifndef MY_ERROR_H
#define MY_ERROR_H

/*
 * Wypisuje informację o błędnym zakończeniu funkcji wraz z errno i przerywa
 * program z kodem EXIT_FAILURE.
 * Argument: nazwa funkcji, w której wystąpił błąd.
 */
extern void fatal_err(char const *func);

/*
 * Wypisuje informację o błędnym zakończeniu funkcji i przerywa program z 
 * kodem EXIT_FAILURE.
 * Argument: wiadomość do wyświetlenia po napisie "ERROR: ".
 */
extern void fatal(char const *mess);



#endif/*MY_ERROR_H*/

