#pragma once
#include "Minigame.h"

// Ensamblaje Progresivo
// Cada ciclo muestra una figura en el centro, dividida en N fragmentos dispersos cerca
// de los bordes. Un token recoge un fragmento al tocarlo y lo suelta al entrar en la zona
// central, donde queda flotando suelto (sin armarse). Con todos los fragmentos en la zona,
// los tokens deben unirse dentro de ella: solo entonces los fragmentos se fusionan en la figura.
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
		bool deposited = false;
		glm::vec2 loosePos; // dónde queda flotando en la zona hasta que los tokens se unen
		float bobPhase = 0;
		// Tokens que ya estaban encima al aparecer el fragmento: no pueden recogerlo
		// hasta salir de su radio (evita recoger sin intención).
		std::set<int> blocked;
	};

	void startCycle();
	bool zoneHasJoinedGroup(const TokenTracker & tracker) const;
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
};
