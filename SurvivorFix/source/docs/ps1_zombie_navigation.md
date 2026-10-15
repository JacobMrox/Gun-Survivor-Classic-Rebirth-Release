# Spec PS1: navigazione dello zombie in inseguimento (stato 4)

Fonte: Ghidra headless sul progetto PS1 (`out/ps1-activation/ghidra/`), banco zombie
`ENEMY_SLOT40.bin` a `80100000` e `TEST.BIN`, letto il 2026-10-15. Solo lettura: nessun codice PC.
Angoli in unità PS1 (4096 per giro); `E` = entità, `P` = stato ausiliario (`*80105FBC`),
giocatore a `800A85D8` (x) / `800A85E0` (z), heading dello zombie `E+1C6`.

**Risposta breve:** inseguimento diretto con velocità di rotazione limitata e un controllo di
linea di vista contro una **lista di segmenti di muro 2D**. Se un muro taglia la linea verso il
giocatore e anche il raggio davanti è bloccato, lo zombie **ruota sul posto** di circa 1,9° per update
verso il lato del giocatore, finché il raggio davanti non è libero, poi riparte dritto.
**Niente waypoint né pathfinding**, e in questa logica nessuno scivolamento sui muri.

## Struttura dello stato 4: `801027C0`, a ogni update

1. `80100AE8(E.pos, giocatore.pos)`: linea di vista (sotto). Libera → `8007F210`; bloccata → `80100E24`.
2. `80101D44`: uscita dallo stato (parametri del preset, `80082C38(E,k)`): zombie fuori dal settore
   davanti al giocatore, contatore `E+852` oltre `preset[6]` o distanza oltre `preset[4]` →
   `800802C8(E, preset[3], locomozione)`.
3. `80101130`: la testa segue il giocatore (`E+364`, ±0x400, passo 0x14).
4. `80101948`: presa da dietro. Con `d ≤ 1000`, lo zombie fuori dal settore del giocatore e il gate
   `80100628` → `8008035C(E,0x0E,…)`, poi pausa di 0xF0 update (`80105FC0`).
5. Poi `80102828` (inciampo e azioni a distanza, vedi [ps1_zombie_stumble.md](ps1_zombie_stumble.md)).

## Linea di vista: `80100AE8(a, b)`

Scorre `DAT_800B60C8` segmenti a `*800B60CC`, record da 16 byte `(x1, z1, x2, z2)` interi. Ritorna
l'indice del primo segmento che il segmento `a→b` attraversa, con `a` sul lato "frontale" del muro
(prodotto vettoriale ≤ 0). Ritorna −1 se non ne attraversa nessuno. Solo piano XZ, nessuna altezza.

## Linea libera: sterzata diretta `8007F210`

- Velocità: `E+87C/E+884 = avanti(E+1C4) × E+866` (`8008286C`).
- `c = cos(velocità, verso il giocatore)` in 1/4096. Passo di rotazione per update secondo `c×10`:
  ≤0,1 (o giocatore dietro) **0x28** (3,5°), poi 0x24, 0x20, 0x1C, 0x18, 0x14, 0x10, 0x0C, 8, e
  0,9–1,0 **6** (0,5°). Il verso è dato dal segno di `8007C04C`.
- È un inseguimento "a ricerca" verso la posizione **attuale** del giocatore: niente previsione.

## Linea bloccata: aggiramento `80100E24`

Modo in `P+0` (0 = dritto, 1/2 = ruota):

- **Modo 0:** sonda davanti, `80100C90(E,100)`: punto a `avanti × 100 × 0x14` dallo zombie, poi
  `80100AE8`. Se è libero e il giocatore è nel cono di ±0x5A×0x0B (≈87°), cammina dritto
  (`E+87C = avanti × E+866`) anche senza linea di vista.
- Altrimenti sonda a +90° e −90° (`E+1C6 ± 0x400`), guarda da che lato sta il giocatore
  (`80100D40`) e la distanza dal muro trovato (`80100DC8`, soglia 500). Sceglie il modo 1 (gira +) o
  il modo 2 (gira −).
- **Modi 1/2:** `E+1C6 ±= 0x16` (≈1,9°) a ogni update (sonda: `8008286C` dà un vettore lungo `len`, quindi 2000); dopo ogni passo sonda davanti, e se il raggio
  è libero torna al modo 0. In questo ramo la velocità non viene riscritta.

## Dati: `ROOMENV.DAT`, e il PC li ha già

`8008DAD0` carica il record di stanza da `ROOMENV.DAT` (indice `stage×0x1E + stanza`, tabella
`800A32F0`). Nella sezione all'offset `+4`: `n1` (byte), `n2` (`+2`), `n3` (`+4`), poi `n1` **muri** da 16 byte
(`800B60CC`, usati dal movimento) e `n2` **ostacoli alla vista** da 0x14 byte (`x1,z1,x2,z2` + attributo `+0x10`,
`800B60D4`, usati da `80100944`/`8007FA98` per l'attivazione), seguiti da altri due blocchi da 0x14 (`800B60DC`,
`800B60E4`: zone, non navigazione).

**`Resource/ROOMENV.DAT` del PC ha lo stesso formato e gli stessi record.** Tutte le 102 stanze PS1 che hanno muri
ci sono, con contenuto identico ma offset diversi: l'indice stanza→offset PC è in `src/roomenv_index.h`.
**Il PC non lo legge:** in `gun_bio.exe` non c'è nessun riferimento al file, e con il gioco aperto nella stanza 7/0 i
record non sono in memoria (scansione in sola lettura, 2026-10-15).

**Coordinate: stessa numerazione delle stanze, Z specchiata: `x_PC = x_PS1`, `z_PC = −z_PS1`.** Misura in gioco del
2026-10-15 con `RoomObserver` nella stanza **7/0** (`PSX_DATA/stage/STAGE07/0700`, primo zombie, spawn PC `(8200, −100, 1000)`).
Con la Z negata il giocatore nel vicolo `(427, −3634)`, lo spawn e lo spigolo della ricerca (facce PC x = 6500 e
z = −2028, cioè PS1 x = 6500 e z = 2028) cadono tutti nel vicolo a L dei dati PS1; senza negazione no (una prima prova
con le coordinate non convertite ha mandato lo zombie contro un muro). Lo stesso segno compare nella traslazione del
modello PC nel visualizzatore della ricerca. I muri di movimento sono le pareti spostate verso l'interno di circa 900–1000
(raggio più margine). Sul PS1 anche il giocatore si muove contro questi muri (`8007CB9C`, chiamata da `800785C0` e
`8008369C`; gli zombie da `8007F404`, con scivolamento `8007C60C`), quindi non entra mai nel margine; sul PC sì, e il port
ignora nella linea di vista i muri a meno di `PlayerWallMargin` dal giocatore.

## PS1 contro PC, passo per passo (2026-10-15)

| | PS1 (stato 4) | PC (`0043A8A0`, tipo 0) |
|---|---|---|
| Linea libera | `8007F210`: **cammina e intanto sterza** verso la posizione attuale del giocatore, 0,5–3,5° per frame secondo l'allineamento | Bersaglio portato nel locale dello zombie (non normalizzato): se lo scarto laterale è **≥ 200 unità** (o il bersaglio è dietro e fuori quota di ±2000) **ruota fermo di ±3°** (richieste 3/4), altrimenti **cammina dritto senza sterzare** (richiesta 2) |
| Muro in mezzo | `80100E24`: dritto se davanti (2000) è libero e il giocatore è entro ±87°; altrimenti sceglie il lato e **ruota fermo di 1,93° per frame** finché davanti è libero | Nessuna differenza: punta comunque il giocatore |
| Movimento e muri | `8007F404`: la velocità si azzera a ogni frame; lo spostamento passa per `8007CB9C` contro **gli stessi muri di ROOMENV** e, al contatto, **scivola lungo il muro** | Collisione con le mesh (`004609D0`); nessun dato di ROOMENV; agli spigoli il movimento viene azzerato e lo zombie resta fermo (ricerca T3/T7) |
| Giocatore | Si muove contro gli stessi muri (`8007CB9C`): non entra mai nel margine vicino alle pareti | Collisione con le mesh: entra nel margine |
| Cadenza | Probabilmente 30 frame/s (`DAT_80062E1C = 2` prima dell'attesa del frame; da confermare) | ~29,4 update/s (T6, limitatore da 34 ms; i ~20/s della misura con quattro breakpoint erano falsati) |

**Port dal 2026-10-15 (sera):** la direzione dello zombie è calcolata interamente dalla logica PS1 (`8007F210` a linea
libera, `80100E24` con il muro) e scritta nella matrice di `O` prima e dopo l'AI PC; l'AI PC mira dritto davanti per
camminare o di lato per girare da fermo (con la sua animazione). Rotazioni per update × `RateScale` (1,0: stessa cadenza). Resta fuori
lo scivolamento sui muri di ROOMENV (fisica PC).

**Conseguenze della prima versione** (che spostava solo il punto di mira dell'AI PC): con la linea libera resta il moto PC
"ruota fermo / va dritto"; durante l'aggiramento il PC segue la direzione PS1 a scatti (ruota fermo finché lo scarto
è < 200 a 3000 di distanza, cioè ±3,8°). Lo zombie non scivola sui muri di ROOMENV come sul PS1, perché la fisica PC non li
conosce. Per avvicinarsi al PS1 servirebbe impostare direttamente la direzione dello zombie (matrice `O+3C`, righe da
16 byte, terza riga = avanti) con la logica PS1 completa (`8007F210` e `80100E24`), lasciando al PC solo l'avanzamento.

## Aperto

- Se `E+87C` si azzera a ogni frame: decide se nei modi 1/2 lo zombie gira fermo o scivola.
- Cadenza: l'AI PC gira a circa 20 update/s (T6), il PS1 probabilmente a 30 frame/s; a parità di tempo
  `TurnStep` dovrebbe essere circa 2,8 invece di 1,93. Da verificare la cadenza PS1.
- Risposta alle collisioni con i muri (fisica, fuori da questo stato): lo "scivolamento" va cercato lì.
