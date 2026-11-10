#include "Resonancia.h"

void Resonancia::start() {
	auto & s = settings().resonancia;
	// Semilla fija: todas las sesiones ven la misma secuencia de distancias y nodos,
	// lo que permite comparar parejas de forma cualitativa.
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
	finished = false;
	individualDone = individualTotal = doubleDone = doubleTotal = 0;
	event("game_start");
	startCycle();
}

// Ubica `count` nodos como polígono regular de lado `spacing`, dentro de la pantalla y lejos
// del nodo doble del centro. Devuelve false si no caben a esa distancia exacta.
bool Resonancia::layoutIndividualNodes(int count, float spacing) {
	auto & s = settings().resonancia;
	float margin = cm(s.edgeMarginCm);
	float clear = cm(s.centerClearCm);
	glm::vec2 screenCenter(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
	std::uniform_real_distribution<float> angle(0, TWO_PI);
	std::uniform_real_distribution<float> ux(margin, ofGetWidth() - margin);
	std::uniform_real_distribution<float> uy(margin, ofGetHeight() - margin);
	float circumradius = count == 2 ? spacing * 0.5f : spacing / (2.0f * sin(PI / count));

	for (int attempt = 0; attempt < 2000; ++attempt) {
		glm::vec2 c(ux(rng), uy(rng));
		float rot = angle(rng);
		std::vector<glm::vec2> pts;
		bool fits = true;
		for (int i = 0; i < count && fits; ++i) {
			float a = rot + i * TWO_PI / count;
			glm::vec2 p = c + glm::vec2(cos(a), sin(a)) * circumradius;
			if (p.x < margin || p.x > ofGetWidth() - margin || p.y < margin || p.y > ofGetHeight() - margin) fits = false;
			if (glm::distance(p, screenCenter) < clear) fits = false;
			pts.push_back(p);
		}
		if (!fits) continue;
		for (auto & p : pts) {
			Node n;
			n.pos = p;
			nodes.push_back(n);
		}
		return true;
	}
	return false;
}

void Resonancia::startCycle() {
	cycleTime = 0;
	nodes.clear();

	// Nodos individuales: si no caben a la distancia del ciclo, se usan menos nodos;
	// la distancia nunca se achica (es la variable de interés).
	float spacing = spacingSequence[cycle];
	int count = countSequence[cycle];
	while (count > 2 && !layoutIndividualNodes(count, cm(spacing)))
		count--;
	if (nodes.empty()) {
		layoutIndividualNodes(2, cm(spacing));
		ofLogWarning("Resonancia") << "No caben nodos a " << spacing << " cm";
	}
	individualTotal += (int)nodes.size();

	// Nodo doble opcional, al centro
	Node d;
	d.pos = glm::vec2(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
	d.isDouble = true;
	nodes.push_back(d);
	doubleTotal += 1;

	// "phase_start": el registro lo anota como "Cambio de fase" (salvo el primero, que coincide
	// con el inicio del juego).
	event("phase_start", "ciclo=" + ofToString(cycle + 1) + " nodos=" + ofToString((int)nodes.size() - 1)
			+ " distanciaCm=" + ofToString(spacing));
}

float Resonancia::points() const {
	auto & s = settings().resonancia;
	return individualDone * s.individualValue + doubleDone * s.doubleValue;
}

void Resonancia::update(float dt, const TokenTracker & tracker) {
	if (finished) return;
	auto & s = settings().resonancia;
	cycleTime += dt;
	float radius = cm(s.activationRadiusCm);

	for (auto & n : nodes) {
		if (n.completed) continue;

		bool active = false;
		if (n.isDouble) {
			// Requiere un grupo de tokens unidos sobre el nodo
			for (auto & g : tracker.getGroups())
				if (g.isJoined() && glm::distance(g.center, n.pos) < radius) active = true, n.actor = "Par";
		} else {
			// Cualquier token sirve, solo o unido
			for (auto & t : tracker.getTokens()) {
				// Un token que desapareció hace poco (tolerancia) sigue contando en su última posición.
				if (glm::distance(t.pos, n.pos) >= radius) continue;
				int g = tracker.groupIndexOf(t.uid);
				bool joined = g >= 0 && tracker.getGroups()[g].isJoined();
				if (!active) n.actor = joined ? "Par" : t.label;
				active = true;
			}
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
			event("node_completed", std::string("tipo=") + (n.isDouble ? "doble" : "individual") + " ciclo=" + ofToString(cycle + 1), n.actor);
		}
	}

	if (cycleTime >= s.cycleSec) {
		if (++cycle >= s.cycles) {
			finished = true;
			nodes.clear();
			event("game_end", "individuales=" + ofToString(individualDone) + "/" + ofToString(individualTotal)
					+ " dobles=" + ofToString(doubleDone) + "/" + ofToString(doubleTotal) + " puntos=" + ofToString(points()));
		} else {
			startCycle();
		}
	}
}

void Resonancia::draw(const TokenTracker & tracker) {
	if (finished) return;
	auto & s = settings().resonancia;
	float r = cm(s.nodeRadiusCm);
	float now = ofGetElapsedTimef();

	Ui::timerBar(1.0f - cycleTime / s.cycleSec, Ui::accent);
	ofSetColor(255, 200);
	Ui::text(num(points()), ofGetWidth() * 0.5f, cm(1.2f), 1);

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
	Ui::text("Puntaje   " + num(points()), cx, y, 2);
	Ui::text("Nodos individuales   " + ofToString(individualDone) + " de " + ofToString(individualTotal), cx, y + cm(3), 1);
	Ui::text("Nodos dobles   " + ofToString(doubleDone) + " de " + ofToString(doubleTotal), cx, y + cm(4.5f), 1);
}

std::vector<std::string> Resonancia::instructions() const {
	auto & s = settings().resonancia;
	float minutes = s.cycles * s.cycleSec / 60.0f;
	return {
		"OBJETIVO: sumar puntos activando nodos.",
		"",
		"- Cada ciclo dura " + num(s.cycleSec) + " s (barra de arriba) y trae nodos nuevos.",
		"- Nodos de anillo simple: se activan con 1 token encima durante " + num(s.fillSec) + " s.",
		"  Valen " + num(s.individualValue) + " pt.",
		"- Nodo de anillo doble (centro, opcional): se activa con los 2 tokens UNIDOS",
		"  encima durante " + num(s.doubleFillSec) + " s. Vale " + num(s.doubleValue) + " pts.",
		"- No alcanza el tiempo para hacerlo todo juntos: decidan cuándo repartirse.",
		"- La distancia entre los nodos simples cambia en cada ciclo.",
		"",
		"GANAR: puntaje al terminar (" + ofToString(s.cycles) + " ciclos, " + num(std::round(minutes * 10) / 10) + " min).",
	};
}
