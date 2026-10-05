# Titulo_Julian_Colab

openFrameworks 0.12.1 prototype for Julián's Design thesis (UDD): three tabletop minigames (Ensamblaje Progresivo, Resonancia Dividida, Marea de Presión) played with passive tangible tokens on a capacitive touchscreen.

## Working with Julián
- He writes in Spanish or English. Reply in the language he used. Everything participants see on screen is in Spanish.
- Build and run with `make -j8 && make run` (the Xcode project is out of date).
- All calibration values are in `bin/data/settings.json`. The detection in `Token.cpp` / `TokenTracker.cpp` is the validated core: reuse it, don't rewrite it.

## Keep the decision log up to date
Whenever a technical or design decision is made (a game rule, a detection approach, a parameter choice, a reverted idea), add an entry to `docs/DECISIONES.md` in Spanish, in that file's format: date, decision, reason, commit. Do it in the same session as the decision. The periodic progress reports depend on this log, because decisions made in conversation leave no trace in Git.

## Progress reports
When Julián asks for a progress report ("reporte de progreso"), follow `.claude/skills/reporte-progreso/SKILL.md`. Reports are saved in `docs/reportes/`.

## Pending work
`PENDING.md` lists Julián's open commitments and the rules he still has to review while testing. Update it when items change.
