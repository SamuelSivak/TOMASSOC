#include "Password.h"

// Definície konštantných znakových sád pre generovanie hesiel.
static const char lowercase_chars[] = "abcdefghijklmnopqrstuvwxyz";
static const char uppercase_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char number_chars[] = "0123456789";
static const char special_chars[] = "!@#$%^&*()_+-=[]{}|;:,.<>?";

/**
 * @brief Generuje náhodné heslo na základe špecifikovaných kritérií.
 *
 * Funkcia vytvára heslo požadovanej dĺžky, pričom zaručuje, že bude obsahovať
 * aspoň jeden znak z každej zvolenej kategórie (malé/veľké písmená, čísla, špeciálne znaky).
 * Zvyšok hesla je doplnený náhodnými znakmi a celé heslo je nakoniec premiešané.
 */
int generate_password(char *password, int length, int include_special, 
                     int include_numbers, int include_uppercase, int include_lowercase) {
    // Kontrola vstupných parametrov.
    if (!password || length < MIN_PASSWORD_LENGTH || length > MAX_PASSWORD_LENGTH) {
        return 0;
    }
    
    // Ak nie je vybraná žiadna sada znakov, použijú sa všetky ako predvolené.
    if (!include_lowercase && !include_uppercase && !include_numbers && !include_special) {
        include_lowercase = include_uppercase = include_numbers = include_special = 1;
    }
    
    // Vytvorenie kompletnej sady povolených znakov (charset).
    char charset[256] = "";
    int charset_len = 0;
    
    if (include_lowercase) {
        strcat(charset, lowercase_chars);
        charset_len += strlen(lowercase_chars);
    }
    if (include_uppercase) {
        strcat(charset, uppercase_chars);
        charset_len += strlen(uppercase_chars);
    }
    if (include_numbers) {
        strcat(charset, number_chars);
        charset_len += strlen(number_chars);
    }
    if (include_special) {
        strcat(charset, special_chars);
        charset_len += strlen(special_chars);
    }
    
    // Inicializácia generátora náhodných čísel.
    srand(time(NULL));
    
    // Zabezpečenie, že heslo obsahuje aspoň jeden znak z každej požadovanej kategórie.
    int pos = 0;
    
    if (include_lowercase && pos < length) {
        password[pos++] = lowercase_chars[rand() % strlen(lowercase_chars)];
    }
    if (include_uppercase && pos < length) {
        password[pos++] = uppercase_chars[rand() % strlen(uppercase_chars)];
    }
    if (include_numbers && pos < length) {
        password[pos++] = number_chars[rand() % strlen(number_chars)];
    }
    if (include_special && pos < length) {
        password[pos++] = special_chars[rand() % strlen(special_chars)];
    }
    
    // Doplnenie zvyšku hesla náhodnými znakmi z vytvorenej sady.
    while (pos < length) {
        password[pos++] = charset[rand() % charset_len];
    }
    
    // Premiešanie znakov v hesle (Fisher-Yates shuffle) pre zvýšenie náhodnosti.
    for (int i = length - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }
    
    password[length] = '\0'; // Ukončenie reťazca.
    return 1;
}

/**
 * @brief Vylepšuje existujúce heslo tak, aby bolo silnejšie.
 *
 * Funkcia prevezme existujúce heslo, zachová ho a pridá chýbajúce typy znakov
 * (veľké/malé písmená, čísla, špeciálne znaky). Ak je heslo stále príliš krátke,
 * doplní ho na požadovanú dĺžku a nakoniec premieša jeho znaky.
 */
int strengthen_password(const char *weak_password, char *strong_password) {
    if (!weak_password || !strong_password) {
        return 0;
    }
    
    // Skopírovanie pôvodného hesla.
    strcpy(strong_password, weak_password);
    int current_length = strlen(strong_password);
    
    // Pridanie chýbajúcich typov znakov.
    add_missing_characters(strong_password, &current_length);
    
    // Ak je heslo stále kratšie ako cieľová dĺžka, doplní sa náhodnými znakmi.
    if (current_length < STRONG_PASSWORD_LENGTH) {
        char charset[256] = "";
        strcat(charset, lowercase_chars);
        strcat(charset, uppercase_chars);
        strcat(charset, number_chars);
        strcat(charset, special_chars);
        
        // Inicializácia generátora náhodných čísel.
        srand(time(NULL));

        while (current_length < STRONG_PASSWORD_LENGTH) {
            strong_password[current_length++] = charset[rand() % strlen(charset)];
        }
        strong_password[current_length] = '\0';
    }
    
    // Premiešanie znakov pre zvýšenie zložitosti.
    shuffle_password(strong_password);
    
    // Orezanie hesla, ak by po pridaní znakov presiahlo cieľovú dĺžku.
    if (strlen(strong_password) > STRONG_PASSWORD_LENGTH) {
        strong_password[STRONG_PASSWORD_LENGTH] = '\0';
    }
    
    return 1;
}

/**
 * @brief Vyhodnocuje silu hesla na základe viacerých kritérií.
 *
 * Funkcia analyzuje dĺžku, prítomnosť rôznych typov znakov a vypočítanú entropiu.
 * Na základe toho priradí skóre (0-100) a vygeneruje textovú spätnú väzbu
 * s odporúčaniami na zlepšenie.
 */
int evaluate_password_strength(const char *password, PasswordStrength *result) {
    if (!password || !result) {
        return 0;
    }
    
    // Inicializácia výslednej štruktúry.
    result->is_strong = 0;
    result->score = 0;
    strcpy(result->feedback, "");
    
    int length = strlen(password);
    
    // Zistenie prítomnosti jednotlivých typov znakov.
    int has_lower = has_lowercase(password);
    int has_upper = has_uppercase(password);
    int has_nums = has_numbers(password);
    int has_special = has_special_chars(password);
    
    // Výpočet skóre na základe dĺžky.
    int score = 0;
    if (length >= 12) score += 40;
    else if (length >= 8) score += 25;
    else if (length >= 6) score += 15;
    else score += 5;
    
    // Pridelenie bodov za rozmanitosť znakov.
    if (has_lower) score += 10;
    if (has_upper) score += 10;
    if (has_nums) score += 10;
    if (has_special) score += 10;
    
    // Pridelenie bodov za (zjednodušenú) entropiu.
    int entropy = calculate_entropy(password);
    if (entropy >= 60) score += 20;
    else if (entropy >= 40) score += 15;
    else if (entropy >= 30) score += 10;
    else score += 5;
    
    result->score = score;
    
    // Finálne určenie, či je heslo považované za silné.
    // Pridaná kontrola `has_special` pre zosúladenie s odporúčaniami.
    result->is_strong = (score >= 80 && length >= MIN_PASSWORD_LENGTH && 
                        has_lower && has_upper && has_nums && has_special);
    
    // Vytvorenie textovej spätnej väzby pre používateľa.
    char feedback[256] = "";
    
    if (result->is_strong) {
        strcat(feedback, "Heslo je silné!");
    } else {
        // Zostavenie reťazca s odporúčaniami bez úvodnej vety "Heslo je slabé."
        char recommendations[200] = "";
        
        if (length < MIN_PASSWORD_LENGTH) {
            strcat(recommendations, "Použite aspoň 8 znakov. ");
        }
        if (!has_lower) {
            strcat(recommendations, "Pridajte malé písmená. ");
        }
        if (!has_upper) {
            strcat(recommendations, "Pridajte veľké písmená. ");
        }
        if (!has_nums) {
            strcat(recommendations, "Pridajte čísla. ");
        }
        if (!has_special) {
            strcat(recommendations, "Pridajte špeciálne znaky. ");
        }

        // Ak existujú nejaké odporúčania, pridá sa k nim úvodný text.
        if (strlen(recommendations) > 0) {
            sprintf(feedback, "Odporúčania: %s", recommendations);
        }
    }
    
    strcpy(result->feedback, feedback);
    return 1;
}

// --- Pomocné funkcie na kontrolu znakov ---

// Kontroluje, či reťazec obsahuje aspoň jedno malé písmeno.
int has_lowercase(const char *password) {
    for (int i = 0; password[i]; i++) {
        if (islower(password[i])) return 1;
    }
    return 0;
}

// Kontroluje, či reťazec obsahuje aspoň jedno veľké písmeno.
int has_uppercase(const char *password) {
    for (int i = 0; password[i]; i++) {
        if (isupper(password[i])) return 1;
    }
    return 0;
}

// Kontroluje, či reťazec obsahuje aspoň jedno číslo.
int has_numbers(const char *password) {
    for (int i = 0; password[i]; i++) {
        if (isdigit(password[i])) return 1;
    }
    return 0;
}

// Kontroluje, či reťazec obsahuje aspoň jeden špeciálny znak.
int has_special_chars(const char *password) {
    for (int i = 0; password[i]; i++) {
        if (strchr(special_chars, password[i])) return 1;
    }
    return 0;
}

/**
 * @brief Vypočíta zjednodušenú entropiu hesla.
 *
 * Entropia je miera nepredvídateľnosti. Táto funkcia ju aproximuje na základe
 * veľkosti použitej znakovej sady a dĺžky hesla.
 */
int calculate_entropy(const char *password) {
    int charset_size = 0;
    
    if (has_lowercase(password)) charset_size += 26;
    if (has_uppercase(password)) charset_size += 26;
    if (has_numbers(password)) charset_size += 10;
    if (has_special_chars(password)) charset_size += strlen(special_chars);
    
    // Zjednodušený výpočet entropie: log2(veľkosť_sady) * dĺžka
    int length = strlen(password);
    double entropy_per_char = 0;
    
    if (charset_size > 0) {
        // Aproximácia log2 na základe veľkosti sady znakov.
        if (charset_size >= 95) entropy_per_char = 6.6;      // Všetky znaky (~95)
        else if (charset_size >= 62) entropy_per_char = 5.9; // Písmená a čísla (62)
        else if (charset_size >= 36) entropy_per_char = 5.2; // Malé písmená a čísla (36)
        else if (charset_size >= 26) entropy_per_char = 4.7; // Len písmená (26)
        else entropy_per_char = 3.3;                         // Len čísla (10)
    }
    
    return (int)(entropy_per_char * length);
}

/**
 * @brief Pridá do hesla chýbajúce typy znakov.
 *
 * Funkcia skontroluje, či heslo obsahuje malé/veľké písmená, čísla a špeciálne znaky.
 * Ak niektorý typ chýba, pridá na koniec hesla jeden náhodný znak daného typu.
 */
void add_missing_characters(char *password, int *length) {
    srand(time(NULL));
    
    if (!has_lowercase(password) && *length < MAX_PASSWORD_LENGTH - 1) {
        password[(*length)++] = lowercase_chars[rand() % strlen(lowercase_chars)];
    }
    if (!has_uppercase(password) && *length < MAX_PASSWORD_LENGTH - 1) {
        password[(*length)++] = uppercase_chars[rand() % strlen(uppercase_chars)];
    }
    if (!has_numbers(password) && *length < MAX_PASSWORD_LENGTH - 1) {
        password[(*length)++] = number_chars[rand() % strlen(number_chars)];
    }
    if (!has_special_chars(password) && *length < MAX_PASSWORD_LENGTH - 1) {
        password[(*length)++] = special_chars[rand() % strlen(special_chars)];
    }
    
    password[*length] = '\0';
}

/**
 * @brief Náhodne premieša znaky v reťazci.
 *
 * Používa algoritmus Fisher-Yates shuffle na dosiahnutie náhodného
 * usporiadania znakov v zadanom reťazci.
 */
void shuffle_password(char *password) {
    int length = strlen(password);
    if (length <= 1) return;

    for (int i = length - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }
}