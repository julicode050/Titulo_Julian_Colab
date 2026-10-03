#pragma once
#include "Minigame.h"

// Ensamblaje Progresivo
// Cada ciclo muestra una figura en el centro, dividida en N fragmentos dispersos cerca
// de los bordes. Un token recoge un fragmento al tocarlo y lo lleva consigo, también dentro
// de la zona central. Cuando un token con fragmento se une a otro dentro de la zona, ese
// fragmento se fija en la figura: un fragmento por unión (hay que separarse y volver a unirse
// para fijar el siguiente). La figura se completa al fijar el último fragmento.
class Ensamblaje : public Minigame {
public:
	std::string id() const override { return "ensamblaje"; }
	std::string title() const override { return "Ensamblaje Progresivo"; }
	void start() override;
	void update(float dt, const TokenTracker & tracker) override;
	void draw(const TokenTracker & tracker) override;
	bool isFinished() const override { return phase == FINISHED; }
	void drawResults() override;

private:
	enum Phase { PLAYING, ASSEMBLING, RESULT, FINISHED };
	struct Fragment {
		std::vector<glm::vec2> shape; // relativo a su propio centroide
		glm::vec2 slot; // posición final (px)
		glm::vec2 pos;
		int carrier = -1; // uid del token que lo lleva
		bool fixed = false; // ya está en su lugar de la figura
		float fixedAt = -1;
		// Tokens que ya estaban encima al aparecer el fragmento: no pueden recogerlo
		// hasta salir de su radio (evita recoger sin intención).
		std::set<int> blocked;
	};

	void startCycle();
	bool groupInZone(const TokenTracker & tracker, const TokenTracker::Group & g) const;
	static void drawPolygon(const std::vector<glm::vec2> & pts, const glm::vec2 & at, bool filled);

	Phase phase = PLAYING;
	int cycle = 0;
	float phaseTime = 0;
	float cycleTime = 0;
	bool freshCycle = false;
	glm::vec2 center;
	std::vector<glm::vec2> figure; // contorno relativo al centro
	std::vector<Fragment> fragments;
	std::vector<float> cycleTimes;
	std::set<int> spentUnion; // tokens cuya unión actual ya fijó un fragmento
};
