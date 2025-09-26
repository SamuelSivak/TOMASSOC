#include "HTTPserver.h"
#include "../Logic/Password.h"
#include <ctype.h>
#include <sys/select.h> // Pre select() a fd_set
#include <fcntl.h>      // Pre fcntl()

/**
 * @brief Extrahuje hodnotu reťazca z jednoduchého JSON objektu.
 * 
 * Funkcia hľadá zadaný kľúč v JSON reťazci a vracia jeho hodnotu.
 * Je navrhnutá pre veľmi jednoduchý formát, napr. `{"kľúč":"hodnota"}`.
 * 
 * @param json Vstupný JSON reťazec.
 * @param key Kľúč, ktorého hodnotu treba nájsť.
 * @return Novonalokovaný reťazec s hodnotou alebo NULL pri neúspechu.
 *         Volajúci je zodpovedný za uvoľnenie pamäte.
 */
char* get_json_string_value(const char* json, const char* key) {
    char key_pattern[100];
    sprintf(key_pattern, "\"%s\":\"", key);
    
    const char* key_ptr = strstr(json, key_pattern);
    if (!key_ptr) return NULL;

    const char* value_start = key_ptr + strlen(key_pattern);
    const char* value_end = strchr(value_start, '"');
    if (!value_end) return NULL;

    int value_len = value_end - value_start;
    char* value = (char*)malloc(value_len + 1);
    if (!value) return NULL;

    strncpy(value, value_start, value_len);
    value[value_len] = '\0';
    return value;
}

/**
 * @brief Extrahuje číselnú hodnotu z jednoduchého JSON objektu.
 * 
 * Hľadá zadaný kľúč a parsuje nasledujúce číslo.
 * Ignoruje medzery medzi dvojbodkou a hodnotou.
 * 
 * @param json Vstupný JSON reťazec.
 * @param key Kľúč, ktorého číselnú hodnotu treba nájsť.
 * @return Extrahovaná číselná hodnota alebo 0, ak kľúč nebol nájdený.
 */
int get_json_int_value(const char* json, const char* key) {
    char key_pattern[100];
    sprintf(key_pattern, "\"%s\":", key); // Hľadá "kľúč":

    const char* key_ptr = strstr(json, key_pattern);
    if (!key_ptr) return 0;

    const char* value_start = key_ptr + strlen(key_pattern);
    
    // Preskočíme medzery
    while (*value_start && isspace(*value_start)) {
        value_start++;
    }

    // Konvertujeme číslo na integer
    return atoi(value_start);
}

/**
 * @brief Určuje MIME typ súboru na základe jeho prípony.
 * 
 * @param path Cesta k súboru.
 * @return Reťazec reprezentujúci MIME typ (napr. "text/html").
 *         Ak typ nie je rozpoznaný, vráti "application/octet-stream".
 */
const char* get_mime_type(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".js")) return "application/javascript";
    return "application/octet-stream"; // Fallback pre neznáme typy
}

/**
 * @brief Načíta a odošle statický súbor klientovi.
 * 
 * Funkcia vytvorí cestu k súboru v rámci adresára 'Frontend',
 * načíta ho a odošle ako HTTP odpoveď s príslušnými hlavičkami.
 * 
 * @param client_socket Socket klienta.
 * @param file_path Relatívna cesta k súboru (napr. "/index.html").
 */
void serve_static_file(int client_socket, const char* file_path) {
    char full_path[256];
    // Vytvoríme cestu k súboru v zložke Frontend
    snprintf(full_path, sizeof(full_path), "Frontend%s", file_path);

    FILE* file = fopen(full_path, "rb");
    if (!file) {
        // Súbor sa nenašiel, pošleme odpoveď 404 Not Found.
        char response[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(client_socket, response, strlen(response));
        return;
    }

    // Zistíme veľkosť súboru
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Alokujeme pamäť a načítame obsah súboru
    char* buffer = (char*)malloc(file_size);
    if (!buffer) {
        fclose(file);
        // Chyba alokácie pamäte, pošleme odpoveď 500 Internal Server Error.
        char response[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        write(client_socket, response, strlen(response));
        return;
    }

    fread(buffer, 1, file_size, file);
    fclose(file);

    // Pripravíme a odošleme HTTP hlavičky
    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             get_mime_type(full_path), file_size);

    write(client_socket, header, strlen(header));
    // Odošleme samotný obsah súboru
    write(client_socket, buffer, file_size);

    free(buffer);
}

/**
 * @brief Inicializuje a spustí HTTP server.
 * 
 * Vytvorí socket, nastaví jeho parametre, naviaže ho na port
 * a vstúpi do nekonečnej slučky, kde prijíma nové spojenia a obsluhuje ich.
 */
void start_server() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Vytvorenie socketu
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Nastavenie možnosti opätovného použitia adresy a portu
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Konfigurácia adresy servera
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Naviazanie socketu na adresu a port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Začatie počúvania na prichádzajúce spojenia
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server počúva na http://localhost:%d\n", PORT);

    // Nekonečná slučka na prijímanie spojení
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue; // Pri chybe pokračujeme na ďalšie spojenie
        }

        // Spracovanie spojenia
        handle_connection(client_socket);
    }

    // Uvoľnenie zdrojov
    close(server_fd);
}

/**
 * @brief Spracuje jedno prichádzajúce klientske HTTP spojenie.
 * 
 * Prečíta HTTP požiadavku, odovzdá ju na spracovanie
 * a nakoniec uzavrie socket.
 * 
 * @param client_socket Socket pripojeného klienta.
 */
void handle_connection(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = 0;
    int total_bytes_read = 0;

    // Nastavenie neblokujúceho režimu pre socket
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    // Čítanie s časovým limitom, aby sme počkali na prichádzajúce dáta
    fd_set readfds;
    struct timeval tv;
    int retval;

    // Čakáme maximálne 1 sekundu na dáta
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(client_socket, &readfds);

    retval = select(client_socket + 1, &readfds, NULL, NULL, &tv);

    if (retval == -1) {
        perror("select()");
    } else if (retval) {
        // Dáta sú k dispozícii, čítame ich v cykle
        do {
            bytes_read = read(client_socket, buffer + total_bytes_read, BUFFER_SIZE - 1 - total_bytes_read);
            if (bytes_read > 0) {
                total_bytes_read += bytes_read;
            }
        } while (bytes_read > 0 && total_bytes_read < BUFFER_SIZE - 1);
    }
    // Ak po časovom limite neprišli žiadne dáta, total_bytes_read bude 0

    // Diagnostický výpis prijatej požiadavky na konzolu
    printf("--- Prijatá požiadavka (%d bytes) ---\n%s\n--------------------------\n", total_bytes_read, buffer);
    
    // Ak sme nič neprečítali, nemá zmysel pokračovať
    if (total_bytes_read > 0) {
        // Všetku logiku spracovania presunieme do funkcie handle_request
        handle_request(client_socket, buffer);
    }

    // Uzavretie spojenia
    close(client_socket);
}

/**
 * @brief Parzuje a spracováva HTTP požiadavku.
 * 
 * Táto funkcia je hlavným smerovačom aplikácie. Rozlišuje medzi požiadavkami
 * na statické súbory (GET) a API volaniami (POST, OPTIONS) a posiela odpovede.
 * 
 * @param client_socket Socket pripojeného klienta pre odosielanie odpovedí.
 * @param request Reťazec obsahujúci celú HTTP požiadavku.
 */
void handle_request(int client_socket, const char *request) {
    // --- Spracovanie GET požiadaviek na statické súbory ---
    if (strncmp(request, "GET ", 4) == 0) {
        char path[256];
        sscanf(request, "GET %255s", path);
        // Ak je cesta "/", servírujeme index.html
        if (strcmp(path, "/") == 0) {
            serve_static_file(client_socket, "/index.html");
        } else {
            serve_static_file(client_socket, path);
        }
        return;
    }

    // --- Spracovanie API požiadaviek (POST, OPTIONS) ---
    char response[BUFFER_SIZE] = {0};
    char json_response[1024] = "{ \"error\": \"Invalid request\" }"; // Predvolená chybová správa

    // --- Obsluha pre OPTIONS (CORS preflight) ---
    // Potrebné pre moderné prehliadače na povolenie Cross-Origin požiadaviek.
    if (strncmp(request, "OPTIONS /api/", 13) == 0) {
        sprintf(response, 
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: keep-alive\r\n"
            "\r\n");
        write(client_socket, response, strlen(response));
        return;
    }

    // --- Spracovanie POST požiadaviek na API endpointy ---

    // Endpoint na generovanie hesla
    if (strncmp(request, "POST /api/generate", 18) == 0) {
        const char* body = strstr(request, "\r\n\r\n"); // Nájdenie tela požiadavky
        if (body) {
            // Parsovanie parametrov z JSON tela
            int length = get_json_int_value(body, "length");
            int upper = get_json_int_value(body, "includeUppercase");
            int lower = get_json_int_value(body, "includeLowercase");
            int nums = get_json_int_value(body, "includeNumbers");
            int syms = get_json_int_value(body, "includeSymbols");

            // Validácia dĺžky, aby sa predišlo chybám
            if (length < MIN_PASSWORD_LENGTH || length > MAX_PASSWORD_LENGTH) {
                length = 12; // Predvolená hodnota
            }

            char password[MAX_PASSWORD_LENGTH + 1] = {0};
            if (generate_password(password, length, syms, nums, upper, lower)) {
                PasswordStrength result;
                evaluate_password_strength(password, &result); // Vyhodnotenie sily vygenerovaného hesla
                // Vytvorenie JSON odpovede s heslom a jeho skóre
                sprintf(json_response, "{ \"password\": \"%s\", \"score\": %d, \"feedback\": \"%s\" }", 
                        password, result.score, result.feedback);
            }
        }

    // Endpoint na vyhodnotenie hesla
    } else if (strncmp(request, "POST /api/evaluate", 18) == 0) {
        const char* body = strstr(request, "\r\n\r\n");
        if (body) {
            char* password = get_json_string_value(body, "password");
            if (password) {
                PasswordStrength result;
                evaluate_password_strength(password, &result);
                // Vytvorenie JSON odpovede so skóre a spätnou väzbou
                sprintf(json_response, "{ \"score\": %d, \"feedback\": \"%s\" }", result.score, result.feedback);
                free(password); // Uvoľnenie pamäte alokovanej v get_json_string_value
            }
        }

    // Endpoint na vylepšenie hesla
    } else if (strncmp(request, "POST /api/strengthen", 20) == 0) {
        const char* body = strstr(request, "\r\n\r\n");
        if (body) {
            char* password = get_json_string_value(body, "password");
            if (password) {
                char strong_password[MAX_PASSWORD_LENGTH + 1] = {0};
                strengthen_password(password, strong_password);
                // Vytvorenie JSON odpovede s vylepšeným heslom
                sprintf(json_response, "{ \"strong_password\": \"%s\" }", strong_password);
                free(password); // Uvoľnenie pamäte
            }
        }
    }

    // Vytvorenie finálnej HTTP odpovede s JSON obsahom a CORS hlavičkami
    sprintf(response, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n" // Povoľuje prístup z akejkoľvek domény
        "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        strlen(json_response), json_response);
    
    // Odoslanie odpovede klientovi
    write(client_socket, response, strlen(response));
}
