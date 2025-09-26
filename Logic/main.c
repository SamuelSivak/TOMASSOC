#include "../BackEnd/HTTPserver.h"

/**
 * @brief Hlavný vstupný bod programu.
 *
 * Funkcia main je zodpovedná za spustenie celej aplikácie.
 * Jej jedinou úlohou je zavolať funkciu `start_server()`, ktorá
 * inicializuje a spustí HTTP server v nekonečnej slučke.
 *
 * @return 0 po úspešnom ukončení (hoci v tomto prípade sa slučka servera nikdy neukončí).
 */
int main() {
    // Spustí HTTP server, ktorý začne počúvať na prichádzajúce spojenia.
    start_server();
    return 0;
}
