#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// Definícia portu, na ktorom bude server počúvať
#define PORT 8080
// Maximálna veľkosť buffera pre požiadavky
#define BUFFER_SIZE 4096

/**
 * @brief Spustí HTTP server a začne počúvať na definovanom porte.
 * 
 * Táto funkcia inicializuje socket, naviaže ho na port a začne prijímať
 * prichádzajúce spojenia v nekonečnej slučke.
 */
void start_server();

/**
 * @brief Inicializuje SSL kontext pre server.
 * 
 * Načíta certifikát a privátny kľúč a vytvorí SSL_CTX.
 * 
 * @return Ukazovateľ na SSL_CTX alebo NULL pri chybe.
 */
SSL_CTX *create_ssl_context();

/**
 * @brief Konfiguruje SSL kontext s certifikátom a kľúčom.
 * 
 * @param ctx SSL kontext.
 */
void configure_ssl_context(SSL_CTX *ctx);

/**
 * @brief Spracuje prichádzajúce HTTPS spojenie.
 * 
 * Prečíta požiadavku od klienta, zavolá funkciu na spracovanie požiadavky
 * a odošle odpoveď späť klientovi cez SSL.
 * 
 * @param client_socket Socket klienta.
 * @param ssl Ukazovateľ na SSL štruktúru.
 */
void handle_connection(int client_socket, SSL *ssl);

/**
 * @brief Spracuje HTTP požiadavku a vygeneruje odpoveď.
 * 
 * Táto funkcia je "srdcom" API. Parzuje URL a telo požiadavky,
 * volá príslušné funkcie z Password.c a generuje JSON odpoveď.
 * 
 * @param client_socket Socket klienta.
 * @param request Buffer obsahujúci HTTP požiadavku.
 * @param ssl Ukazovateľ na SSL štruktúru.
 */
void handle_request(int client_socket, const char *request, SSL *ssl);

/**
 * @brief Servíruje statický súbor klientovi cez SSL.
 * 
 * Táto funkcia načíta obsah súboru a odošle ho ako HTTPS odpoveď.
 * 
 * @param ssl Ukazovateľ na SSL štruktúru.
 * @param path Cesta k súboru.
 */
void serve_static_file(SSL *ssl, const char* path);

/**
 * @brief Získa MIME typ súboru na základe jeho cesty.
 * 
 * Táto funkcia vráti MIME typ na základe prípony súboru.
 * 
 * @param path Cesta k súboru.
 * @return MIME typ súboru.
 */
const char* get_mime_type(const char* path);

#endif // HTTPSERVER_H
