#include "Ensamblaje.h"

// Contornos de las figuras (radio unitario, en estrella desde el centro).
// Cambian en cada ciclo para que ningún ciclo sea idéntico al anterior.
static std::vector<glm::vec2> figureShape(int idx) {
	auto regular = [](int sides, float rot, std::vector<float> radii = {}) {
		std::vector<glm::vec2> v;
		for (int i = 0; i < sides; ++i) {
			float a = rot + i * TWO_PI / sides;
			float r = radii.empty() ? 1.0f : radii[i % radii.size()];
			v.push_back(glm::vec2(cos(a), sin(a)) * r);
		}
		return v;
	};
	switch (idx % 6) {
	case 0: return regular(3, -HALF_PI);
	case 1: return regular(4, PI * 0.25f);
	case 2: return regular(5, -HALF_PI, { 1.0f, 0.75f, 1.0f, 0.9f, 0.7f });
	case 3: return regular(6, 0);
	case 4: return regular(10, -HALF_PI, { 1.0f, 0.5f });
	default: return regular(8, PI / 8, { 1.0f, 0.65f, 0.95f, 0.8f });
	}
}

// Punto donde un rayo desde el origen con ángulo `a` corta el contorno.
static glm::vec2 rayHit(const std::vector<glm::vec2> & poly, float a) {
	glm::vec2 d(cos(a), sin(a));
	float best = 0;
	for (size_t i = 0; i < poly.size(); ++i) {
		glm::vec2 p = poly[i], q = poly[(i + 1) % poly.size()];
		glm::vec2 e = q - p;
		float den = d.x * e.y - d.y * e.x;
		if (fabs(den) < 1e-6) continue;
		float t = (p.x * e.y - p.y * e.x) / den; // distancia sobre el rayo
		float u = (p.x * d.y - p.y * d.x) / den; // posición sobre el lado
		if (t > 0 && u >= 0 && u <= 1) best = std::max(best, t);
	}
	return d * best;
}

void Ensamblaje::start() {
	cycle = 0;
	cycleTimes.clear();
	spentUnion.clear();
	center = glm::vec2(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
	event("game_start");
	startCycle();
}

void Ensamblaje::startCycle() {
	auto & s = settings().ensamblaje;
	int n = s.fragmentsPerCycle[cycle];
	float R = cm(s.figureRadiusCm);

	figure = figureShape(cycle);
	for (auto & p : figure)
		p *= R;

	// Cortes angulares: cada fragmento es una cuña desde el centro.
	float rot = ofRandom(TWO_PI);
	fragments.clear();
	for (int i = 0; i < n; ++i) {
		float a0 = rot + i * TWO_PI / n;
		float a1 = rot + (i + 1) * TWO_PI / n;
		std::vector<glm::vec2> wedge { glm::vec2(0), rayHit(figure, a0) };
		for (int step = 1; step < 24; ++step)
			wedge.push_back(rayHit(figure, ofLerp(a0, a1, step / 24.0f)));
		wedge.push_back(rayHit(figure, a1));

		glm::vec2 c(0);
		for (auto & p : wedge)
			c += p;
		c /= (float)wedge.size();

		Fragment f;
		for (auto & p : wedge)
			f.shape.push_back(p - c);
		f.slot = center + c;
		fragments.push_back(f);
	}

	// Posiciones iniciales repartidas alrededor del borde (territorio de almacenamiento).
	float margin = cm(s.edgeMarginCm);
	float rx = ofGetWidth() * 0.5f - margin;
	float ry = ofGetHeight() * 0.5f - margin;
	float a0 = ofRandom(TWO_PI);
	std::vector<int> order(n);
	std::iota(order.begin(), order.end(), 0);
	std::shuffle(order.begin(), order.end(), std::default_random_engine((unsigned)ofRandom(1e6)));
	for (int i = 0; i < n; ++i) {
		float a = a0 + i * TWO_PI / n;
		fragments[order[i]].pos = center + glm::vec2(cos(a) * rx, sin(a) * ry);
	}

	phase = PLAYING;
	phaseTime = 0;
	cycleTime = 0;
	freshCycle = true;
	event("cycle_start", "ciclo=" + ofToString(cycle + 1) + " fragmentos=" + ofToString(n));
}

bool Ensamblaje::groupInZone(const TokenTracker & tracker, const TokenTracker::Group & g) const {
	float zone = cm(settings().ensamblaje.zoneRadiusCm);
	for (int uid : g.uids)
		if (glm::distance(tracker.findToken(uid)->pos, center) > zone) return false;
	return true;
}

void Ensamblaje::update(float dt, const TokenTracker & tracker) {
	auto & s = settings().ensamblaje;
	phaseTime += dt;

	if (phase == PLAYING) {
		cycleTime += dt;

		float pickup = cm(s.pickupRadiusCm);
		for (auto & f : fragments) {
			for (auto & t : tracker.getTokens()) {
				bool near = glm::distance(f.pos, t.pos) < pickup;
				if (freshCycle && near) f.blocked.insert(t.uid);
				if (!near) f.blocked.erase(t.uid);
			}
		}
		freshCycle = false;

		// Fragmentos cuyo token se retiró: quedan donde estaban.
		for (auto & f : fragments)
			if (f.carrier >= 0 && !tracker.findToken(f.carrier)) {
				event("fragment_dropped", "token=" + ofToString(f.carrier));
				f.carrier = -1;
			}

		for (auto & t : tracker.getTokens()) {
			if (!t.visible) continue;
			Fragment * carried = nullptr;
			for (auto & f : fragments)
				if (f.carrier == t.uid) carried = &f;

			if (carried) {
				// El fragmento sigue al token, también dentro de la zona.
				carried->pos = t.pos;
			} else {
				// Recoger el fragmento libre más cercano dentro del radio.
				Fragment * nearest = nullptr;
				float best = pickup;
				for (auto & f : fragments) {
					if (f.fixed || f.carrier >= 0 || f.blocked.count(t.uid)) continue;
					float d = glm::distance(f.pos, t.pos);
					if (d < best) {
						best = d;
						nearest = &f;
					}
				}
				if (nearest) {
					nearest->carrier = t.uid;
					event("fragment_picked", "token=" + ofToString(t.uid));
				}
			}
		}

		// Una unión ya usada se libera cuando sus tokens se separan.
		for (auto it = spentUnion.begin(); it != spentUnion.end();) {
			int g = tracker.groupIndexOf(*it);
			if (g < 0 || !tracker.getGroups()[g].isJoined())
				it = spentUnion.erase(it);
			else
				++it;
		}

		// Un fragmento fijado por unión: grupo unido dentro de la zona que lleva un fragmento.
		for (auto & g : tracker.getGroups()) {
			if (g.size() < s.minJoinedTokens || !groupInZone(tracker, g)) continue;
			bool spent = false;
			for (int uid : g.uids)
				if (spentUnion.count(uid)) spent = true;
			if (spent) continue;

			Fragment * toFix = nullptr;
			for (auto & f : fragments)
				for (int uid : g.uids)
					if (!toFix && f.carrier == uid) toFix = &f;
			if (!toFix) continue;

			int carrier = toFix->carrier;
			toFix->fixed = true;
			toFix->carrier = -1;
			toFix->fixedAt = ofGetElapsedTimef();
			for (int uid : g.uids)
				spentUnion.insert(uid);
			int remaining = (int)std::count_if(fragments.begin(), fragments.end(), [](auto & f) { return !f.fixed; });
			event("fragment_fixed", "token=" + ofToString(carrier) + " restantes=" + ofToString(remaining));
		}

		// Los fragmentos fijados se deslizan a su lugar en la figura.
		for (auto & f : fragments)
			if (f.fixed) f.pos = glm::mix(f.pos, f.slot, 1.0f - exp(-dt * 8.0f));

		bool allFixed = std::all_of(fragments.begin(), fragments.end(), [](auto & f) { return f.fixed; });
		if (allFixed) {
			cycleTimes.push_back(cycleTime);
			event("figure_completed", "ciclo=" + ofToString(cycle + 1) + " segundos=" + ofToString(cycleTime, 1));
			phase = ASSEMBLING;
			phaseTime = 0;
		}
	} else if (phase == ASSEMBLING) {
		for (auto & f : fragments)
			f.pos = glm::mix(f.pos, f.slot, 1.0f - exp(-dt * 10.0f));
		if (phaseTime > s.assembleAnimSec) {
			phase = RESULT;
			phaseTime = 0;
		}
	} else if (phase == RESULT) {
		if (phaseTime > s.resultShowSec) {
			if (++cycle >= (int)s.fragmentsPerCycle.size()) {
				phase = FINISHED;
				event("game_end");
			} else {
				startCycle();
			}
		}
	}
}

void Ensamblaje::drawPolygon(const std::vector<glm::vec2> & pts, const glm::vec2 & at, bool filled) {
	if (filled)
		ofFill();
	else
		ofNoFill();
	ofBeginShape();
	for (auto & p : pts)
		ofVertex(at.x + p.x, at.y + p.y);
	ofEndShape(true);
}

void Ensamblaje::draw(const TokenTracker & tracker) {
	auto & s = settings().ensamblaje;
	ofPushStyle();

	// Zona central
	ofNoFill();
	ofSetLineWidth(2);
	ofSetColor(Ui::neutral, 90);
	ofDrawCircle(center, cm(s.zoneRadiusCm));

	// Silueta de la figura objetivo
	ofSetColor(Ui::neutral, 160);
	drawPolygon(figure, center, false);

	float t = ofGetElapsedTimef();
	for (auto & f : fragments) {
		if (phase == ASSEMBLING || phase == RESULT) {
			float flash = phase == ASSEMBLING ? 1.0f - phaseTime / s.assembleAnimSec : 0.0f;
			ofSetColor(Ui::accentJoined.getLerped(ofColor::white, flash * 0.7f));
			drawPolygon(f.shape, f.pos, true);
		} else if (f.carrier >= 0) {
			// En tránsito: brillo tenue pulsante
			float glow = 0.5f + 0.5f * sin(t * 5.0f);
			ofSetColor(Ui::accent, 60 + 50 * glow);
			ofSetLineWidth(8);
			drawPolygon(f.shape, f.pos, false);
			ofSetColor(Ui::accent, 220);
			drawPolygon(f.shape, f.pos, true);
		} else if (f.fixed) {
			// Fijado en la figura, con un destello breve al fijarse
			float k = ofClamp((t - f.fixedAt) / 0.6f, 0, 1);
			ofSetColor(Ui::accentJoined.getLerped(ofColor::white, (1 - k) * 0.7f));
			drawPolygon(f.shape, f.pos, true);
			if (k < 1) {
				ofSetLineWidth(4);
				ofSetColor(255, 255 * (1 - k));
				drawPolygon(f.shape, f.pos, false);
			}
		} else {
			ofSetColor(Ui::neutral, 200);
			drawPolygon(f.shape, f.pos, true);
		}
	}

	if (phase == ASSEMBLING) {
		// Destello expansivo al completar
		float k = phaseTime / s.assembleAnimSec;
		ofNoFill();
		ofSetLineWidth(4);
		ofSetColor(Ui::accentJoined, 255 * (1 - k));
		ofDrawCircle(center, cm(s.figureRadiusCm) * (1 + k * 1.5f));
	}

	if (phase == RESULT) {
		ofSetColor(255);
		int secs = (int)cycleTimes.back();
		Ui::text("Figura " + ofToString(cycle + 1) + " de " + ofToString(s.fragmentsPerCycle.size()), center.x,
			center.y + cm(s.zoneRadiusCm) + cm(2.0f), 1);
		Ui::text(ofToString(secs / 60) + ":" + ofToString(secs % 60, 2, '0'), center.x,
			center.y + cm(s.zoneRadiusCm) + cm(3.5f), 2);
	}

	ofPopStyle();
	drawTokens(tracker);
}

void Ensamblaje::drawResults() {
	float cx = ofGetWidth() * 0.5f;
	float y = ofGetHeight() * 0.3f;
	ofSetColor(255);
	Ui::text("Figuras completadas", cx, y, 2);
	float total = 0;
	for (size_t i = 0; i < cycleTimes.size(); ++i) {
		int secs = (int)cycleTimes[i];
		total += cycleTimes[i];
		Ui::text("Figura " + ofToString(i + 1) + "   " + ofToString(secs / 60) + ":" + ofToString(secs % 60, 2, '0'),
			cx, y + cm(2.5f) + i * cm(1.4f), 1);
	}
	int t = (int)total;
	Ui::text("Total " + ofToString(t / 60) + ":" + ofToString(t % 60, 2, '0'), cx,
		y + cm(3.5f) + cycleTimes.size() * cm(1.4f), 1);
}
