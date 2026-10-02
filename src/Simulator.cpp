#include "Simulator.h"
#include "Settings.h"

void Simulator::setup() {
	tokens[0] = { "A", glm::vec2(ofGetWidth() * 0.3f, ofGetHeight() * 0.82f), {}, 0, true };
	tokens[1] = { "B", glm::vec2(ofGetWidth() * 0.7f, ofGetHeight() * 0.82f), {}, 0, true };
	for (auto & t : tokens)
		t.target = t.pos;
	joined = false;
}

void Simulator::update(float dt) {
	if (joined) tokens[1].target = tokens[0].pos + joinedOffset();
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
	if (t.label == "A") {
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
		glm::vec2 r(p.x * cos(t.angle) - p.y * sin(t.angle), p.x * sin(t.angle) + p.y * cos(t.angle));
		out.emplace_back(t.pos.x + r.x, t.pos.y + r.y);
	}
	return out;
}

glm::vec2 Simulator::joinedOffset() const {
	// Centros a 4 cm: los contactos quedan dentro del umbral de agrupación,
	// igual que dos tokens físicos encajados.
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
		ofDrawCircle(t.pos, settings().cm(1.9f));
		ofSetColor(255, 255, 255, 160);
		ofDrawBitmapString("sim " + t.label, t.pos.x - 20, t.pos.y + settings().cm(2.6f));
	}
	ofSetColor(255, 255, 255, 200);
	ofDrawBitmapString("SIMULADOR  arrastrar: mover token | J: unir/separar | Z/X: poner/quitar A/B | Q/E: rotar | S: apagar",
		20, ofGetHeight() - 20);
	ofPopStyle();
}
