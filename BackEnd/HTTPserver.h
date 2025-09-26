#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
 * @brief Spracuje prichádzajúce HTTP spojenie.
 * 
 * Prečíta požiadavku od klienta, zavolá funkciu na spracovanie požiadavky
 * a odošle odpoveď späť klientovi.
 * 
 * @param client_socket Socket klienta.
 */
void handle_connection(int client_socket);

/**
 * @brief Spracuje HTTP požiadavku a vygeneruje odpoveď.
 * 
 * Táto funkcia je "srdcom" API. Parzuje URL a telo požiadavky,
 * volá príslušné funkcie z Password.c a generuje JSON odpoveď.
 * 
 * @param client_socket Socket klienta.
 * @param request Buffer obsahujúci HTTP požiadavku.
 * @param response Buffer, do ktorého sa zapíše HTTP odpoveď.
 */
void handle_request(int client_socket, const char *request);

/**
 * @brief Servíruje statický súbor klientovi.
 * 
 * Táto funkcia načíta obsah súboru a odošle ho ako HTTP odpoveď.
 * 
 * @param client_socket Socket klienta.
 * @param path Cesta k súboru.
 */
void serve_static_file(int client_socket, const char* path);

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
