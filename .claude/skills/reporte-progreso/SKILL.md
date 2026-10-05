---
name: reporte-progreso
description: Generate Julián's periodic software progress report (in Spanish) for his thesis ("Memoria de Título"), covering progress and decisions since the last report. Use when he asks for a "reporte de progreso", "reporte", "informe de avance", or invokes /reporte-progreso.
---

# Reporte de progreso

A periodic report Julián uses as raw material for the "Desarrollo del prototipo" section of his Design thesis (UDD). Each report covers **what changed since the previous report**, and also restates the current overall state so it can be read on its own.

## 1. Find the baseline
- Previous reports live in `docs/reportes/YYYY-MM-DD.md`. Open the most recent one. Its front matter has `cubre_hasta_commit`.
- If there is no previous report, the baseline is the start of the repository.

## 2. Gather evidence (code first, documents second)
1. `git log <cubre_hasta_commit>..HEAD --format="%h | %ad | %s" --date=format:"%Y-%m-%d"` for the chronology, plus `git diff --stat` for scope.
2. **Read the code itself** to confirm what exists and works: `src/` (especially `TokenTracker`, `Ensamblaje`, `Resonancia`, `Marea`, `ofApp`) and `bin/data/settings.json` for current parameter values. The report describes the real state of the code, not what the briefs or older reports say. If a document and the code disagree, the code wins, and the report says so.
3. `docs/DECISIONES.md`: entries dated after the previous report.
4. `PENDING.md`: items added, changed or ticked since the previous report (`git diff <cubre_hasta_commit> -- PENDING.md`).
5. Uncommitted changes (`git status`): mention them, marked as not yet committed.

## 3. Ask before writing
Testing that happened away from the computer leaves no trace in the repo. Before writing, ask Julián (one short question, with AskUserQuestion if useful) whether since the last report he has done:
- bodystorming or tests at the table (alone or with participants; with which physical tokens);
- design decisions outside the code that should be recorded.

Never claim a test happened unless it is in the repo, in `DECISIONES.md`, or Julián said so. Verification done only in the desktop simulator must be called that.

## 4. Write the report (Spanish, clear prose, no code)
Start with one line naming the audience: Julián, as input for his thesis. Then:

**Encabezado:** period covered (date and commit of the previous report → today and `HEAD`).

**Cronología:** table of the period's commits (date, commit, advance in plain words).

1. **Qué existe y funciona.** Overall state of the detection layer and each of the 3 minigames, highlighting what changed this period. The games are **prototypes whose visuals and mechanics are still being adjusted**; never call them finished unless Julián says so. Include a "Cómo se probó" subsection (simulator / Julián's solo bodystorming / table with physical tokens / participants), saying honestly what has *not* been tested.
2. **Decisiones técnicas y de diseño del período.** Each decision: what was ambiguous or problematic, what was decided, why. Include decisions reverted within the period, and why they were reverted (that is valuable for an RtD thesis).
3. **Limitaciones o incompatibilidades.** New ones found, and earlier ones resolved this period.
4. **Qué falta.** Updated list per minigame and in general, including uncalibrated parameters with their current value from `settings.json`.
5. **Estado de los requisitos no negociables.** Confirm each one explicitly, and flag any that changed: 2 people (not 4); ~60 cm area; no motors or actuators; abstract software without narrative; configurable number of pieces (not hardcoded); Marea's fusion implemented via a configurable threshold, not validated with 4 people; no automatic logging, but structured to add it later.

Use the project's terms consistently: Ensamblaje Progresivo, Resonancia Dividida, Marea de Presión, Mecánica B, tokens A/B, "unidos". Prefer dates and commit hashes over vague time references.

## 5. Save and close
1. Save the report to `docs/reportes/YYYY-MM-DD.md` (today's date) with this front matter:
   ```
   ---
   fecha: YYYY-MM-DD
   cubre_desde_commit: <cubre_hasta_commit of the previous report, or "inicio">
   cubre_hasta_commit: <short hash of HEAD>
   ---
   ```
2. Deliver the report in chat as well.
3. Add any decisions Julián mentioned in step 3 to `docs/DECISIONES.md`.
4. Ask whether to commit and push the report; don't do it unprompted.
