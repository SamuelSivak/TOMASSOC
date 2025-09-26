# === Konfigurácia kompilátora a projektu ===

# Kompilátor pre jazyk C
CC = gcc
# Príznaky pre kompilátor:
# -Wall, -Wextra: Zapne všetky bežné a extra varovania pre lepšiu kvalitu kódu.
# -std=c99: Použije štandard jazyka C99.
# -g: Vygeneruje debug informácie pre jednoduchšie ladenie.
CFLAGS = -Wall -Wextra -std=c99 -g

# Knižnice potrebné pre projekt (napr. pre SSL)
LIBS = -lssl -lcrypto

# Názov výsledného spustiteľného súboru
TARGET = password_server

# Zoznam všetkých zdrojových súborov (.c), ktoré tvoria projekt
SOURCES = Logic/main.c Logic/Password.c BackEnd/HTTPserver.c
# Automatické odvodenie názvov objektových súborov (.c) zo zdrojových (.c)
OBJECTS = $(SOURCES:.c=.o)
# Zoznam všetkých hlavičkových súborov (.h). Zmena v nich spôsobí rekompiláciu.
HEADERS = Logic/Password.h BackEnd/HTTPserver.h

# === Pravidlá pre kompiláciu ===

# Predvolený cieľ, ktorý sa vykoná po zadaní príkazu 'make' bez argumentov.
all: $(TARGET)

# Hlavný cieľ: Vytvorenie spustiteľného súboru.
# Tento cieľ závisí od všetkých objektových súborov (.o).
# Spustí sa až po ich úspešnom vytvorení.
$(TARGET): $(OBJECTS)
	@echo "Linkujem objektové súbory -> $@"
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo "Server bol úspešne skompilovaný: $(TARGET)"

# Pravidlo pre kompiláciu zdrojových súborov (.c) na objektové súbory (.o).
# Každý .o súbor závisí od svojho .c súboru a všetkých hlavičkových súborov.
# Ak sa zmení .c alebo akýkoľvek .h súbor, príslušný .o súbor sa prekompiluje.
%.o: %.c $(HEADERS)
	@echo "Kompilujem $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# === Pomocné príkazy ===

# Vyčistenie projektu: Odstráni všetky vygenerované súbory (objektové súbory a spustiteľný súbor).
clean:
	@echo "Čistím projekt..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Odstraňujem certifikáty..."
	rm -rf certs

# Spustenie servera.
# Najprv sa uistí, že je server aktuálne skompilovaný (závislosť na $(TARGET)).
run: $(TARGET)
	@echo "Spúšťam server na porte 8080..."
	./$(TARGET)

# Generovanie SSL certifikátov pre HTTPS server.
certs:
	@echo "Vytváram adresár pre certifikáty..."
	mkdir -p certs
	@echo "Generujem self-signed SSL certifikát a kľúč pre doménu tomasmajerik.site..."
	openssl req -x509 -newkey rsa:2048 -keyout certs/key.pem -out certs/cert.pem -sha256 -days 365 -nodes -subj "/C=SK/ST=Slovakia/L=Bratislava/O=tomasmajerik.site/OU=IT/CN=tomasmajerik.site"

# Označenie cieľov, ktoré nie sú názvami súborov.
# Zabezpečí, že 'make' sa nepokúsi hľadať súbory s názvami 'all', 'clean', 'run'.
.PHONY: all clean run certs