# noise_ble — Smart Halloween Clown Nose

Prototipo sperimentale di naso luminoso con buzzer e controllo BLE su **Seeed XIAO ESP32-C3**. Branch `test` dedicato alla verifica; nessuna modifica a `main`.

## Comandi
| Comando | Azione |
| --- | --- |
| `1` | LED acceso + doppio tono |
| `0` | Stop immediato di LED e buzzer |
| `H`, `B` | Doppio tono, LED invariato |
| `L` | Alterna lo stato LED |
| `S` | Sei impulsi LED + buzzer; LED spento alla fine |

Gli effetti sono non bloccanti (macchina a stati e `millis()`), quindi `0` può interromperli. Alla disconnessione BLE viene accodato lo spegnimento. Se l'alimentazione si interrompe, il circuito si spegne.

## Build
Richiede [PlatformIO](https://platformio.org/).

- Hardware XIAO ESP32-C3: `pio run -e xiao_esp32c3`
- Simulazione Wokwi: `pio run -e wokwi`, poi aprire Wokwi in VS Code con `diagram.json` e `wokwi.toml`.
- In Wokwi inviare `1`, `0`, `H`, `L`, `S` via terminale seriale.
- Sulla scheda fisica usare un client BLE GATT con **Write** sul servizio `4fafc201-1fb5-459e-8fcc-c5c9c331914b` / caratteristica `beb5483e-36e1-4688-b7f5-ea07361b26a8`, inviando un singolo byte ASCII.

**Nota:** Wokwi non emula il collegamento BLE; `SIMULATION_MODE` esclude lo stack BLE per testare LED, buzzer e logica. Il test BLE va eseguito con scheda reale.

## Cablaggio preliminare
- D2 / GPIO4 -> resistore **220Ω** -> anodo LED rosso; catodo -> GND.
- D3 / GPIO5 -> **solo un buzzer piezo passivo compatibile con pilotaggio GPIO 3,3V a corrente molto bassa**; altro terminale -> GND.
- Per buzzer da 5V, magnetici o di corrente non specificata usare un driver transistor/MOSFET dimensionato e alimentazione corretta: **non** collegarli direttamente al GPIO.
- Alimentare XIAO via USB-C da 5V; per LiPo seguire lo schema batteria specifico della board, senza collegare 5V al pad LiPo.
- Il LED e le connessioni vicino al volto devono essere isolati; niente componenti rigidi o scoperti a contatto con la pelle. Fissare il cavo in modo che non tiri sul naso.

## Cose da verificare prima del merge
1. Compilazione entrambi gli ambienti PlatformIO.
2. Verifica simulazione: sequenze `1 -> 0`, `S -> 0`, `H -> 0` e `L`.
3. Collegamento/disconnessione BLE, scrittura comando e riavvio advertising su XIAO reale.
4. Compatibilità del buzzer, assorbimento elettrico, luminosità, autonomia e fissaggio.
5. Verifica scheda ESP32-C3 Super Mini separatamente: pinout diverso dalla XIAO.

**Stato:** proof of concept da compilare e collaudare; non certificato su hardware e non verificato in Wokwi in questa sessione. Il progetto non usa autenticazione BLE: chiunque nelle vicinanze, potenzialmente, può inviare comandi; valutare pairing/bonding o controllo accessi prima di un utilizzo pubblico.
