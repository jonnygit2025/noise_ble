# Simulazione ESP32-C3 Super Mini

Questa variante è separata dal diagramma originale nella root, che resta intatto.
Riproduce il **montaggio finale** del 31/10/2026: LED nel naso e buzzer KY-006
collegati con cavi fuori dalla breadboard. Stato completo del progetto:
[`../STATO_PROGETTO.md`](../STATO_PROGETTO.md).

## Scheda e breadboard

- Scheda: `board-aitewinrobot-esp32c3-supermini` (definizione ufficiale Wokwi).
- Breadboard: `wokwi-breadboard-half`, 30 colonne.
- Nel diagramma la scheda è fuori dalla breadboard: i fili da `esp:4`, `esp:5`,
  `esp:6` e `esp:GND` rappresentano i jumper che sul montaggio reale partono dai
  fori della scheda (j4, a1, a2, j2).
- I nomi Wokwi `18t.a` … `18t.e` sono i fori a–e della colonna 18.

## Collegamenti

| Elemento | Montaggio reale | Diagramma Wokwi |
| --- | --- | --- |
| GPIO4 → colonna 18 | jumper j4 ↔ a18 | `esp:4` → `bb1:18t.a` |
| Resistenza 220 Ω | e18 ↔ e12 | `r1` fra `18t.e` e `12t.e` |
| Cavo naso, "+" (jumper rosso) | c12 → gamba lunga LED | `12t.c` → `led1:A` |
| Cavo naso, "−" (jumper nero) | c11 → gamba corta LED | `led1:C` → `11t.c` |
| Ritorno LED a G | jumper a11 ↔ j2 | `11t.a` → `esp:GND` |
| GPIO5 → colonna 24 | jumper a1 ↔ a24 | `esp:5` → `24t.a` |
| Cavo buzzer, S | c24 → pin S | `24t.c` → `bz1:2` |
| Cavo buzzer, − | c22 → pin − | `bz1:1` → `22t.c` |
| GPIO6 → colonna 22 | jumper a2 ↔ a22 | `22t.a` → `esp:6` |

Catene:

```
GPIO4 → 220 Ω → LED (nel naso) → G
GPIO5 → S → piezo → − → GPIO6   (GPIO6 in opposizione a GPIO5)
```

`bz1` è il piezo generico di Wokwi a due terminali: rappresenta i due pin usati
del modulo KY-006 / HW-508 (S e −; il pin centrale non è collegato).

## Build e avvio

Dalla root del progetto, in PowerShell:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --environment wokwi
```

Aprire **questa cartella** `simulazione_supermini` come cartella di lavoro in
VS Code e scegliere `Wokwi: Start Simulator`. Il suo `wokwi.toml` punta a
`../.pio/build/wokwi/firmware.bin` e `.elf`.

Il firmware simulato (`SIMULATION_MODE`) esclude il BLE: i comandi si scrivono
nel monitor seriale virtuale, una lettera seguita da Invio (vedi la tabella dei
comandi in `STATO_PROGETTO.md`).
