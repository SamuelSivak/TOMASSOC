#!/bin/bash

# Tento skript najprv vyčistí projekt od starých build súborov,
# potom vygeneruje SSL certifikáty, skompiluje projekt a spustí server.

echo "Spúšťa sa čistenie projektu..."
make clean

echo "Generujem SSL certifikáty..."
make certs

echo "Projekt sa kompiluje..."
make

# Kontrola, či kompilácia prebehla úspešne
if [ $? -eq 0 ]; then
    echo "Kompilácia úspešná. Spúšťa sa server na http://localhost:8080"
    ./password_server
else
    echo "Chyba pri kompilácii. Server sa nespustí."
    exit 1
fi
