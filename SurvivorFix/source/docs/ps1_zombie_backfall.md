# Spec PS1: caduta all'indietro dello zombie (0x20 → 08 → 0E → 0F)

Fonte: banco zombie PS1 a `80100000` (`ENEMY_SLOT40.bin`), Ghidra headless, 2026-10-15.
Logica pura in `src/ps1/backfall_logic.*`, aggancio PC in `src/features/ps1_zombie_stumble.cpp`
(stessi hook dell'inciampo), interruttore `Ps1ZombieBackFall`.

## Avvio: stato 6, `80102D74`, ramo `LAB_801030C0`

- Animazione corrente `E+85E == 0x20` e cursore Q1 `== *(P+94)[0x20] + 10` (esattamente inizio+10).
- `80011570() & 0xFF < 0xCC` (≈80%) → `80080334(E, 0x08, 4)`, `P+14 = 0x14`, sottostato +1.
- Altrimenti, a fine animazione: locomozione (`80101CE4`) oppure `800802C8(E,4,0x10)`.

## Spinta all'indietro

- Caso 0 dello stato 6: per l'animazione 0x20 (`case 0x1A`) `P+10 = 500`;
  `E+378 = avanti(E+1C4) × (±P+10/100)`, quindi modulo 5, con il segno che dipende da `+0x32B`.
- A ogni update: `E+87C += E+378 × P+14`, poi `P+14 −= 2` fino a 0: 5×(20, 18, …, 2), in totale 550 unità.

## Recupero

- Fine della 08 (`80100728`): `800802C8(E, 4, 0x0E)`.
- Fine della 0E (`80104108`, caso 4, `E+320` bit 8): `80080334(E, 0x0F, 2)`.
- Fine della 0F (caso 5): locomozione.

## PC

- La 0x20 sul PC è uno dei gesti `1B`–`21` che il mapper sceglie per la richiesta 08
  (azione a breve distanza, ANIMATIONS.md). Sul PS1 la stessa sequenza (dati identici) è trattata
  nello stato 6. **ASSUNZIONE:** lo stesso gesto ha lo stesso ruolo; la prova in gioco lo verifica.
- Velocità PC di 08/0E/0F per il tipo 0: `S+40 = 2,0` (mapper `00436FC2`, `004371F0`).
- Spinta tramite `0045FD70` (accumulatore locale, `RET 0xC`), con `−Z` locale = indietro.
  La scala PC è ignota (divide per `O+120`): `Push` nel `.ini`, default 0.
