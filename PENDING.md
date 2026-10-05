# Pending — things Julián will do

Commitments made while starting the minigame sprint (Oct 2026). Tick them off as they're done.

## 1. Build the new physical tokens
- [ ] Fabricate tokens **A** and **B** so they are detected on their own *and*, when interlocked, are recognized as one unit.
  - Plan: fabrication starts the week of 2026-10-05. On Monday 2026-10-05 Julián asks last year's student for his 3D models, to use as a starting point.
- [ ] When they exist, share their geometry so detection can be tuned in `bin/data/settings.json` (`deteccion` section):
  - contact pattern of each token (number of points, spacing in cm)
  - what the interlocked pair looks like to the screen (how many points, how far apart)

Until then, "joined" is inferred by proximity: two tokens whose centres are closer than `joinDistanceCm` (5 cm), or whose contacts merge into one group of ≥ `joinedMinPoints` (6) points. This means tokens that merely pass close to each other also count as joined for a moment. With the new tokens, `joinDistanceCm` can probably be set to `0`.

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

**Resonancia Dividida**
- [ ] Activation radius 3.5 cm. Individual node fills in 2 s, double in 2.5 s. Fill drains at 0.25/s when the token leaves.
- [ ] A joined pair can also activate individual nodes.
- [ ] Completed nodes don't respawn within a phase.
- [ ] Individual-node spacing is 6 / 14 / 24 cm, balanced across cycles and shuffled with a **fixed seed** (every session gets the same sequence). 2–3 nodes per cycle.
- [ ] 7 cycles of 27 s individual + 18 s double (~5.3 min).
- [ ] The double node activates when the joined pair's centre is within the activation radius.

**Marea de Presión**
- [ ] Session 330 s. At most 3 points at once, a new one every 6–12 s. Each point lasts 40 s.
- [ ] Fill starts at 60 %, decays 0.04/s. Fill gain is +0.05/s with 1 token, +0.15/s joined. Score is 1/s low mode, 4/s high mode.
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
- [ ] Bodystorming with two people.

**Detection (unresolved limitations)**
- [ ] The software can't tell "interlocked" from "very close" (see section 1). This is resolved by the new tokens.
- [ ] Splitting a merged cluster into its tokens assumes **3 contact points per token**. If the new tokens use a different number of contacts, this assumption must be adjusted.

**Research workflow**
- [ ] There's no time reference on screen to sync the game with the session video. The brief asks to point to specific moments in the interview. Consider a small session clock, or rely on automatic logging once it exists.

**Ensamblaje Progresivo**
- [ ] Calibrate the pickup radius (2.5 cm); figure, zone and match sizes are covered by the table test above.
- [ ] Fragment positions use **no fixed seed** (unlike Resonancia and Marea), so every session differs. Consider seeding them to make sessions comparable.

**Resonancia Dividida**
- [ ] The session lasts ~5.3 min, slightly under the brief's 6–7 min. Raise `cycles` (e.g. to 8) if needed.

**Marea de Presión**
- [ ] Calibrate the exact visual difference between low and high mode (currently colour, ring thickness and pulse speed).

**Tooling**
- [ ] The Xcode project (`Titulo3D.xcodeproj`) doesn't include the new source files. Build with `make`, or regenerate the Xcode project if needed.

## 6. Later
- [ ] Confirm the final number of tokens (2 for now, one per person).
- [ ] Automatic event logging (CSV/JSON). Out of scope this sprint, so Julián logs manually. The code is ready: listen to `Events::token()` and `Events::game()` in `src/Events.h`.
- [ ] Scaling to 4 people (out of scope this sprint).
