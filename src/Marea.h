#pragma once
#include "Minigame.h"
#include <random>

// Marea de Presión
// Puntos de presión aparecen escalonados y se vacían con el tiempo.
// 1 token los sostiene a tasa baja; tokens unidos, a tasa alta (más puntaje).
// Dos puntos cercanos se fusionan si un grupo de >= fusionMinTokens tokens unidos
// cubre ambos (con 2 personas no ocurre: esperado, queda listo para 4).
class Marea : public Minigame {
public:
	std::string id() const override { return "marea"; }
	std::string title() const override { return "Marea de Presión"; }
	void start() override;
	void update(float dt, const TokenTracker & tracker) override;
	void draw(const TokenTracker & tracker) override;
	bool isFinished() const override { return finished; }
	void drawResults() override;
	std::vector<std::string> instructions() const override;

private:
	enum Mode { NONE, LOW, HIGH };
	struct Point {
		int id;
		glm::vec2 pos;
		float fill;
		float age = 0;
		float value = 1; // multiplicador de puntaje (los puntos fusionados valen más)
		Mode mode = NONE;
		bool alive = true;
		float fade = 1; // 1 -> 0 al desaparecer
	};

	void spawnPoint();
	void tryFusion(const TokenTracker & tracker);
	float randomRange(float a, float b);

	std::vector<Point> points;
	float elapsed = 0;
	float nextSpawn = 0;
	int nextId = 1;
	bool finished = false;
	float scoreLow = 0, scoreHigh = 0;
	std::mt19937 rng;
};
