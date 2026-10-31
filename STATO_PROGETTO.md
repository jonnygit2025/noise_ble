# Stato del progetto: Naso Clown / Allegro Chirurgo

Ultimo aggiornamento: 31 ottobre 2026.

Questo file serve a riprendere il lavoro in una nuova sessione: cosa funziona, come è cablato, quali comandi esistono e qual è il prossimo obiettivo.

Documentazione completa: [docs/DOCUMENTAZIONE.md](docs/DOCUMENTAZIONE.md). Schema interattivo del circuito: https://claude.ai/artifact/1g9RuKc29m5QmF5Foy48PC (copia modificabile in [docs/circuito-interattivo.html](docs/circuito-interattivo.html)). App per il telefono: https://jonnygit2025.github.io/noise_ble/app/

## Obiettivo finale

Un costume ispirato all'**Allegro Chirurgo**:

- sul busto c'è una scatola (scatola da scarpe o cassetta della frutta) con dentro degli "organi";
- la breadboard con l'elettronica è fissata dietro o sopra la scatola;
- il **LED rosso** sta dentro un **naso da clown bucato**, che si illumina;
- quando qualcuno **tocca un organo**, il buzzer suona e il naso si illumina;
- lo stesso effetto si può far partire **dall'app sul telefono** (BLE).

## Stato attuale: montaggio finale funzionante (31/10/2026)

- **Montato:** LED saldato al cavo bianco sulla punta del naso rosso; buzzer anche lui su cavo, fuori dalla breadboard (mezzi jumper in c24 → S e c22 → −). Breadboard incollata dentro una scatoletta, cavi che escono dall'alto. Tutto saldato, isolato e provato con luce, suono e app.
- LED su GPIO4: verificato acceso/spento da nRF Connect.
- Buzzer KY-006 / HW-508 su GPIO5 + GPIO6 in modalità due pin: verificato, è la modalità più forte.
- Il volume del piezo resta limitato: probabilmente non basta all'aperto fra la gente (vedi "Limiti noti").

## Hardware

- ESP32-C3 Super Mini (USB-C, porta **COM5** su questo PC, USB VID:PID 303A:1001). Pin risaldati il 30/10/2026: prima non si accendeva per una saldatura difettosa.
- LED rosso 5 mm + resistenza 220 Ω.
- Modulo buzzer passivo **KY-006**, serigrafia **HW-508 V0.2.0**. Tre pin ad angolo: **S**, centrale (non collegato), **−**. Nessun transistor: è un piezo pilotato direttamente dai GPIO. Con e senza resistenza il volume era uguale, quindi assorbe pochissimo e si usa senza resistenza.

## Cablaggio attuale

Breadboard vista con la colonna 1 a sinistra e la riga j in alto. Scheda nelle colonne 1–8, righe c e g, USB verso la colonna 1.

Pin della scheda:

| Colonna | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| riga g (in alto) | 5V | G | 3.3 | **4** | 3 | 2 | 1 | 0 |
| riga c (in basso) | **5** | **6** | 7 | 8 | 9 | 10 | 20 | 21 |

Collegamenti (sempre a USB scollegata):

| Elemento | Fori |
|---|---|
| Jumper GPIO4 → LED | j4 ↔ a18 |
| Resistenza 220 Ω | e18 ↔ e12 |
| Jumper rosso ("+" verso il naso) | c12 |
| Jumper nero ("−" verso il naso) | c11 |
| Jumper LED → G | a11 ↔ j2 |
| Cavo buzzer (mezzi jumper) | c24 → pin S, c22 → pin −; pin centrale libero. Modulo fuori dalla breadboard |
| Jumper GPIO5 → S | a1 ↔ a24 |
| Jumper − buzzer → GPIO6 | a22 ↔ a2 |

**LED a distanza (31/10/2026, funzionante):** il LED non è più sulla breadboard. Al suo posto, in c12 e c11, ci sono due mezzi jumper maschio (rosso = "+", nero = "−"), saldati e isolati con nastro ai due fili di un cavo piattino bianco a due conduttori. All'altra estremità del cavo c'è il LED: gamba lunga sul filo collegato al rosso. La resistenza da 220 Ω resta sulla breadboard (e18 ↔ e12). Il cavo bianco è troppo grosso per entrare direttamente nei fori: per questo servono i mezzi jumper. Provato con luce, suono e app.

Percorsi elettrici:

```
GPIO4 → 220 Ω → LED (+ → −) → G
GPIO5 → S → piezo → − → GPIO6   (GPIO6 in opposizione a GPIO5)
```

## Comandi (BLE e seriale)

- BLE: nome `NasoClown-Halloween`, service `4fafc201-1fb5-459e-8fcc-c5c9c331914b`, characteristic `beb5483e-36e1-4688-b7f5-ea07361b26a8` (Write). Conta **solo il primo byte**.
- Seriale: COM5, 115200 baud, una lettera seguita da Invio.
- Quando cade la connessione BLE, il firmware esegue `0` e riavvia l'advertising.

| Hex | Lettera | Effetto |
|---|---|---|
| 4C | L | LED acceso / spento |
| 48 | H | Doppio bip "bip … bip" con il suono scelto (anche 42 = B) |
| 31 | 1 | LED acceso + doppio bip |
| 53 | S | Strobo: 6 lampeggi con suono, LED spento alla fine |
| 4D | M | Musichetta horror (inizio della Toccata e Fuga di Bach, ~9 s): il naso si accende su ogni nota e si spegne nelle pause |
| 46 | F | Monitor della sala operatoria: battito che rallenta, poi linea piatta (~13 s). Il naso lampeggia a ogni battito e resta acceso sulla linea piatta |
| 57 | W | Melodia originale "Notte": ostinato veloce in 7/8 (~5 s) |
| 52 | R | Melodia originale "Rosso": arpeggio nervoso stile prog (~4,5 s) |
| 59 | Y | Melodia originale "Nebbia": note lente e dissonanti (~9 s) |
| 30 | 0 | Spegne tutto e interrompe gli effetti |
| 4B | K | Suono iniziale: doppio bip 3750 / 4250 Hz, strobo 4000 Hz *(all'accensione)* |
| 49 | I | Tutti i suoni a 2250 Hz |
| 4A | J | Tutti i suoni a 4000 Hz |
| 41 | A | Volume alto *(all'accensione)* |
| 4E | N | Volume medio |
| 51 | Q | Volume basso |
| 50 | P | Buzzer a due pin, GPIO5 e GPIO6 in opposizione *(all'accensione)* |
| 47 | G | Buzzer a un pin, GPIO6 fisso a 0 V |
| 54 | T | Scala di prova 1500 → 5000 Hz, passo 250 Hz, 15 toni |

I comandi I/J/K, A/N/Q e P/G fanno un doppio bip di conferma. Le impostazioni si perdono al riavvio. Le frequenze 2250 e 4000 Hz sono il 4° e l'11° tono della scala T, quelli che si sentivano più forti.

## Firmware (`src/main.cpp`)

- Effetti gestiti con una macchina a stati e `millis()`, senza attese bloccanti.
- Il suono usa LEDC direttamente, non più `tone()`. I canali 0 e 1 condividono il timer 0; il canale 1 (GPIO6) ha lo stesso duty del canale 0, spostato di mezzo periodo (`hpoint`). Così i due pin sono opposti per costruzione.
- Un primo tentativo, con l'inversione nel GPIO matrix, suonava **peggio**: è stato sostituito.
- Il volume è il duty cycle: 512/1024 (alto), 128 (medio), 32 (basso).

Caricare sulla scheda fisica con l'ambiente esplicito. L'ambiente predefinito in `platformio.ini` è ancora `xiao_esp32c3`.

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32c3_supermini -t upload --upload-port COM5
```

Monitor seriale (chiuderlo prima di ogni upload):

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --port COM5 --baud 115200 --filter send_on_enter --echo
```

## Limiti noti e punti aperti

- **Volume**: un piezo da 12 mm pilotato a 3,3 V resta debole anche in modalità due pin. Per l'aperto serve un componente nuovo: altoparlante 8 Ω 2–3 W con amplificatore (PAM8403 o MAX98357A), oppure buzzer attivo 5 V con transistor NPN (2N2222 o BC547) e resistenza da 1 kΩ.
- Il messaggio `ledc_get_duty ... LEDC is not initialized` può comparire nel log: nasce da `ledcAttachPin()` su un canale non ancora configurato. Non ha bloccato il suono.
- La simulazione in `simulazione_supermini/` è aggiornata al montaggio finale (LED e buzzer su cavo, buzzer fra GPIO5 e GPIO6). Non è stata avviata in Wokwi da Claude: è verificata solo la compilazione.
- Firmware con tutti i comandi (musiche M, F, W, R, Y comprese) caricato e provato sulla scheda.

## Prossima fase

1. ~~LED nel naso~~ e ~~montaggio nella scatoletta~~: **fatti** il 31/10/2026.
2. **Montaggio sulla scatola grande**: scatoletta incollata dietro o sopra la scatola degli organi, alimentazione da power bank via USB-C.
3. **Sensori sugli organi**: un ingresso per organo.
   - La ESP32-C3 **non ha** i sensori touch capacitivi delle altre ESP32.
   - Opzione A, come il gioco originale: contatto metallico, cioè pinzetta collegata a G e un bordo o una lamina metallica su ogni organo collegata a un GPIO con `INPUT_PULLUP`.
   - Opzione B: un modulo touch **TTP223** per organo, che dà un'uscita digitale.
   - GPIO liberi: 0, 1, 3, 7, 10, 20, 21. Evitare 2, 8 e 9 (servono all'avvio); 20 e 21 sono anche l'UART.
4. **Firmware**: leggere gli ingressi con debounce e, al tocco, far partire lo stesso effetto del comando `1` o `S`. Eventualmente notificare all'app quale organo è stato toccato (BLE notify).
5. **Volume**: valutare l'altoparlante con amplificatore.
