#include "HTTPserver.h"
#include "../Logic/Password.h"
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

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
 * @brief Inicializuje SSL knižnice a vytvára SSL kontext.
 * 
 * @return Ukazovateľ na novovytvorený SSL_CTX alebo NULL pri chybe.
 */
SSL_CTX *create_ssl_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    // Inicializácia SSL knižníc
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // Vytvorenie novej SSL metódy a kontextu
    method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

/**
 * @brief Konfiguruje SSL kontext načítaním certifikátu a privátneho kľúča.
 * 
 * @param ctx SSL kontext, ktorý sa má konfigurovať.
 */
void configure_ssl_context(SSL_CTX *ctx) {
    // Načítanie certifikátu
    if (SSL_CTX_use_certificate_file(ctx, "certs/cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Načítanie privátneho kľúča
    if (SSL_CTX_use_PrivateKey_file(ctx, "certs/key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
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
 * @brief Načíta a odošle statický súbor klientovi cez SSL.
 * 
 * Funkcia vytvorí cestu k súboru v rámci adresára 'Frontend',
 * načíta ho a odošle ako HTTPS odpoveď s príslušnými hlavičkami.
 * 
 * @param ssl SSL spojenie klienta.
 * @param file_path Relatívna cesta k súboru (napr. "/index.html").
 */
void serve_static_file(SSL* ssl, const char* file_path) {
    char full_path[256];
    // Vytvoríme cestu k súboru v zložke Frontend
    snprintf(full_path, sizeof(full_path), "Frontend%s", file_path);

    FILE* file = fopen(full_path, "rb");
    if (!file) {
        // Súbor sa nenašiel, pošleme odpoveď 404 Not Found.
        char response[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        SSL_write(ssl, response, strlen(response));
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
        SSL_write(ssl, response, strlen(response));
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

    SSL_write(ssl, header, strlen(header));
    // Odošleme samotný obsah súboru
    SSL_write(ssl, buffer, file_size);

    free(buffer);
}

/**
 * @brief Inicializuje a spustí HTTPS server.
 * 
 * Vytvorí socket, nastaví jeho parametre, naviaže ho na port,
 * inicializuje SSL kontext a vstúpi do nekonečnej slučky, 
 * kde prijíma nové spojenia a obsluhuje ich cez SSL/TLS.
 */
void start_server() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    SSL_CTX *ssl_ctx;

    // Inicializácia a konfigurácia SSL
    ssl_ctx = create_ssl_context();
    configure_ssl_context(ssl_ctx);

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

    printf("Server počúva na https://localhost:%d\n", PORT);

    // Nekonečná slučka na prijímanie spojení
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue; // Pri chybe pokračujeme na ďalšie spojenie
        }

        // Vytvorenie SSL štruktúry pre nové spojenie
        SSL *ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, client_socket);

        // Uskutočnenie SSL handshake
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            // Spracovanie zabezpečeného spojenia
            handle_connection(client_socket, ssl);
        }
    }

    // Uvoľnenie zdrojov
    close(server_fd);
    SSL_CTX_free(ssl_ctx);
}

/**
 * @brief Spracuje jedno prichádzajúce klientske HTTPS spojenie.
 * 
 * Prečíta HTTPS požiadavku z SSL spojenia, odovzdá ju na spracovanie
 * a nakoniec bezpečne uzavrie SSL spojenie a socket.
 * 
 * @param client_socket Socket pripojeného klienta.
 * @param ssl SSL štruktúra pre spojenie.
 */
void handle_connection(int client_socket, SSL *ssl) {
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read_total = 0;
    int bytes_read_last = 0;

    // Čítanie požiadavky v cykle, aby sme sa uistili, že máme celú požiadavku.
    // Toto je zjednodušená implementácia, ktorá predpokladá, že požiadavka sa načíta rýchlo.
    do {
        // Ak by sme prekročili veľkosť buffera, prestaneme čítať.
        if (bytes_read_total >= BUFFER_SIZE - 1) {
            break;
        }
        
        bytes_read_last = SSL_read(ssl, buffer + bytes_read_total, BUFFER_SIZE - 1 - bytes_read_total);
        
        if (bytes_read_last > 0) {
            bytes_read_total += bytes_read_last;
        }
        // Jednoduchá heuristika: Ak sme načítali menej, ako sme mohli,
        // predpokladáme, že je to koniec dát. Pre robustnejšie riešenie
        // by bolo potrebné parsovať Content-Length hlavičku.
    } while (bytes_read_last > 0 && SSL_pending(ssl) > 0);


    // Diagnostický výpis prijatej požiadavky na konzolu
    printf("--- Prijatá požiadavka (%d bytes) ---\n%s\n--------------------------\n", bytes_read_total, buffer);
    
    // Všetku logiku spracovania presunieme do funkcie handle_request
    handle_request(client_socket, buffer, ssl);

    // Bezpečné ukončenie SSL spojenia
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_socket);
}

/**
 * @brief Parzuje a spracováva HTTPS požiadavku.
 * 
 * Táto funkcia je hlavným smerovačom aplikácie. Rozlišuje medzi požiadavkami
 * na statické súbory (GET) a API volaniami (POST, OPTIONS) a posiela odpovede cez SSL.
 * 
 * @param client_socket Socket pripojeného klienta (pre informáciu, nepoužíva sa priamo na zápis).
 * @param request Reťazec obsahujúci celú HTTP požiadavku.
 * @param ssl SSL štruktúra pre odosielanie odpovedí.
 */
void handle_request(int client_socket, const char *request, SSL *ssl) {
    (void)client_socket; // Zabráni varovaniu o nepoužitom parametri

    // --- Spracovanie GET požiadaviek na statické súbory ---
    if (strncmp(request, "GET ", 4) == 0) {
        char path[256];
        sscanf(request, "GET %255s", path);
        // Ak je cesta "/", servírujeme index.html
        if (strcmp(path, "/") == 0) {
            serve_static_file(ssl, "/index.html");
        } else {
            serve_static_file(ssl, path);
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
        SSL_write(ssl, response, strlen(response));
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
    
    // Odoslanie odpovede klientovi cez SSL
    SSL_write(ssl, response, strlen(response));
}
