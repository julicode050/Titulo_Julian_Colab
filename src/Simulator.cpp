#include "Simulator.h"
#include "Settings.h"

// Tokens nuevos (docs/TOKENS.md, v1): centros de las almohadillas en mm, con el par encajado,
// origen en el punto de abajo de A y eje Y hacia arriba.
static const glm::vec2 kPadsA[3] = { { 0.00f, 0.00f }, { 0.00f, 30.00f }, { 25.98f, 15.00f } };
static const glm::vec2 kPadsB[3] = { { -11.51f, 25.68f }, { 10.50f, 33.18f }, { 23.49f, 25.68f } };

static glm::vec2 centroid(const glm::vec2 (&p)[3]) {
	return (p[0] + p[1] + p[2]) / 3.0f;
}
// mm (Y hacia arriba) relativo al centro del token -> px de pantalla (Y hacia abajo)
static glm::vec2 mmToScreen(const glm::vec2 & mm) {
	float pxPerMm = settings().pxPerCm / 10.0f;
	return glm::vec2(mm.x, -mm.y) * pxPerMm;
}
static glm::vec2 rotate(const glm::vec2 & p, float a) {
	return glm::vec2(p.x * cos(a) - p.y * sin(a), p.x * sin(a) + p.y * cos(a));
}

void Simulator::setup() {
	tokens[0] = { "A", glm::vec2(ofGetWidth() * 0.3f, ofGetHeight() * 0.82f), {}, 0, true };
	tokens[1] = { "B", glm::vec2(ofGetWidth() * 0.7f, ofGetHeight() * 0.82f), {}, 0, true };
	for (auto & t : tokens)
		t.target = t.pos;
	joined = false;
}

void Simulator::update(float dt) {
	if (joined) {
		tokens[1].target = tokens[0].pos + joinedOffset();
		if (newTokens) tokens[1].angle = tokens[0].angle; // encajados giran juntos
	}
	float step = settings().cm(50.0f) * dt; // 50 cm/s
	for (auto & t : tokens) {
		glm::vec2 d = t.target - t.pos;
		float len = glm::length(d);
		t.pos = len <= step ? t.target : t.pos + d / len * step;
	}
}

void Simulator::moveTo(int i, const glm::vec2 & target) {
	tokens[i].target = target;
}

bool Simulator::arrived(int i) const {
	return glm::distance(tokens[i].pos, tokens[i].target) < 1.0f;
}

std::vector<ofVec2f> Simulator::shapePoints(const SimToken & t) const {
	float s = settings().cm(settings().simTokenSideCm);
	std::vector<glm::vec2> local;
	if (newTokens) {
		auto & pads = t.label == "A" ? kPadsA : kPadsB;
		glm::vec2 c = centroid(pads);
		for (auto & p : pads)
			local.push_back(mmToScreen(p - c));
	} else if (t.label == "A") {
		// Equilátero -> classifyShape devuelve 'A'
		float r = s / sqrt(3.0f);
		for (int k = 0; k < 3; ++k) {
			float a = -HALF_PI + k * TWO_PI / 3.0f;
			local.push_back(glm::vec2(cos(a), sin(a)) * r);
		}
	} else {
		// Isósceles con ápice de 40° -> 'B' o 'C' según la orientación (ambas mapean a "B")
		float leg = s * 1.1f;
		float half = ofDegToRad(20.0f);
		local = { glm::vec2(0, 0), glm::vec2(-sin(half), cos(half)) * leg, glm::vec2(sin(half), cos(half)) * leg };
		glm::vec2 c = (local[0] + local[1] + local[2]) / 3.0f;
		for (auto & p : local)
			p -= c;
	}

	std::vector<ofVec2f> out;
	for (auto & p : local) {
		glm::vec2 r = rotate(p, t.angle);
		out.emplace_back(t.pos.x + r.x, t.pos.y + r.y);
	}
	return out;
}

glm::vec2 Simulator::joinedOffset() const {
	// Tokens nuevos: B queda en su posición exacta de encaje respecto de A (patrones intercalados).
	if (newTokens) return rotate(mmToScreen(centroid(kPadsB) - centroid(kPadsA)), tokens[0].angle);
	// Tokens actuales: centros a 4 cm, los contactos quedan dentro del umbral de agrupación,
	// igual que dos tokens físicos juntos.
	return glm::vec2(settings().cm(4.0f), 0);
}

void Simulator::applyJoin() {
	if (joined) tokens[1].pos = tokens[1].target = tokens[0].pos + joinedOffset();
}

std::vector<ofVec2f> Simulator::getPoints() const {
	std::vector<ofVec2f> pts;
	if (!active) return pts;
	for (auto & t : tokens) {
		if (!t.present) continue;
		auto p = shapePoints(t);
		pts.insert(pts.end(), p.begin(), p.end());
	}
	return pts;
}

bool Simulator::mousePressed(int x, int y) {
	if (!active) return false;
	glm::vec2 m(x, y);
	float grab = settings().cm(2.5f);
	for (int i = 0; i < 2; ++i) {
		if (tokens[i].present && glm::distance(m, tokens[i].pos) < grab) {
			// Unidos se mueven como uno: siempre se arrastra A
			dragging = joined ? 0 : i;
			lastTouched = i;
			dragOffset = tokens[dragging].pos - m;
			return true;
		}
	}
	return false;
}

bool Simulator::mouseDragged(int x, int y) {
	if (!active || dragging < 0) return false;
	tokens[dragging].pos = tokens[dragging].target = glm::vec2(x, y) + dragOffset;
	applyJoin();
	return true;
}

void Simulator::mouseReleased() {
	dragging = -1;
}

bool Simulator::keyPressed(int key) {
	if (!active) return false;
	switch (key) {
	case 'j':
	case 'J':
		joined = !joined;
		if (!joined) tokens[1].target = tokens[0].pos + glm::vec2(settings().cm(10.0f), 0);
		return true;
	case 't':
	case 'T':
		newTokens = !newTokens;
		return true;
	case 'z':
	case 'Z':
		tokens[0].present = !tokens[0].present;
		return true;
	case 'x':
	case 'X':
		tokens[1].present = !tokens[1].present;
		return true;
	case 'q':
	case 'Q':
		tokens[lastTouched].angle -= ofDegToRad(15);
		return true;
	case 'e':
	case 'E':
		tokens[lastTouched].angle += ofDegToRad(15);
		return true;
	}
	return false;
}

void Simulator::draw() const {
	if (!active) return;
	ofPushStyle();
	ofNoFill();
	ofSetLineWidth(1);
	for (auto & t : tokens) {
		if (!t.present) continue;
		ofSetColor(255, 255, 255, 90);
		float r = settings().cm(newTokens ? 2.3f : 1.9f);
		ofDrawCircle(t.pos, r);
		ofSetColor(255, 255, 255, 160);
		ofDrawBitmapString("sim " + t.label, t.pos.x - 20, t.pos.y + r + settings().cm(0.7f));
	}
	ofSetColor(255, 255, 255, 200);
	ofDrawBitmapString(std::string("SIMULADOR (") + (newTokens ? "tokens nuevos v1" : "tokens actuales")
			+ ")  arrastrar: mover | J: unir/separar | T: tokens nuevos/actuales | Z/X: poner/quitar A/B | Q/E: rotar | S: apagar",
		20, ofGetHeight() - 20);
	ofPopStyle();
}
