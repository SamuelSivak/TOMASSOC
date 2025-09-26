#ifndef PASSWORD_H
#define PASSWORD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// Konštanty pre prácu s heslami
#define MIN_PASSWORD_LENGTH 8
#define MAX_PASSWORD_LENGTH 128
#define STRONG_PASSWORD_LENGTH 12

// Štruktúra pre výsledok vyhodnotenia sily hesla
typedef struct {
    int is_strong;          // 1, ak je heslo silné, inak 0.
    int score;             // Celkové skóre sily hesla (0-100).
    char feedback[256];    // Textová spätná väzba s odporúčaniami na zlepšenie.
} PasswordStrength;

/**
 * Generuje náhodné heslo na základe zadaných kritérií.
 * @param password Buffer, do ktorého sa uloží vygenerované heslo.
 * @param length Požadovaná dĺžka hesla.
 * @param include_special Určuje, či heslo má obsahovať špeciálne znaky.
 * @param include_numbers Určuje, či heslo má obsahovať čísla.
 * @param include_uppercase Určuje, či heslo má obsahovať veľké písmená.
 * @param include_lowercase Určuje, či heslo má obsahovať malé písmená.
 * @return 1 pri úspechu, 0 pri chybe.
 */
int generate_password(char *password, int length, int include_special, 
                     int include_numbers, int include_uppercase, int include_lowercase);

/**
 * Vylepšuje existujúce heslo pridaním chýbajúcich typov znakov a jeho predĺžením.
 * @param weak_password Pôvodné (slabé) heslo.
 * @param strong_password Buffer pre upravené (silnejšie) heslo.
 * @return 1 pri úspechu, 0 pri chybe.
 */
int strengthen_password(const char *weak_password, char *strong_password);

/**
 * Vyhodnocuje silu zadaného hesla a poskytuje spätnú väzbu.
 * @param password Heslo na vyhodnotenie.
 * @param result Ukazovateľ na štruktúru, kde sa uložia výsledky hodnotenia.
 * @return 1 pri úspechu, 0 pri chybe.
 */
int evaluate_password_strength(const char *password, PasswordStrength *result);

// --- Pomocné (interné) funkcie ---

int has_lowercase(const char *password); // Kontroluje prítomnosť malých písmen.
int has_uppercase(const char *password); // Kontroluje prítomnosť veľkých písmen.
int has_numbers(const char *password);   // Kontroluje prítomnosť čísel.
int has_special_chars(const char *password); // Kontroluje prítomnosť špeciálnych znakov.
int calculate_entropy(const char *password); // Vypočíta (zjednodušenú) entropiu hesla.
void add_missing_characters(char *password, int *length); // Pridá do hesla chýbajúce typy znakov.
void shuffle_password(char *password); // Náhodne premieša znaky v hesle.

#endif // PASSWORD_H