# Generátor a Správca Hesiel

Tento projekt je jednoduchá webová aplikácia na generovanie, hodnotenie a vylepšovanie hesiel. Skladá sa z C backendu, ktorý obsluhuje logiku, a frontend rozhrania vytvoreného pomocou HTML, CSS a JavaScriptu.

## Funkcie

- **Generovanie hesiel**: Vytvára náhodné heslá na základe zadaných kritérií (dĺžka, veľké/malé písmená, čísla, špeciálne znaky).
- **Hodnotenie sily hesla**: Analyzuje existujúce heslo a poskytuje skóre a vizuálnu spätnú väzbu o jeho sile.
- **Vylepšenie hesla**: Prevezme existujúce heslo a automaticky ho posilní pridaním chýbajúcich typov znakov a jeho premiešaním.
- **Jednoduché webové rozhranie**: Intuitívne rozhranie pre interakciu s backendom.

## Technologický zásobník

- **Backend**: C (s využitím štandardných knižníc)
- **Frontend**: HTML, CSS, Vanilla JavaScript
- **Build systém**: `make`

## Inštalácia a spustenie

Na spustenie projektu je potrebný C kompilátor (napr. `gcc`) a nástroj `make`.

1.  **Klonovanie repozitára** (ak je v gitu):
    ```bash
    git clone <URL-repozitara>
    cd <nazov-repozitara>
    ```

2.  **Kompilácia projektu**:
    Spustite príkaz `make` v koreňovom adresári projektu. Tým sa skompilujú všetky C zdrojové súbory a vytvorí sa spustiteľný súbor `password_server`.
    ```bash
    make
    ```

3.  **Spustenie servera**:
    Po úspešnej kompilácii spustite server:
    ```bash
    ./password_server
    ```
    Server bude bežať na adrese `http://localhost:8080`.

4.  **Otvorenie v prehliadači**:
    Otvorte webový prehliadač a prejdite na adresu [http://localhost:8080](http://localhost:8080) pre zobrazenie aplikácie.

## Vyčistenie projektu

Pre odstránenie všetkých vygenerovaných `.o` súborov a spustiteľného súboru `password_server` použite príkaz:
```bash
make clean
```
