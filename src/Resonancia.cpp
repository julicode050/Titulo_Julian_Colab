#include "Resonancia.h"

void Resonancia::start() {
	auto & s = settings().resonancia;
	// Semilla fija: todas las sesiones ven la misma secuencia de distancias y nodos,
	// lo que permite comparar grupos de forma cualitativa.
	rng.seed(s.seed);

	// Secuencia balanceada: cada distancia aparece las mismas veces (±1), en orden barajado.
	spacingSequence.clear();
	countSequence.clear();
	for (int c = 0; c < s.cycles; ++c) {
		spacingSequence.push_back(s.spacingsCm[c % s.spacingsCm.size()]);
		std::uniform_int_distribution<int> count(s.minIndividualNodes, s.maxIndividualNodes);
		countSequence.push_back(count(rng));
	}
	std::shuffle(spacingSequence.begin(), spacingSequence.end(), rng);

	cycle = 0;
	individualDone = individualTotal = doubleDone = doubleTotal = 0;
	event("game_start");
	startPhase(INDIVIDUAL);
}

void Resonancia::layoutIndividualNodes(int count, float spacing) {
	auto & s = settings().resonancia;
	float margin = cm(s.edgeMarginCm);
	std::uniform_real_distribution<float> angle(0, TWO_PI);
	std::uniform_real_distribution<float> ux(margin, ofGetWidth() - margin);
	std::uniform_real_distribution<float> uy(margin, ofGetHeight() - margin);

	// Polígono regular de lado `spacing` (2 nodos: segmento; 3 nodos: triángulo).
	float circumradius = count == 2 ? spacing * 0.5f : spacing / (2.0f * sin(PI / count));

	for (float scale = 1.0f; scale > 0.3f; scale -= 0.1f) {
		for (int attempt = 0; attempt < 200; ++attempt) {
			glm::vec2 c(ux(rng), uy(rng));
			float rot = angle(rng);
			std::vector<glm::vec2> pts;
			bool fits = true;
			for (int i = 0; i < count; ++i) {
				float a = rot + i * TWO_PI / count;
				glm::vec2 p = c + glm::vec2(cos(a), sin(a)) * circumradius * scale;
				if (p.x < margin || p.x > ofGetWidth() - margin || p.y < margin || p.y > ofGetHeight() - margin) fits = false;
				pts.push_back(p);
			}
			if (!fits) continue;
			for (auto & p : pts) {
				Node n;
				n.pos = p;
				nodes.push_back(n);
			}
			if (scale < 1.0f) ofLogWarning("Resonancia") << "Distancia reducida al " << scale * 100 << "% para caber en pantalla";
			return;
		}
	}
}

void Resonancia::startPhase(Phase p) {
	phase = p;
	phaseTime = 0;
	nodes.clear();

	if (p == INDIVIDUAL) {
		int count = countSequence[cycle];
		float spacing = spacingSequence[cycle];
		layoutIndividualNodes(count, cm(spacing));
		individualTotal += (int)nodes.size();
		event("phase_start", "ciclo=" + ofToString(cycle + 1) + " fase=individual nodos=" + ofToString(nodes.size())
				+ " distanciaCm=" + ofToString(spacing));
	} else if (p == DOUBLE) {
		Node n;
		n.pos = glm::vec2(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
		n.isDouble = true;
		nodes.push_back(n);
		doubleTotal += 1;
		event("phase_start", "ciclo=" + ofToString(cycle + 1) + " fase=doble");
	} else {
		event("game_end", "individuales=" + ofToString(individualDone) + "/" + ofToString(individualTotal)
				+ " dobles=" + ofToString(doubleDone) + "/" + ofToString(doubleTotal));
	}
}

void Resonancia::update(float dt, const TokenTracker & tracker) {
	if (phase == FINISHED) return;
	auto & s = settings().resonancia;
	phaseTime += dt;
	float radius = cm(s.activationRadiusCm);

	for (auto & n : nodes) {
		if (n.completed) continue;

		bool active = false;
		if (n.isDouble) {
			// Requiere un grupo de tokens unidos sobre el nodo
			for (auto & g : tracker.getGroups())
				if (g.isJoined() && glm::distance(g.center, n.pos) < radius) active = true;
		} else {
			// Cualquier token sirve, solo o unido
			for (auto & t : tracker.getTokens())
				if (t.visible && glm::distance(t.pos, n.pos) < radius) active = true;
		}

		float fillSec = n.isDouble ? s.doubleFillSec : s.fillSec;
		n.fill += active ? dt / fillSec : -dt * s.drainPerSec;
		n.fill = ofClamp(n.fill, 0, 1);

		if (n.fill >= 1) {
			n.completed = true;
			n.completedAt = ofGetElapsedTimef();
			if (n.isDouble)
				doubleDone++;
			else
				individualDone++;
			event("node_completed", std::string("tipo=") + (n.isDouble ? "doble" : "individual") + " ciclo=" + ofToString(cycle + 1));
		}
	}

	float phaseLen = phase == INDIVIDUAL ? s.individualPhaseSec : s.doublePhaseSec;
	if (phaseTime >= phaseLen) {
		if (phase == INDIVIDUAL) {
			startPhase(DOUBLE);
		} else if (++cycle >= s.cycles) {
			startPhase(FINISHED);
		} else {
			startPhase(INDIVIDUAL);
		}
	}
}

void Resonancia::draw(const TokenTracker & tracker) {
	if (phase == FINISHED) return;
	auto & s = settings().resonancia;
	float r = cm(s.nodeRadiusCm);
	float now = ofGetElapsedTimef();

	float phaseLen = phase == INDIVIDUAL ? s.individualPhaseSec : s.doublePhaseSec;
	Ui::timerBar(1.0f - phaseTime / phaseLen, phase == INDIVIDUAL ? Ui::accent : Ui::accentJoined);

	ofPushStyle();
	for (auto & n : nodes) {
		ofColor c = n.isDouble ? Ui::accentJoined : Ui::accent;

		// Relleno progresivo
		ofFill();
		ofSetColor(c, n.completed ? 230 : 60 + 140 * n.fill);
		ofDrawCircle(n.pos, r * (n.completed ? 1.0f : n.fill));

		// Anillo(s)
		ofNoFill();
		ofSetLineWidth(3);
		ofSetColor(n.fill > 0 || n.completed ? c : Ui::neutral);
		ofDrawCircle(n.pos, r);
		if (n.isDouble) ofDrawCircle(n.pos, r * 1.35f);

		// Destello breve al completarse
		if (n.completed) {
			float k = (now - n.completedAt) / 0.6f;
			if (k < 1) {
				ofSetLineWidth(5);
				ofSetColor(255, 255 * (1 - k));
				ofDrawCircle(n.pos, r * (1.0f + k * 1.2f));
			}
		}
	}
	ofPopStyle();
	drawTokens(tracker);
}

void Resonancia::drawResults() {
	float cx = ofGetWidth() * 0.5f;
	float y = ofGetHeight() * 0.35f;
	ofSetColor(255);
	Ui::text("Nodos activados", cx, y, 2);
	Ui::text("Individuales   " + ofToString(individualDone) + " de " + ofToString(individualTotal), cx, y + cm(3), 1);
	Ui::text("Dobles   " + ofToString(doubleDone) + " de " + ofToString(doubleTotal), cx, y + cm(4.5f), 1);
}

std::vector<std::string> Resonancia::instructions() const {
	auto & s = settings().resonancia;
	float minutes = s.cycles * (s.individualPhaseSec + s.doublePhaseSec) / 60.0f;
	return {
		"OBJETIVO: activar la mayor cantidad de nodos.",
		"",
		"- Fase individual (barra naranja, " + num(s.individualPhaseSec) + " s): "
			+ ofToString(s.minIndividualNodes) + "-" + ofToString(s.maxIndividualNodes) + " nodos de anillo simple.",
		"  Se activan con 1 token encima durante " + num(s.fillSec) + " s.",
		"- Fase doble (barra turquesa, " + num(s.doublePhaseSec) + " s): 1 nodo de anillo doble al centro.",
		"  Se activa con los 2 tokens UNIDOS encima durante " + num(s.doubleFillSec) + " s.",
		"- El nodo se llena mientras hay un token encima y se vacía",
		"  de a poco si se retira.",
		"- La distancia entre nodos individuales cambia en cada ciclo.",
		"",
		"GANAR: activar nodos; se cuentan individuales y dobles.",
		ofToString(s.cycles) + " ciclos, unos " + num(std::round(minutes * 10) / 10) + " min.",
	};
}
