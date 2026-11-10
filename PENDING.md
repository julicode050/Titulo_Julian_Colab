# Pending — things Julián will do

Commitments made while starting the minigame sprint (Oct 2026). Tick them off as they're done.

## 1. Build the new physical tokens
- [ ] Fabricate tokens **A** and **B** so they are detected on their own *and*, when interlocked, are recognized as one unit.
  - Plan: fabrication starts the week of 2026-10-05. On Monday 2026-10-05 Julián asks last year's student for his 3D models, to use as a starting point.
  - Status 2026-10-15: pad layout defined (docs/TOKENS.md, v1); Julián is designing the token bodies in Fusion.
- [x] Share their geometry (2026-10-15): full spec in [docs/TOKENS.md](docs/TOKENS.md). A = equilateral, 30 mm sides; B = scalene, 35 / 15 / 23.25 mm; Ø5 mm pads. When interlocked, the 6 pads interleave in a roughly circular layout, 42.5 × 38.2 mm.
- [ ] **Probe tests before fabricating** (see TOKENS.md): two Ø5 mm pads at **11 mm** between centres must read as 2 contacts; a Ø5 mm pad must register reliably; A and B alone must read as 'A' and 'E' in the debug panel.
- [x] **Interlocked-pair recognition by size matching** (2026-10-15, branch `tokens-v1`): `TokenTracker` tries the 10 ways of splitting 6 points into two trios and picks the best match with A's and B's side lengths (`tokenSidesMm`). It counts them as interlocked only if their centres are at the interlock distance (13.2 ± 3 mm). Otherwise they're just close. Falls back to the old spatial split for the current tokens. Verified in the simulator.
- [x] Token B mapped to 'E' (`tokenLetters` B = "BCE", keeps the current B working) and simulator updated with the new shapes (T key switches new / current tokens).
- [ ] **Verify the recognition with the real new tokens** once fabricated, including printing error and screen noise, and calibrate `pairFitTolMm` (2.5) and `interlockedTolMm` (3). Pad positions must be accurate to about **1–1.5 mm**: recognition drops to ~85 % at 2 mm error per pad. (Report 2026-10-15.)
- [ ] **Branch `tokens-v1` stays separate from `main`** (decided 2026-10-15). Julián merges it into `main` himself after testing with the fabricated tokens. Until then `main` keeps the current detection (proximity joining at 5 cm, spatial split). To work on the new tokens: `git checkout tokens-v1`; to go back: `git checkout main`.

Until then, "joined" is inferred by proximity: two tokens whose centres are closer than `joinDistanceCm` (5 cm), or whose contacts merge into one group of ≥ `joinedMinPoints` (6) points. This means tokens that merely pass close to each other also count as joined for a moment. With the new tokens, `joinDistanceCm` can probably be set to `0`.

- [ ] **Electrical isolation when interlocked (confirmed by Julián 2026-11-10):** each token only registers while *its own* owner touches it, also when interlocked. If one person lets go of their token while interlocked, the screen stops seeing its 3 pads and the software logs a **Separación** even though the tokens are still physically joined. Julián identifies these cases when reviewing the log and the video after the session.

## 2. Report on occlusion
- [ ] During testing, tell Claude whether a token physically covering what's drawn under it (fragments, nodes, points) is a problem. All games currently use the token's own centre, with no offset.

## 3. Review the gap-filled ambiguities while testing
All values below live in `bin/data/settings.json` and can be changed without recompiling.

**Ensamblaje Progresivo**
- [ ] Central zone is a **10.5 cm radius**, around a figure of **8.5 cm radius** (both enlarged from 6 / 4.5 cm on 2026-10-04 so puzzle places are far enough apart for a token to aim at). The brief's 25–30 cm radius doesn't fit a ~29.6 cm tall screen. Pieces wait in columns on both sides of the zone.
- [ ] A token carries one fragment at a time and **keeps carrying it inside the zone**. With only 2 tokens, players have to make several trips.
- [ ] **One fragment is fixed per union:** when a token carrying a fragment is joined to another token inside the zone (`minJoinedTokens` = 2), that fragment is fixed into the figure. To fix the next one, the tokens must separate and join again. If both tokens carry a fragment, a union fixes only one. The figure completes when the last fragment is fixed.
  - History: 2026-10-01 fragments snapped into place with a single token. 2026-10-03 (first fix) they were dropped loose on entering the zone and fused all at once on joining, but that broke the game: the fragment was released at the zone border and couldn't be picked up again. 2026-10-03 (current) one fragment per union, as Julián asked.
- [ ] **Puzzle matching (2026-10-04):** each piece has exactly one place in the figure. Figures are cut at irregular angles, so every piece has its own shape. A piece is fixed only if the token carrying it is over its own place (within `matchMaxDistCm` = 3.5 cm) when the union happens. Only the carrying token has to be inside the zone; the partner can join from outside it.
- [ ] **Wrong place:** the piece shakes and flashes red, and the union is **not** used up. While still joined, the tokens can slide to the correct place and the piece fixes there.
- [ ] **Place outlines fade over the cycles** (`slotOutlineAlpha` = 1, 1, 0.6, 0.3, 0, 0): fully visible in cycles 1–2, fainter in 3–4, only the silhouette in 5–6.
- [ ] **Test the sizes at the table with the real tokens (Julián, week of 2026-10-05):** figure radius 8.5 cm, zone 10.5 cm, match distance 3.5 cm, and whether the token covering its target place (occlusion) makes aiming hard.
- [ ] 💡 **Future option, not implemented: rotation matching.** The piece turns with the token and must also be oriented correctly. On hold because the software can't tell token A's rotation apart in 120° steps (equilateral triangle), and joined tokens turn together, which makes fine rotation hard. Worth revisiting with the new tokens.
- [x] 💡 **Mechanic idea, implemented 2026-10-03 as "one fragment fixed per union"** (see above): players bring a fragment into the zone, then connect their tokens to fix it; repeat for each fragment.
- [ ] A token picks up a piece by touching it (inside its shape, or within 2.5 cm of its centre). A token already resting on a piece when a cycle starts can't pick it up until it moves off it.
- [ ] Success = **time per cycle**, shown after each figure. No time limit per cycle. 6 cycles with 3, 3, 4, 4, 5, 5 fragments, and a different shape each cycle.

**Resonancia Dividida** (rules changed 2026-11-10, task 4)
- [ ] **6 cycles of 20 s (2 min).** Each cycle shows **3–4 single nodes** (1 token, fill 2 s, **1 pt**) and **1 optional double node** at the centre (joined pair, fill 2.5 s, **3 pts**). The old separate "double phase" is gone, so joining is no longer forced.
- [ ] Single-node spacing 6 / 14 / 24 cm, balanced (each twice) and shuffled with a **fixed seed**. If 4 nodes don't fit at a cycle's spacing (always the case at 24 cm), fewer nodes are used; the spacing is never shrunk. Single nodes stay ≥ 7 cm from the double node (`centerClearCm`).
- [ ] Activation radius 3.5 cm; fill drains at 0.25/s when the token leaves. A joined pair can also activate single nodes. Completed nodes don't respawn within a cycle.
- [ ] Calibrate in bodystorming whether 20 s per cycle really makes splitting up necessary, and the double node's value (3).

**Marea de Presión**
- [ ] Session 330 s. At most 3 points at once, a new one every 6–12 s. Each point lasts 40 s.
- [ ] Fill starts at 60 %, decays 0.04/s. Fill gain is +0.05/s with 1 token, **+0.125/s joined**. Score is 1/s low mode, **2.5/s high mode**. **A point that empties costs −10** (`lostPointPenalty`); one that expires costs nothing. (Changed 2026-11-10, task 3.)
- [ ] A point disappears when empty or at the end of its lifetime.
- [ ] Two tokens on the same point but *not* joined = low mode.
- [ ] Fusion: 2 points closer than 8 cm merge when a joined group of ≥ `fusionMinTokens` (4) covers both. Never happens with 2 people (expected).

**General**
- [ ] Menu buttons are chosen by holding a token on them for 1.5 s (or a mouse click / keys 1–3).
- [ ] On-screen text faces one side of the table only.

## 4. Screen measurements
- [x] Screen identified (2026-10-04): **ViewSonic TD2455** (model VS17978), 23.8", 1920 x 1080, pixel pitch 0.2745 mm, visible area 527 x 296.5 mm. `pxPerCm` set to **36.43** (was 36.5). The hand-measured 50.3 cm width was off; the real visible width is 52.7 cm.

## 5. Open issues from the progress report (2026-10-03)

**Testing**
- [ ] Systematic testing of all three games on the table with the physical tokens. So far: automated runs in the desktop simulator and Julián's preliminary solo bodystorming.
- [x] Bodystorming with other people (first sessions before 2026-10-15). **Finding:** with the current tokens the experience isn't good enough, because they don't interlock, so the game doesn't encourage working as a team. This supports building the new interlocking tokens.
- [ ] **More bodystorming, and take photos for the final report** (Julián, requested 2026-10-15).
- [ ] Bodystorming with the new interlocking tokens once they're fabricated, to check whether interlocking does encourage teamwork.

**Detection (unresolved limitations)**
- [ ] The software can't tell "interlocked" from "very close" (see section 1). **Solved in the simulator** on branch `tokens-v1` (2026-10-15); still to verify with the real new tokens. Proximity joining is now off (`joinDistanceCm` = 0).
- [x] ~~Splitting a merged cluster assumes 3 contact points per token.~~ The new tokens keep 3 points each (2026-10-15), so the assumption holds.

**Research workflow**
- [ ] There's no time reference on screen to sync the game with the session video. The brief asks to point to specific moments in the interview. Consider a small session clock, or rely on automatic logging once it exists.

**Ensamblaje Progresivo**
- [ ] Calibrate the pickup radius (2.5 cm); figure, zone and match sizes are covered by the table test above.
- [ ] Calibrate how the place outlines fade (`slotOutlineAlpha` = 1, 1, 0.6, 0.3, 0, 0 per cycle). (Report 2026-10-04.)
- [ ] Re-test the rule that only the **carrying** token must be inside the zone. It was added after the last simulator run on 2026-10-04 and hasn't been tested since. (Report 2026-10-04.)
- [ ] **No fixed seed** for the starting piece positions **or the cut angles of the figures** (unlike Resonancia and Marea), so every session gets differently shaped pieces. With puzzle matching, difficulty depends on piece shape, so sessions aren't comparable. Fine for bodystorming; consider a fixed seed before sessions with participants. (Report 2026-10-04.)

**Resonancia Dividida**
- [x] ~~Session length vs. the brief's 6–7 min~~ → Julián set Resonancia to 2 min (2026-11-10).

**Marea de Presión**
- [ ] Calibrate the exact visual difference between low and high mode (currently colour, ring thickness and pulse speed).

**Tooling**
- [ ] The Xcode project (`Titulo3D.xcodeproj`) doesn't include the new source files. Build with `make`, or regenerate the Xcode project if needed.

## 6. Later
- [ ] Confirm the final number of tokens (2 for now, one per person).
- [ ] ~~Automatic event logging~~ → now in scope: task 2 of the 2026-11-09 plan (section 7).
- [ ] Scaling to 4 people (out of scope this sprint). Note: the interlocked-pair recognition assumes exactly **two** tokens of 3 pads each; it would need generalizing for more tokens. (Report 2026-10-15.)

## 7. Plan after the research update (2026-11-09)
New hypothesis: a pair's interaction quality is higher when working with the tokens interlocked than when working apart (compared within each pair). The pair chooses when to interlock; the software must not force it, except to fix pieces in Ensamblaje. Easy interlocking is a requirement. Data goes to an Excel sheet via an event log.

All tasks are done on branch `tokens-v1` (Julián, 2026-11-10), merged into `main` in task 7.

- [x] **1. Diagnostic mode** (2026-11-10): **P** key. Raw contacts with stable numbers and positions in mm, distances between contacts (centre to centre and edge to edge), min/max contacts in the last 5 s, counts of appearances / disappearances / brief contacts (< 0.5 s), what the system recognizes (A, B read as E, interlocked pair), and a timed log of contact and token events. **C** resets the counters. No game runs while it's on. Verified in the simulator; still to use with the real probes.
- [x] **2. Event log and session clock** (2026-11-10):
  - **Session start:** the app opens asking for the pair code (typed, Enter). The session clock starts there and shows `P01 mm:ss` in the **top-right corner**. **N** closes the session and asks for a new code.
  - **CSV:** one file per session in `bin/data/registros/<code>_<date-time>.csv`, `Pareja;Juego;t (s);Evento;Token`, UTF-8 with BOM. Separator, decimal and folder are in `settings.json` → `registro`.
  - **Events → token:** Inicio / Fin de juego (No aplica), Unión / Separación voluntaria (Par), Pieza fijada (Par), Nodo completado (A, B or Par), Punto perdido (only when a point empties, not when it expires), Cambio de fase (Resonancia, not counting the first phase).
  - **Extra event, asked by Julián: «Juego interrumpido»** when a game is left halfway (M, R, another game, N or quitting the app).
  - **Outside a game nothing goes to the main CSV.** Token appear/disappear goes to `<...>_tokens.csv` (Juego = «Menú» outside games).
  - `bin/data/registros/` is **excluded from Git**, so participants' data never goes to GitHub.
  - Verified with a simulated session. «Pieza fijada» wasn't exercised in that run.
- [ ] Check at the table that the clock box (top-right, 7.5 × 1.9 cm) and the score (top-centre, Marea and Resonancia) don't cover nodes or pressure points near the top edge; if they do, keep games out of those areas.
- [x] **3. Marea** (2026-11-10): the pair now fills and scores **2.5×** a single token (0.125/s fill, 2.5 pts/s), and a point that empties costs **−10**, shown as a red "−10" over the lost point and in the results. Verified in the simulator. With one active point joining pays off (2.5 vs 1/s); with two, splitting up does (≈30 vs 27.5 over the ~15 s it takes the neglected point to empty).
- [ ] Calibrate the Marea balance in bodystorming: check whether pairs really face a choice, and adjust `highScorePerSec`, `lostPointPenalty` and `decayPerSec` if they don't.
- [x] **4. Resonancia** (2026-11-10): one phase per cycle with 3–4 single nodes (1 pt) + 1 optional double node (3 pts); 6 cycles of 20 s = 2 min; spacing 6 / 14 / 24 cm kept (seeded, never shrunk). Score shown during the game and in the results. Verified in the simulator.
- [ ] **5. Ensamblaje and game order:** one piece per union is already implemented (`ac5b32b`). Add a 4th menu button, **"Modo testeo"**, that runs the three games in a fixed order: Ensamblaje, Resonancia, Marea. The individual game buttons stay.
- [ ] **6. Brief contact losses:** configurable tolerance for brief losses, individual and for the pair (today: 15 frames per token, separation confirmed after 10 frames), so re-gripping doesn't count as a separation.
- [ ] **7. Real tokens:** verify pair recognition, calibrate `pairFitTolMm` / `interlockedTolMm`, then merge `tokens-v1` into `main`.
- [ ] **8. Report** at the end of each working day.
