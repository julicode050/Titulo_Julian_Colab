#include "Marea.h"

float Marea::randomRange(float a, float b) {
	return std::uniform_real_distribution<float>(a, b)(rng);
}

void Marea::start() {
	rng.seed(settings().marea.seed);
	points.clear();
	elapsed = 0;
	nextSpawn = 0.5f;
	nextId = 1;
	finished = false;
	scoreLow = scoreHigh = 0;
	event("game_start");
}

void Marea::spawnPoint() {
	auto & s = settings().marea;
	float margin = cm(s.edgeMarginCm);
	glm::vec2 best;
	float bestDist = -1;
	// Busca una posición lejos de los puntos activos (o la más lejana encontrada).
	for (int attempt = 0; attempt < 50; ++attempt) {
		glm::vec2 p(randomRange(margin, ofGetWidth() - margin), randomRange(margin, ofGetHeight() - margin));
		float d = FLT_MAX;
		for (auto & o : points)
			if (o.alive) d = std::min(d, glm::distance(p, o.pos));
		if (d > bestDist) {
			bestDist = d;
			best = p;
		}
		if (d > cm(s.minSpawnDistanceCm)) break;
	}
	Point pt;
	pt.id = nextId++;
	pt.pos = best;
	pt.fill = s.startFill;
	points.push_back(pt);
	event("point_spawned", "punto=" + ofToString(pt.id));
}

void Marea::tryFusion(const TokenTracker & tracker) {
	auto & s = settings().marea;
	float hold = cm(s.holdRadiusCm);
	for (size_t i = 0; i < points.size(); ++i) {
		for (size_t j = i + 1; j < points.size(); ++j) {
			auto & a = points[i];
			auto & b = points[j];
			if (!a.alive || !b.alive || glm::distance(a.pos, b.pos) > cm(s.fusionDistanceCm)) continue;

			for (auto & g : tracker.getGroups()) {
				if (g.size() < s.fusionMinTokens) continue;
				bool onA = false, onB = false;
				for (int uid : g.uids) {
					auto * t = tracker.findToken(uid);
					if (glm::distance(t->pos, a.pos) < hold) onA = true;
					if (glm::distance(t->pos, b.pos) < hold) onB = true;
				}
				if (onA && onB) {
					event("points_fused", "puntos=" + ofToString(a.id) + "+" + ofToString(b.id));
					a.pos = (a.pos + b.pos) * 0.5f;
					a.fill = std::max(a.fill, b.fill);
					a.value += b.value;
					a.age = 0;
					b.alive = false;
					b.fade = 0;
					break;
				}
			}
		}
	}
}

void Marea::update(float dt, const TokenTracker & tracker) {
	if (finished) return;
	auto & s = settings().marea;
	elapsed += dt;
	float hold = cm(s.holdRadiusCm);

	// Aparición escalonada
	nextSpawn -= dt;
	int alive = (int)std::count_if(points.begin(), points.end(), [](auto & p) { return p.alive; });
	if (nextSpawn <= 0 && alive < s.maxActivePoints) {
		spawnPoint();
		nextSpawn = randomRange(s.spawnMinSec, s.spawnMaxSec);
	}

	tryFusion(tracker);

	for (auto & p : points) {
		if (!p.alive) {
			p.fade = std::max(0.0f, p.fade - dt * 2.0f);
			continue;
		}

		// Modo: alto si un grupo unido está sobre el punto, bajo si hay al menos un token.
		Mode mode = NONE;
		for (auto & g : tracker.getGroups()) {
			if (g.isJoined() && glm::distance(g.center, p.pos) < hold) mode = HIGH;
		}
		if (mode == NONE) {
			for (auto & t : tracker.getTokens())
				if (t.visible && glm::distance(t.pos, p.pos) < hold) mode = LOW;
		}
		if (mode != p.mode) {
			const char * names[] = { "ninguno", "bajo", "alto" };
			event("point_mode", "punto=" + ofToString(p.id) + " modo=" + names[mode]);
			p.mode = mode;
		}

		float gain = mode == HIGH ? s.highFillPerSec : mode == LOW ? s.lowFillPerSec : 0.0f;
		p.fill = ofClamp(p.fill + (gain - s.decayPerSec) * dt, 0, 1);
		if (mode == LOW) scoreLow += s.lowScorePerSec * p.value * dt;
		if (mode == HIGH) scoreHigh += s.highScorePerSec * p.value * dt;

		p.age += dt;
		if (p.fill <= 0) {
			p.alive = false;
			event("point_emptied", "punto=" + ofToString(p.id));
		} else if (p.age >= s.pointLifetimeSec) {
			p.alive = false;
			event("point_expired", "punto=" + ofToString(p.id));
		}
	}
	points.erase(std::remove_if(points.begin(), points.end(), [](auto & p) { return !p.alive && p.fade <= 0; }), points.end());

	if (elapsed >= s.sessionSec) {
		finished = true;
		event("game_end", "puntajeBajo=" + ofToString(std::lround(scoreLow)) + " puntajeAlto=" + ofToString(std::lround(scoreHigh)));
	}
}

void Marea::draw(const TokenTracker & tracker) {
	if (finished) return;
	auto & s = settings().marea;
	float r = cm(s.pointRadiusCm);
	float t = ofGetElapsedTimef();

	Ui::timerBar(1.0f - elapsed / s.sessionSec, Ui::neutral);

	ofPushStyle();
	for (auto & p : points) {
		float alpha = p.alive ? 1.0f : p.fade;
		// Desvanecer al final de su vida
		alpha *= ofClamp((s.pointLifetimeSec - p.age) / 2.0f, 0, 1);
		float size = r * (1.0f + 0.25f * (p.value - 1));
		ofColor c = p.mode == HIGH ? Ui::accentJoined : Ui::accent;

		// Pulso suave: lento en modo bajo, rápido y más amplio en modo alto
		if (p.mode != NONE) {
			float speed = p.mode == HIGH ? 6.0f : 2.0f;
			float amp = p.mode == HIGH ? 0.35f : 0.15f;
			float k = 0.5f + 0.5f * sin(t * speed);
			ofNoFill();
			ofSetLineWidth(2);
			ofSetColor(c, 120 * alpha * (1 - k));
			ofDrawCircle(p.pos, size * (1.15f + amp * k));
		}

		// Nivel de llenado: disco interior + arco
		ofFill();
		ofSetColor(c, 70 * alpha);
		ofDrawCircle(p.pos, size * p.fill);
		ofSetColor(Ui::neutral, 80 * alpha);
		Ui::ring(p.pos, size, 1.0f, 4);
		ofSetColor(p.mode == NONE ? Ui::neutral : c, 255 * alpha);
		Ui::ring(p.pos, size, p.fill, p.mode == HIGH ? 10 : 5);
	}
	ofPopStyle();

	ofSetColor(255, 200);
	Ui::text(ofToString(std::lround(scoreLow) + std::lround(scoreHigh)), ofGetWidth() * 0.5f, cm(1.2f), 1);
	drawTokens(tracker);
}

void Marea::drawResults() {
	float cx = ofGetWidth() * 0.5f;
	float y = ofGetHeight() * 0.35f;
	ofSetColor(255);
	// Redondeo único para que el total coincida con la suma visible
	long low = std::lround(scoreLow), high = std::lround(scoreHigh);
	Ui::text("Puntaje total   " + ofToString(low + high), cx, y, 2);
	Ui::text("Modo individual   " + ofToString(low), cx, y + cm(3), 1);
	Ui::text("Modo unido   " + ofToString(high), cx, y + cm(4.5f), 1);
}

std::vector<std::string> Marea::instructions() const {
	auto & s = settings().marea;
	return {
		"OBJETIVO: sumar puntaje sosteniendo puntos de presión.",
		"",
		"- Los puntos aparecen de a poco (máx. " + ofToString(s.maxActivePoints) + " a la vez) y se vacían solos.",
		"- 1 token sobre un punto: modo bajo, llena lento (" + num(s.lowScorePerSec) + " pt/s).",
		"- 2 tokens UNIDOS sobre un punto: modo alto, llena rápido (" + num(s.highScorePerSec) + " pts/s).",
		"- Un punto desaparece al vaciarse o a los " + num(s.pointLifetimeSec) + " s.",
		"",
		"GANAR: puntaje total al terminar (" + num(s.sessionSec / 60.0f) + " min),",
		"separado en modo individual y modo unido.",
	};
}
