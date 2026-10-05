#pragma once
#include "Minigame.h"
#include <random>

// Resonancia Dividida
// Ciclos de dos fases: individual (2-3 nodos de anillo simple, se activan con 1 token)
// y doble (1 nodo de anillo doble en el centro, requiere 2 tokens unidos).
// La distancia entre los nodos individuales varía de ciclo a ciclo (cerca / medio / lejos)
// para que separarse no sea siempre la estrategia obvia.
class Resonancia : public Minigame {
public:
	std::string id() const override { return "resonancia"; }
	std::string title() const override { return "Resonancia Dividida"; }
	void start() override;
	void update(float dt, const TokenTracker & tracker) override;
	void draw(const TokenTracker & tracker) override;
	bool isFinished() const override { return phase == FINISHED; }
	void drawResults() override;
	std::vector<std::string> instructions() const override;

private:
	enum Phase { INDIVIDUAL, DOUBLE, FINISHED };
	struct Node {
		glm::vec2 pos;
		bool isDouble = false;
		float fill = 0;
		bool completed = false;
		float completedAt = -1;
	};

	void startPhase(Phase p);
	void layoutIndividualNodes(int count, float spacing);

	Phase phase = INDIVIDUAL;
	int cycle = 0;
	float phaseTime = 0;
	std::vector<Node> nodes;
	std::vector<float> spacingSequence; // una distancia por ciclo, barajada con semilla fija
	std::vector<int> countSequence;
	std::mt19937 rng;

	int individualDone = 0, individualTotal = 0;
	int doubleDone = 0, doubleTotal = 0;
};
