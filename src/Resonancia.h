#pragma once
#include "Minigame.h"
#include <random>

// Resonancia Dividida
// Ciclos cortos, cada uno con 3-4 nodos individuales (anillo simple, se activan con 1 token) y
// 1 nodo doble opcional al centro (anillo doble, requiere los 2 tokens unidos) que vale más.
// En el tiempo del ciclo no alcanza a hacerse todo unidos: la pareja decide cuándo repartirse
// y cuándo unirse. La distancia entre los nodos individuales varía de ciclo a ciclo
// (cerca / medio / lejos), para que separarse no sea siempre la estrategia obvia.
class Resonancia : public Minigame {
public:
	std::string id() const override { return "resonancia"; }
	std::string title() const override { return "Resonancia Dividida"; }
	void start() override;
	void update(float dt, const TokenTracker & tracker) override;
	void draw(const TokenTracker & tracker) override;
	bool isFinished() const override { return finished; }
	void drawResults() override;
	std::vector<std::string> instructions() const override;

private:
	struct Node {
		glm::vec2 pos;
		bool isDouble = false;
		float fill = 0;
		bool completed = false;
		float completedAt = -1;
		std::string actor = "No aplica"; // quién lo estaba activando: "A", "B" o "Par"
	};

	void startCycle();
	bool layoutIndividualNodes(int count, float spacing);
	float points() const;

	bool finished = false;
	int cycle = 0;
	float cycleTime = 0;
	std::vector<Node> nodes;
	std::vector<float> spacingSequence; // una distancia por ciclo, barajada con semilla fija
	std::vector<int> countSequence;
	std::mt19937 rng;

	int individualDone = 0, individualTotal = 0;
	int doubleDone = 0, doubleTotal = 0;
};
