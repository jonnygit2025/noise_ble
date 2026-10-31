# Naso Clown / Allegro Chirurgo: documentazione completa

Ultimo aggiornamento: 31 ottobre 2026.

Questo documento raccoglie tutto il lavoro fatto: hardware, cablaggio, firmware, comandi, app per il telefono, simulazione, strumenti, problemi incontrati e prossimi passi. Il riassunto breve è in [`../STATO_PROGETTO.md`](../STATO_PROGETTO.md).

---

## 1. Obiettivo

Costume per Halloween ispirato al gioco **Allegro Chirurgo**:

- sul busto c'è una **scatola** (scatola da scarpe o cassetta della frutta) con dentro degli **organi**;
- dietro o sopra la scatola c'è una **scatoletta** con l'elettronica: breadboard e ESP32-C3 Super Mini;
- il **LED rosso** è nella punta di un **naso da clown**, collegato con un cavo;
- quando qualcuno **tocca un organo**, il buzzer suona e il naso si illumina; lo stesso effetto si comanda **dal telefono** via Bluetooth.

**Stato al 31/10/2026:** naso, buzzer, scatoletta, firmware e app del telefono sono **finiti e funzionanti**. Manca il rilevamento del tocco sugli organi (sezione 12).

---

## 2. Link e file importanti

| Cosa | Dove |
|---|---|
| App per il telefono (Chrome Android) | https://jonnygit2025.github.io/noise_ble/app/ |
| Pagina interattiva del circuito (pubblicata su claude.ai, privata) | https://claude.ai/artifact/1g9RuKc29m5QmF5Foy48PC |
| Stessa pagina, file modificabile nel repo | [`circuito-interattivo.html`](circuito-interattivo.html) |
| Repository GitHub | https://github.com/jonnygit2025/noise_ble (branch `test`) |
| Firmware | [`../src/main.cpp`](../src/main.cpp) |
| Codice dell'app | [`../app/index.html`](../app/index.html) |
| Simulazione Wokwi del montaggio finale | [`../simulazione_supermini/`](../simulazione_supermini/) |
| Riassunto dello stato | [`../STATO_PROGETTO.md`](../STATO_PROGETTO.md) |

---

## 3. Hardware

| Componente | Note |
|---|---|
| **ESP32-C3 Super Mini** | USB-C nativa (CDC, VID:PID 303A:1001), porta **COM5** su questo PC. Pin risaldati il 30/10/2026: prima non si accendeva per una saldatura difettosa |
| **LED rosso 5 mm** | nella punta del naso, su cavo |
| **Resistenza 220 Ω** | in serie al LED, resta sulla breadboard (~6 mA a 3,3 V) |
| **Buzzer KY-006**, serigrafia **HW-508 V0.2.0** | modulo passivo, 3 pin ad angolo: **S**, centrale (non collegato), **−**. Nessun transistor: è un **piezo** pilotato direttamente dai GPIO. Con e senza resistenza il volume era uguale: assorbe pochissimo, si usa senza resistenza |
| **Breadboard** | righe a–j, colonne 1–30, fessura centrale |
| **Cavo piattino bianco** a due conduttori (multifilare) | per naso e buzzer. Troppo grosso per i fori: ai capi del lato breadboard sono saldati **mezzi jumper maschio** |
| **Power bank USB-C** | alimentazione fuori casa |

### Piedinatura della Super Mini (vista dall'alto, USB a sinistra)

| Colonna breadboard | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| riga g (fila in alto) | 5V | G | 3.3 | **4** | 3 | 2 | 1 | 0 |
| riga c (fila in basso) | **5** | **6** | 7 | 8 | 9 | 10 | 20 | 21 |

- GPIO usati: **4** (LED), **5** e **6** (buzzer).
- GPIO liberi: 0, 1, 3, 7, 10, 20, 21.
- Da evitare: **2, 8, 9**, che decidono la modalità di avvio; 20 e 21 sono anche l'UART.
- **Attenzione a non confondere il pin 5 (GPIO5) con 5V.**

---

## 4. Cablaggio finale

Breadboard vista con la colonna 1 a sinistra e la riga j in alto. Scheda nelle colonne 1–8, righe c e g, USB verso la colonna 1.

| Elemento | Fori | Scopo |
|---|---|---|
| Jumper | j4 ↔ a18 | GPIO4 alla colonna 18 |
| Resistenza 220 Ω | e18 ↔ e12 | limita la corrente del LED |
| Mezzo jumper **rosso** | c12 | cavo → gamba lunga (+) del LED nel naso |
| Mezzo jumper **nero** | c11 | cavo → gamba corta (−) del LED nel naso |
| Jumper | a11 ↔ j2 | ritorno del LED a G |
| Jumper | a1 ↔ a24 | GPIO5 alla colonna 24 |
| Mezzo jumper | c24 | cavo → pin **S** del buzzer |
| Mezzo jumper | c22 | cavo → pin **−** del buzzer |
| Jumper | a22 ↔ a2 | colonna 22 a GPIO6 |

Percorsi elettrici:

```
GPIO4 → a18 ═ e18 → 220 Ω → e12 ═ c12 → cavo → LED (+ → −) → cavo → c11 ═ a11 → j2 = G
GPIO5 → a1 → a24 ═ c24 → cavo → S → piezo → − → cavo → c22 ═ a22 → a2 = GPIO6
```

`═` indica il collegamento interno della breadboard (stessa colonna, stesso lato della fessura).

- In modalità due pin (P) **S e − del buzzer si possono scambiare**: il piezo non ha verso.
- Il LED invece ha un verso: gamba lunga (+) verso la resistenza.

### Montaggio meccanico

- Breadboard **incollata** dentro una scatoletta di plastica o cartone, non di metallo, perché il metallo blocca il Bluetooth. I cavi escono dall'alto.
- Giunzioni saldate e **isolate una per una** con nastro isolante.
- Jumper bloccati con nastro isolante; cavi fissati alla scatola vicino al punto di uscita, perché uno strappo non arrivi ai jumper.
- Nella breadboard **non si salda**: è di plastica e si scioglierebbe.

---

## 5. Firmware ([`src/main.cpp`](../src/main.cpp))

### Architettura

- **Arduino su PlatformIO**, `espressif32@6.9.0` (core Arduino 2.0.17).
- **BLE**: server con un service e una characteristic in scrittura. Ogni scrittura usa **solo il primo byte** come comando, che viene messo in una **coda FreeRTOS** (12 posti) e letto nel `loop()`.
- **Seriale**: 115200 baud, una lettera alla volta; `\n` e `\r` sono ignorati; i caratteri sconosciuti stampano `[WARN] Unknown command: …`.
- **Effetti non bloccanti**: macchina a stati (`Effect`) con `millis()` e `nextTransition`; nel loop c'è solo `delay(1)`.
- **Alla disconnessione BLE** viene messo in coda il comando `0` (spegne tutto) e riparte l'advertising.

### BLE

| | Valore |
|---|---|
| Nome | `NasoClown-Halloween` |
| Service UUID | `4fafc201-1fb5-459e-8fcc-c5c9c331914b` |
| Characteristic UUID | `beb5483e-36e1-4688-b7f5-ea07361b26a8` (Write) |

Il nome completo non sta nel pacchetto di advertising insieme all'UUID a 128 bit: l'app filtra quindi per UUID del servizio o per prefisso `NasoClown`.

### Audio

- **LEDC usato direttamente**, non `tone()`. Canale 0 → GPIO5, canale 1 → GPIO6; i due canali condividono il **timer 0**, quindi la stessa frequenza.
- **Modalità due pin (P, predefinita)**: il canale 1 ha lo stesso duty del canale 0, **spostato di mezzo periodo** (`ledc_set_duty_with_hpoint`, hpoint = 512). I due pin sono quindi sempre opposti e il piezo vede una tensione doppia. Provato: molto più forte.
- **Modalità un pin (G)**: GPIO6 resta fisso a 0 V e fa da massa.
- **Volume** = duty cycle su 1024: 512 (alto), 128 (medio), 32 (basso).
- In silenzio i due pin vengono staccati dal LEDC e tenuti a 0 V.
- **Risonanza del buzzer**: con la scala `T` i toni più forti sono risultati **3750, 4000 e 4250 Hz**, poi 2250 Hz.

### Effetti

| Effetto | Dettaglio |
|---|---|
| Doppio bip (H, B, 1) | tono 1 per 120 ms, 100 ms di silenzio, tono 2 per 200 ms |
| Strobo (S) | 6 lampeggi di 40 ms acceso e 40 ms spento, con suono |
| Scala (T) | da 1500 a 5000 Hz a passi di 250 Hz, 400 ms acceso e 150 ms spento; stampa `[SWEEP] … Hz` |
| Musiche (M, F, W, R, Y) | tabelle di `Note {hz, ms}`, dove 0 Hz è una pausa; dopo ogni nota 25 ms di stacco. **Il LED è acceso sulle note e spento su pause e stacchi** |

Brani:

- **M**: inizio della **Toccata e Fuga in re minore di Bach** (pubblico dominio), un'ottava sopra.
- **F**: **monitor da sala operatoria**: 9 battiti a 3000 Hz sempre più lenti, poi linea piatta di 4 s.
- **W "Notte"**, **R "Rosso"**, **Y "Nebbia"**: **melodie originali** scritte per questo progetto. I temi di *Halloween* (Carpenter), *Profondo Rosso* (Goblin) e *Silent Hill* (Yamaoka) sono protetti da diritto d'autore e **non** sono stati trascritti.

### Impostazioni all'accensione

Suono **K** (3750/4250 Hz, strobo 4000 Hz), volume **A** (alto), modalità **P** (due pin). Le impostazioni cambiate con i comandi si perdono al riavvio.

---

## 6. Comandi

Da nRF Connect: esadecimale. Dal monitor seriale: la lettera seguita da Invio. Dall'app: i bottoni.

| Hex | Lettera | Effetto |
|---|---|---|
| 4C | L | Naso acceso / spento |
| 48 | H | Doppio bip "bip … bip" con il suono scelto |
| 42 | B | Uguale a H |
| 31 | 1 | Naso acceso + doppio bip |
| 53 | S | Strobo: 6 lampeggi con suono, naso spento alla fine |
| 30 | 0 | Spegne tutto e interrompe qualsiasi effetto |
| 4D | M | Toccata e Fuga di Bach (~9 s) |
| 46 | F | Monitor sala operatoria: battito che rallenta, poi linea piatta (~13 s) |
| 57 | W | "Notte": ostinato in 7/8 (~5 s) |
| 52 | R | "Rosso": arpeggio stile prog (~4,5 s) |
| 59 | Y | "Nebbia": note lente e dissonanti (~9 s) |
| 4B | K | Suono iniziale: doppio bip 3750/4250 Hz, strobo 4000 Hz |
| 49 | I | Tutti i suoni a 2250 Hz |
| 4A | J | Tutti i suoni a 4000 Hz |
| 41 | A | Volume alto |
| 4E | N | Volume medio |
| 51 | Q | Volume basso |
| 50 | P | Buzzer a due pin, il più forte |
| 47 | G | Buzzer a un pin (GPIO6 = massa) |
| 54 | T | Scala di prova, 15 toni |

K, I, J, A, N, Q, P e G fanno un doppio bip di conferma.

---

## 7. App per il telefono ([`app/index.html`](../app/index.html))

- **Indirizzo:** https://jonnygit2025.github.io/noise_ble/app/, ospitata su **GitHub Pages** dal branch `test`, cartella root.
- **Tecnologia:** una sola pagina HTML con **Web Bluetooth**. Funziona in **Chrome su Android**; non funziona in Safari su iPhone (lì servirebbe l'app Bluefy) né in Samsung Internet.
- **Bottoni:** Rapidi (Bip bip, Naso + bip, Paziente muore, STOP), Luce, Musiche, Volume, Tipo di suono, Buzzer e prova.
- **Comportamento:** ogni tocco (`pointerdown`, senza ritardo) manda un byte; le scritture sono messe **in coda** perché il BLE ne accetta una alla volta. Tocchi ripetuti partono tutti in ordine. Vibrazione breve a ogni tocco.
- **Collegamento:** "Collega al naso" filtra per UUID del servizio o prefisso `NasoClown`; "Mostra tutti i dispositivi" è il piano B. Dopo 15 s senza risposta i bottoni si sbloccano.
- **Riga di diagnosi** in fondo: dice se il Bluetooth web c'è, se l'adattatore è pronto, che browser è e se la pagina è aperta dentro un'altra app.
- **Uso:** la scheda accetta **un solo telefono alla volta**, quindi va chiusa la connessione in nRF Connect. Internet serve solo per aprire la pagina; i comandi viaggiano **solo via Bluetooth** (~10 m). Il PC non serve.
- **Permessi Android necessari** (erano la causa del "non lo trova"): Chrome → **Dispositivi nelle vicinanze** e **Posizione**, posizione del telefono attiva.
- **Aggiornare l'app:** modificare `app/index.html`, fare commit e push su `test`. Pages si aggiorna in 1–2 minuti.

---

## 8. Pagina interattiva del circuito ([`circuito-interattivo.html`](circuito-interattivo.html))

È la pagina pubblicata come artifact su claude.ai, salvata anche nel repository. Si apre con doppio clic nel browser del PC.

- **Disegno della breadboard** con la scheda, i nodi colorati (GPIO4, nodo intermedio, G, GPIO5 → S, − → GPIO6) e i cavi verso naso e buzzer.
- **Comandi:** cursore per spostare la scheda (colonna più bassa occupata), breadboard ruotata (colonna 1 o 30 a sinistra), scheda girata di 180° (USB verso la colonna 1 o 30), righe dei pin (b–f, c–g, d–h), lato dei componenti (colonne basse o alte), pulsante "Il tuo montaggio attuale".
- **A ogni cambio ricalcola** i fori di jumper, resistenza, cavo del naso e cavo del buzzer, e spiega perché sono corretti.
- **Sezioni:** la regola dei nodi, il metodo in 6 passi, gli errori tipici, la tabella dei comandi, il montaggio finale.
- **Utile per progetti futuri:** la funzione `layout()` nel `<script>` contiene la regola (trovare i pin, scegliere colonne libere, catena GPIO → resistenza → LED → G). `PLACEMENTS`, `ZERO_ROW` e `OTHER_ROW` descrivono le file di pin della Super Mini e si possono adattare a un'altra scheda.
- Per ripubblicarla su claude.ai dopo una modifica serve una sessione di Claude con lo strumento artifact.

### La regola per cablare in qualsiasi posizione

1. Ogni gruppo di 5 fori (a–e oppure f–j) della stessa colonna è un **nodo**. La fessura separa i due lati.
2. Serve una catena di nodi: **GPIO → resistenza → nodo intermedio → LED (+ → −) → G**.
3. Ogni componente ha le gambe in **due nodi diversi**, mai nello stesso.
4. Si leggono sempre **4 e G sulla serigrafia**, non si va a memoria: girando la scheda cambiano.
5. Le due file di pin della Super Mini distano 6 fori, fessura compresa: le uniche posizioni sono **b–f, c–g, d–h**.

---

## 9. Simulazione Wokwi ([`simulazione_supermini/`](../simulazione_supermini/))

- `diagram.json` riproduce il **montaggio finale**: scheda `board-aitewinrobot-esp32c3-supermini`, `wokwi-breadboard-half`, resistenza, LED "Naso", piezo fra GPIO5 e GPIO6.
- `wokwi.toml` punta a `../.pio/build/wokwi/firmware.bin` ed `.elf`.
- **Uso:** compilare con `-e wokwi`, aprire la cartella `simulazione_supermini` in VS Code, poi `Wokwi: Start Simulator`. In simulazione il BLE è escluso (`SIMULATION_MODE`): i comandi si scrivono nel monitor seriale virtuale.
- Il `diagram.json` e il `wokwi.toml` nella root sono il diagramma **originale** e restano separati.
- È verificata la compilazione, non un avvio reale in Wokwi.

---

## 10. Comandi utili (PowerShell, dalla cartella del progetto)

L'ambiente predefinito in `platformio.ini` è ancora `xiao_esp32c3`: **per la scheda vera indicare sempre `-e esp32c3_supermini`**.

```powershell
# compilare e caricare sulla scheda
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32c3_supermini -t upload --upload-port COM5

# monitor seriale (chiuderlo con Ctrl+C prima di ogni upload)
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --port COM5 --baud 115200 --filter send_on_enter --echo

# vedere se la scheda è collegata
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device list

# compilare per la simulazione Wokwi
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e wokwi
```

Ambienti in `platformio.ini`: `xiao_esp32c3` (predefinito, non usato), `esp32c3_supermini` (scheda vera, BLE e USB CDC), `wokwi` (`SIMULATION_MODE`, niente BLE).

---

## 11. Problemi incontrati e soluzioni

| Problema | Causa | Soluzione |
|---|---|---|
| LED inaffidabile all'inizio | pin della scheda solo appoggiati, senza saldatura | saldare i pin |
| Scheda che non si accendeva, nessuna porta COM | saldatura difettosa | risaldare; provare la scheda fuori dalla breadboard |
| Mappa dei fori sbagliata | scheda spostata e girata rispetto alla mappa scritta | leggere sempre la serigrafia; foto della breadboard |
| `[WARN] Unknown command: ␛ [ D` | frecce e tasti speciali nel monitor | scrivere solo la lettera e Invio |
| Nessun messaggio di avvio dopo RST | la USB nativa si riavvia con la scheda | normale: provare con `x` + Invio |
| `ledc_get_duty … LEDC is not initialized` | `ledcAttachPin()` su un canale non ancora configurato | solo un messaggio nel log, non blocca il suono |
| Upload: `Could not open COM5` / `ClearCommError` | porta occupata da un monitor, oppure USB nativa che si riconnette durante il reset | chiudere il monitor e riprovare; oppure BOOT premuto + RST, poi upload |
| Buzzer muto | modulo appoggiato piatto, pin sdraiati sopra i fori | pin infilati nei fori (oggi: cavo) |
| Volume basso | piezo da 12 mm a 3,3 V; toni a 440/880 Hz, lontani dalla risonanza | toni a 3750–4250 Hz; modalità due pin |
| Primo tentativo a due pin più debole | inversione del segnale tramite GPIO matrix | sostituita con due canali LEDC sfasati di mezzo periodo (`hpoint`) |
| Cavo bianco che non entra nei fori | conduttore multifilare troppo grosso | mezzo jumper maschio saldato e isolato |
| Punta del saldatore "sparita" | punta consumata: temperatura alta, punta lasciata senza stagno, pulizia abrasiva | cambiare punta; 330–350 °C; stagnare sempre; paglietta d'ottone |
| App: "ricerca…", non trova la scheda | permessi Android di Chrome mancanti; filtro sul nome esatto | concedere Dispositivi nelle vicinanze e Posizione; filtro per UUID o prefisso |

---

## 12. Prossimi passi

1. **Montaggio sulla scatola grande**: incollare la scatoletta dietro o sopra la scatola degli organi, con velcro o biadesivo. Power bank in tasca o nella scatola.
2. **Provare il power bank** per 15–20 minuti: alcuni si spengono quando la corrente richiesta è bassa.
3. **Tocco sugli organi**: un ingresso per organo.
   - La ESP32-C3 **non ha** i sensori touch capacitivi delle altre ESP32.
   - **Opzione A**, come il gioco originale: pinzetta metallica collegata a G e una lamina metallica su ogni organo collegata a un GPIO libero con `INPUT_PULLUP`. Toccando si chiude il contatto.
   - **Opzione B**: un modulo touch **TTP223** per organo, con uscita digitale.
   - Nel firmware: leggere gli ingressi con un debounce e, al tocco, far partire `F` (paziente che muore) oppure `S`/`1`. Eventualmente notificare all'app quale organo è stato toccato (BLE notify).
4. **Volume per l'aperto** (facoltativo): altoparlante da 8 Ω 2–3 W con amplificatore **PAM8403** o **MAX98357A**, oppure buzzer attivo 5 V con transistor NPN (2N2222 o BC547) e resistenza da 1 kΩ.
5. **App offline** (facoltativo): aggiungere un service worker perché la pagina si apra anche senza internet.
6. **Versione definitiva** (facoltativo): rifare il circuito su una **millefori** saldata.
