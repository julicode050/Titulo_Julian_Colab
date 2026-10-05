#pragma once
#include "Minigame.h"

// Ensamblaje Progresivo
// Cada ciclo muestra una figura en el centro, cortada en N fragmentos de formas distintas
// que esperan a los lados de la zona central. Un token recoge un fragmento al tocarlo y lo
// lleva consigo. Cada fragmento tiene un único lugar en la figura (rompecabezas): se fija
// cuando el token que lo lleva está sobre ese lugar y se une a otro token. Un fragmento por
// unión: hay que separarse y volver a unirse para fijar el siguiente. Si el lugar es
// incorrecto, el fragmento tiembla y la unión no se gasta (pueden deslizarse al lugar correcto).
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
		int index = 0;
		glm::vec2 slot; // su único lugar en la figura (px)
		glm::vec2 pos;
		int carrier = -1; // uid del token que lo lleva
		bool fixed = false; // ya está en su lugar de la figura
		float fixedAt = -1;
		int rejectSlot = -1; // lugar incorrecto donde se intentó fijar (-1 = no se está rechazando)
		float rejectAt = -1;
		// Tokens que ya estaban encima al aparecer el fragmento: no pueden recogerlo
		// hasta salir de su radio (evita recoger sin intención).
		std::set<int> blocked;
	};

	void startCycle();
	bool touches(const Fragment & f, const glm::vec2 & p) const;
	int nearestSlot(const glm::vec2 & p) const;
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
