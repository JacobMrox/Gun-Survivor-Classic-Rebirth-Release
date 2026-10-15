# Spec PS1: inciampo/caduta dello zombie

Fonte completa: nota di ricerca PS1_ZOMBIE_FALL_TRIGGER_AUDIT (repository di ricerca, non inclusa).
Riferimento: SLUS_010.87, banco codice zombie a `80100000`, risorsa 2 (PC: tipo 0, ENEM00.NAB/NAC).

## Dati

Le sequenze `23`, `10`, `11` sono **identiche** su PS1 e PC (tabelle 14×42, 11812 comandi,
770 chiavi verificate). Non servono nuovi asset: manca solo la logica che le attiva.

## Logica (implementata in `src/ps1/stumble_logic.*`)

**Avvio, `80102828`** (stato 4, inseguimento), a ogni update:
1. Fascia: `3000 < distanza ≤ 5000` → fascia 1 (E+358).
2. Se fascia 1 e tentativo non ancora consumato (E+35C bit 8): lo consuma **subito**.
3. `rng & 0xFF < 0x0C` (circa 4,7% per tentativo).
4. Giocatore nel settore angolare davanti allo zombie (`80100924(E,0x28)` → `8010078C`).
5. Se tutto passa: stato C, selettore `23`, blend 4 (`8008035C(E,0x0C,0x23,4)`).

**Esito, `80103C6C`**, quando il cursore della sequenza raggiunge inizio+8:
- distanza `< 0x380` **e** zombie davanti al giocatore (E+320 bit 0x40) **e** gate `80100628` → selettore `24` (contatto);
- altrimenti → stato 4, selettore `10`; al completamento `11`; poi locomozione.

## Verificato nel MIPS (Ghidra, 2026-10-15)

- **Distanza** `8007F4F4`: planare, intera, `sqrt((P.x − E+198 + off.x)² + (P.z − E+1A0 + off.z)²)`
  con `off = *(E+388)+18/+20`.
- **Fasce e riarmo** `80102828`: fascia 1 `3000 < d ≤ 5000` (conserva il bit 8), fascia 2
  `1400 < d ≤ 1600`, fascia 3 `1000 < d ≤ 1400`, fascia 4 `d ≤ 1000`; le fasce 2–4 **riarmano**
  il bit 8. Fra 1600 e 3000 e oltre 5000 fascia e bit restano invariati. All'ingresso nello
  stato 4 (`E+31C == 0`) tutto si azzera.
- **Settore** `8010078C`: raggi a heading `E+1C6 ± 0x28×0x0B` (440/4096 di giro, ±38,7°),
  segni di due prodotti vettoriali: cono simmetrico davanti allo zombie, senza limite di distanza.

## Aggancio PC (verificato in Ghidra, 2026-10-15)

- Mapper `00436E40`: `ECX = S`, un argomento, 211 `RET 4`. Per il tipo 0 con richieste 2/3/4
  (locomozione) "mantiene" il selettore: restituire 23/10/11 tiene la sequenza.
- Update `0043A8A0`: `ECX = S`, nessun argomento, 35 `RET`. O e T da `C+0C`, indici `S+10` e `C+14`.
- Uscita: cursore `Q+44` della traccia 1 contro l'inizio letto dalle coppie `Q+5C[Q+10]`.

Direzione in avanti: terza riga della matrice di O (`O+5C/+64`), verificata in gioco; `O+E0`
non è l'heading. Prima prova: il primo tentativo a d=3131 rientra nella fascia 1, coerente con
la scala 1:1.

Le ASSUNZIONI rimaste (scala 1:1, T = giocatore, cursore) sono nei commenti di
`src/features/ps1_zombie_stumble.cpp` e si verificano dal log della prova.
