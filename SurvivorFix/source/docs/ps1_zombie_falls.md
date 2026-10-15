# PS1: quando lo zombie cade (avanti e indietro)

Fonte: Ghidra headless sul progetto PS1, banco zombie `ENEMY_SLOT40.bin` (`80100000`) e `TEST.BIN`,
letto il 2026-10-15. Solo lettura statica: nessuna osservazione in emulatore.
`E` = entità, `+0x39A` del record di colpo = tipo d'arma, `+0x32B` = lato del colpo
(0,1,6,7 davanti; 2–5 dietro), `P` = `*80105FBC`.

| Quando | Codice | Animazione | Probabilità |
|---|---|---|---|
| Inseguimento, `3000 < d ≤ 5000`, giocatore nel cono ±38,7° | `80102828` | **23** avanti → vicino: presa **24**, altrimenti **10 → 11** | 4,7% per avvicinamento ([spec](ps1_zombie_stumble.md)) |
| Fine della presa, il giocatore si libera | `800830F8` caso 7 (`TEST.BIN`): se il preset ha `param 9 & 0x8000`, colpo forzato con `E+320 \|= 0x10 \| 0x1000` | `80101474` → **0x20**, poi a inizio+10 (`80102D74`) **08 indietro → 0E → 0F** | 80% (`rng < 0xCC`) |
| Colpito da vivo mentre cammina o è fermo (animazioni 0–5, 14, 1B, 1C, 1F, 26–29) | `801012CC` → `80101474`, tabella `801059E0` | riga 0: **15** (in ginocchio, poi avanti a terra); riga 2: 07/0B; riga 3: 06/0A; riga 4: nessuna reazione, solo rotazione | riga 0 ≈ 7,8%, righe 2 e 3 ≈ 2% ciascuna, riga 4 ≈ 88% |
| Morte, arma tipo 1 | `80101700` | davanti → **08 indietro**; dietro → **0C avanti** | sempre |
| Morte, armi tipo 3/4 | `80101700` | 12,5%: davanti 06 (80%) / 0A (20%), dietro il contrario; altrimenti 0B | — |
| Morte, armi tipo 2, 7–10 | `80101700` | 0B | sempre |
| Morte, arma tipo 0 | `80101700` | stato 9 | — |

Tabella `801059E0`, indice = riga×4 + (arma 7–10 ? 2 : 0) + (dietro ? 1 : 0):
`15 15 15 15 | 08 0C 0C 08 | 07 0B 0B 07 | 06 0A 0A 06`. La riga 1 non è raggiungibile dal tiro.

Scivolamento dopo il colpo (`P+0C`, poi `P+10` nello stato 6): 1000 se `d ≤ 5000`, 500 fino a
10000, 200 oltre. Con la 0x20 `P+10 = 500`, quindi la spinta è 5×(20, 18, …, 2) unità per frame.

## Confronto con il PC

- Il PC ha una propria caduta all'indietro: richiesta `0E` → `08 → 12 → 0E → 0F` (vista in gioco).
  Ha anche cadute di reazione (richieste 0B/0C/0D → 0C → 13 ecc., ANIMATIONS.md).
- La 0x20 sul PC è uno dei gesti `1B`–`21` dell'attacco ravvicinato (richiesta 08). Il port
  `Ps1ZombieBackFall` attuale la usa come innesco: **contesto sbagliato**. Sul PS1 la 0x20 che porta alla
  08 è quella forzata alla fine della presa. Da rifare agganciando la fine della presa PC.
- Velocità: le 08/0E/0F portate girano come le native PC (≈0,34/0,34/2,25 s). La cadenza PS1
  (avanzamento per frame, probabilmente in `KERNEL.BIN` `8002F9E0`, e VSync) non è ancora letta.
