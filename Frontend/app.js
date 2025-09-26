/**
 * Frontend JavaScript pre aplikáciu Generátor Hesiel.
 *
 * Zodpovedá za:
 * - Interakciu s používateľom (kliknutia, zadávanie textu).
 * - Komunikáciu s backend API na generovanie, hodnotenie a vylepšovanie hesiel.
 * - Dynamickú aktualizáciu používateľského rozhrania (zobrazenie hesla, sily, atď.).
 */

// --- Výber DOM elementov ---
const passwordLengthInput = document.getElementById('passwordLength');
const includeUppercase = document.getElementById('includeUppercase');
const includeLowercase = document.getElementById('includeLowercase');
const includeNumbers = document.getElementById('includeNumbers');
const includeSymbols = document.getElementById('includeSymbols');
const generateBtn = document.getElementById('generateBtn');
const passwordOutput = document.getElementById('passwordOutput');
const copyBtn = document.getElementById('copyBtn');
const strengthBar = document.getElementById('strengthBar');
const strengthText = document.getElementById('strengthText');
const existingPassword = document.getElementById('existingPassword');
const evaluateBtn = document.getElementById('evaluateBtn');
const strengthenBtn = document.getElementById('strengthenBtn');
const passwordSuggestions = document.getElementById('passwordSuggestions');
const existingStrengthBar = document.getElementById('existingStrengthBar');
const existingStrengthText = document.getElementById('existingStrengthText');

// --- Konfigurácia ---
const API_URL = '/api'; // Relatívna cesta k backend API.

// --- Event Listenery (Spracovanie udalostí) ---

// Kliknutie na tlačidlo "Generovať heslo".
generateBtn.addEventListener('click', generatePassword);

// Kliknutie na tlačidlo "Kopírovať".
copyBtn.addEventListener('click', copyPassword);

// Kliknutie na tlačidlo "Vyhodnotiť silu".
evaluateBtn.addEventListener('click', evaluateExistingPassword);

// Kliknutie na tlačidlo "Vylepšiť heslo".
strengthenBtn.addEventListener('click', strengthenPassword);

// Reakcia na písanie do poľa pre existujúce heslo.
existingPassword.addEventListener('input', () => {
    // Resetuje zobrazenie sily a návrhov, keď používateľ začne písať nové heslo.
    if (existingStrengthBar) existingStrengthBar.style.width = '0%';
    if (existingStrengthText) existingStrengthText.textContent = 'Sila hesla: Nevyhodnotené';
    passwordSuggestions.innerHTML = '';
    passwordSuggestions.classList.remove('show');
});

// --- Komunikácia s API ---

/**
 * @brief Pošle požiadavku na server na vygenerovanie nového hesla.
 * Získa parametre z UI, odošle ich na /api/generate a výsledok zobrazí.
 */
async function generatePassword() {
    const length = parseInt(passwordLengthInput.value);
    const wantsUppercase = includeUppercase.checked;
    const wantsLowercase = includeLowercase.checked;
    const wantsNumbers = includeNumbers.checked;
    const wantsSymbols = includeSymbols.checked;

    try {
        const response = await fetch(`${API_URL}/generate`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                length: length,
                includeUppercase: wantsUppercase ? 1 : 0,
                includeLowercase: wantsLowercase ? 1 : 0,
                includeNumbers: wantsNumbers ? 1 : 0,
                includeSymbols: wantsSymbols ? 1 : 0
            })
        });
        if (!response.ok) throw new Error('Chyba pri generovaní hesla');
        
        const data = await response.json();
        passwordOutput.value = data.password;
        
        // Po získaní hesla a skóre zo servera sa aktualizuje indikátor sily.
        updateStrengthIndicator(data.score, strengthBar, strengthText);

    } catch (error) {
        console.error('Chyba API:', error);
        alert('Nepodarilo sa spojiť so serverom pre generovanie hesla.');
    }
}

/**
 * @brief Pošle požiadavku na server na vyhodnotenie zadaného hesla.
 * Získa heslo z inputu, odošle ho na /api/evaluate a zobrazí výslednú silu a spätnú väzbu.
 */
async function evaluateExistingPassword() {
    const password = existingPassword.value.trim();
    if (!password) {
        alert('Prosím, zadajte heslo na vyhodnotenie');
        return;
    }

    try {
        const response = await fetch(`${API_URL}/evaluate`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ password: password })
        });
        if (!response.ok) throw new Error('Chyba pri hodnotení hesla');

        const data = await response.json();
        updateStrengthIndicator(data.score, existingStrengthBar, existingStrengthText);
        
        // Zobrazenie textovej spätnej väzby od servera.
        passwordSuggestions.innerHTML = `<div class="suggestion-item">${data.feedback}</div>`;
        passwordSuggestions.classList.add('show');

    } catch (error) {
        console.error('Chyba API:', error);
        alert('Nepodarilo sa spojiť so serverom pre hodnotenie hesla.');
    }
}

/**
 * @brief Pošle požiadavku na server na vylepšenie existujúceho hesla.
 * Získa heslo z inputu, odošle ho na /api/strengthen, a aktualizované heslo
 * vloží späť do inputu a rovno ho aj vyhodnotí.
 */
async function strengthenPassword() {
    const password = existingPassword.value.trim();
    if (!password) {
        alert('Prosím, zadajte heslo na vylepšenie');
        return;
    }

    try {
        const response = await fetch(`${API_URL}/strengthen`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ password: password })
        });
        if (!response.ok) throw new Error('Chyba pri vylepšovaní hesla');

        const data = await response.json();
        existingPassword.value = data.strong_password;
        
        // Po vylepšení heslo hneď aj vyhodnotíme, aby používateľ videl novú silu.
        await evaluateExistingPassword();

    } catch (error) {
        console.error('Chyba API:', error);
        alert('Nepodarilo sa spojiť so serverom pre vylepšenie hesla.');
    }
}


// --- Pomocné UI funkcie ---

/**
 * @brief Skopíruje vygenerované heslo do schránky používateľa.
 * Používa moderné a bezpečné Clipboard API.
 */
async function copyPassword() {
    if (!passwordOutput.value) {
        alert('Najprv vygenerujte heslo');
        return;
    }
    
    try {
        await navigator.clipboard.writeText(passwordOutput.value);
        
        // Vizuálna odozva pre používateľa.
        const originalText = copyBtn.innerHTML;
        copyBtn.innerHTML = '<span class="icon">✓</span>';
        setTimeout(() => {
            copyBtn.innerHTML = originalText;
        }, 1500);

    } catch (err) {
        console.error('Nepodarilo sa skopírovať heslo: ', err);
        alert('Kopírovanie do schránky zlyhalo.');
    }
}

/**
 * @brief Aktualizuje vizuálny indikátor sily hesla (farebný pásik a text).
 * @param {number} score Skóre sily hesla (0-100) získané zo servera.
 * @param {HTMLElement} barElement Element pre farebný pásik.
 * @param {HTMLElement} textElement Element pre textový popis sily.
 */
function updateStrengthIndicator(score, barElement, textElement) {
    if (!barElement || !textElement) return;

    let strengthClass = '';
    let strengthWidth = '0%';
    let strengthTextValue = 'Sila hesla: Nevyhodnotené';
    
    if (score < 20) {
        strengthClass = 'very-weak';
        strengthWidth = '20%';
        strengthTextValue = 'Sila hesla: Veľmi slabé';
    } else if (score < 40) {
        strengthClass = 'weak';
        strengthWidth = '40%';
        strengthTextValue = 'Sila hesla: Slabé';
    } else if (score < 60) {
        strengthClass = 'medium';
        strengthWidth = '60%';
        strengthTextValue = 'Sila hesla: Stredné';
    } else if (score < 80) {
        strengthClass = 'strong';
        strengthWidth = '80%';
        strengthTextValue = 'Sila hesla: Silné';
    } else {
        strengthClass = 'very-strong';
        strengthWidth = '100%';
        strengthTextValue = 'Sila hesla: Veľmi silné';
    }

    barElement.className = 'strength-bar'; // Reset tried
    barElement.classList.add(strengthClass);
    barElement.style.width = strengthWidth;
    textElement.textContent = strengthTextValue;
}

/**
 * @brief Inicializačná funkcia, ktorá sa spustí po načítaní stránky.
 * Nastaví predvolené hodnoty a pridá globálne event listenery.
 */
function init() {
    // Nastavenie predvolenej dĺžky hesla v inpute.
    passwordLengthInput.value = 12;
    
    // Pridanie klávesových skratiek pre lepšiu použiteľnosť.
    document.addEventListener('keydown', function(event) {
        // Generovanie hesla pri stlačení Alt+G
        if (event.altKey && event.key === 'g') {
            event.preventDefault(); // Zabráni predvolenej akcii prehliadača
            generatePassword();
        }
        
        // Kopírovanie hesla pri stlačení Alt+C
        if (event.altKey && event.key === 'c') {
            event.preventDefault(); // Zabráni predvolenej akcii prehliadača
            copyPassword();
        }
    });
}

// Spustenie inicializačnej funkcie po úplnom načítaní DOM.
window.addEventListener('DOMContentLoaded', init);