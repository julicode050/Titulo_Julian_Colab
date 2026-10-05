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

	// Cortes angulares irregulares: cada fragmento es una cuña desde el centro con un ángulo
	// distinto (entre 0,6 y 1,4 veces el reparto parejo), para que cada pieza tenga forma propia.
	std::vector<float> spans(n);
	float total = 0;
	for (auto & w : spans)
		total += (w = ofRandom(0.6f, 1.4f));
	float rot = ofRandom(TWO_PI);
	fragments.clear();
	float a0 = rot;
	for (int i = 0; i < n; ++i) {
		float a1 = a0 + spans[i] / total * TWO_PI;
		// Cuña: centro, corte inicial, esquinas de la figura dentro del ángulo, corte final.
		std::vector<glm::vec2> wedge { glm::vec2(0), rayHit(figure, a0) };
		std::vector<std::pair<float, glm::vec2>> corners;
		for (auto & v : figure) {
			float rel = fmod(atan2(v.y, v.x) - a0 + 4 * TWO_PI, TWO_PI);
			if (rel > 1e-4f && rel < a1 - a0 - 1e-4f) corners.push_back({ rel, v });
		}
		std::sort(corners.begin(), corners.end(), [](auto & x, auto & y) { return x.first < y.first; });
		for (auto & c : corners)
			wedge.push_back(c.second);
		wedge.push_back(rayHit(figure, a1));

		// Centroide de área: el punto donde el token debe ubicar la pieza.
		glm::vec2 c(0);
		float area = 0;
		for (size_t k = 0; k < wedge.size(); ++k) {
			glm::vec2 p = wedge[k], q = wedge[(k + 1) % wedge.size()];
			float cross = p.x * q.y - q.x * p.y;
			area += cross;
			c += (p + q) * cross;
		}
		c /= (3.0f * area);

		Fragment f;
		f.index = i;
		for (auto & p : wedge)
			f.shape.push_back(p - c);
		f.slot = center + c;
		fragments.push_back(f);
		a0 = a1;
	}

	// Posiciones iniciales en columnas a ambos lados de la zona (territorio de almacenamiento),
	// en orden barajado para que la posición no delate el lugar de cada pieza.
	float margin = cm(s.edgeMarginCm);
	float zone = cm(s.zoneRadiusCm);
	float sideX = (zone + ofGetWidth() * 0.5f - margin) * 0.5f; // centro de la franja lateral
	std::vector<int> order(n);
	std::iota(order.begin(), order.end(), 0);
	std::shuffle(order.begin(), order.end(), std::default_random_engine((unsigned)ofRandom(1e6)));
	int leftCount = (n + (ofRandom(1) < 0.5f ? 1 : 0)) / 2;
	for (int i = 0; i < n; ++i) {
		bool left = i < leftCount;
		int k = left ? i : i - leftCount;
		int count = left ? leftCount : n - leftCount;
		float y = margin + (k + 0.5f) * (ofGetHeight() - 2 * margin) / count;
		float x = center.x + (left ? -sideX : sideX) + ofRandom(-cm(1.0f), cm(1.0f));
		fragments[order[i]].pos = glm::vec2(x, y);
	}

	phase = PLAYING;
	phaseTime = 0;
	cycleTime = 0;
	freshCycle = true;
	event("cycle_start", "ciclo=" + ofToString(cycle + 1) + " fragmentos=" + ofToString(n));
}

// El token toca el fragmento si está dentro de su forma o a menos del radio de recogida.
bool Ensamblaje::touches(const Fragment & f, const glm::vec2 & p) const {
	if (glm::distance(f.pos, p) < cm(settings().ensamblaje.pickupRadiusCm)) return true;
	bool inside = false;
	for (size_t i = 0, j = f.shape.size() - 1; i < f.shape.size(); j = i++) {
		glm::vec2 a = f.pos + f.shape[i], b = f.pos + f.shape[j];
		if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) inside = !inside;
	}
	return inside;
}

int Ensamblaje::nearestSlot(const glm::vec2 & p) const {
	int best = -1;
	float bestDist = FLT_MAX;
	for (auto & f : fragments) {
		float d = glm::distance(p, f.slot);
		if (d < bestDist) {
			bestDist = d;
			best = f.index;
		}
	}
	return best;
}

void Ensamblaje::update(float dt, const TokenTracker & tracker) {
	auto & s = settings().ensamblaje;
	phaseTime += dt;

	if (phase == PLAYING) {
		cycleTime += dt;

		for (auto & f : fragments) {
			for (auto & t : tracker.getTokens()) {
				bool near = touches(f, t.pos);
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
				// Recoger el fragmento libre más cercano que el token esté tocando.
				Fragment * nearest = nullptr;
				float best = FLT_MAX;
				for (auto & f : fragments) {
					if (f.fixed || f.carrier >= 0 || f.blocked.count(t.uid) || !touches(f, t.pos)) continue;
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

		// Un fragmento fijado por unión, solo en su propio lugar de la figura.
		float matchDist = cm(s.matchMaxDistCm);
		float now = ofGetElapsedTimef();
		std::set<int> wrongNow; // fragmentos que este frame están en un lugar incorrecto
		float zone = cm(s.zoneRadiusCm);
		for (auto & g : tracker.getGroups()) {
			// Basta con que el token que lleva la pieza esté en la zona; su compañero puede
			// unirse desde afuera (los lugares del borde de la figura quedan cerca del límite).
			if (g.size() < s.minJoinedTokens) continue;
			bool spent = false;
			for (int uid : g.uids)
				if (spentUnion.count(uid)) spent = true;
			if (spent) continue;

			Fragment * toFix = nullptr;
			for (auto & f : fragments) {
				if (f.carrier < 0 || std::find(g.uids.begin(), g.uids.end(), f.carrier) == g.uids.end()) continue;
				glm::vec2 at = tracker.findToken(f.carrier)->pos;
				if (glm::distance(at, center) > zone) continue;
				int slot = nearestSlot(at);
				if (slot == f.index && glm::distance(at, f.slot) < matchDist) {
					if (!toFix) toFix = &f;
				} else {
					// Lugar incorrecto: el fragmento tiembla; la unión no se gasta.
					wrongNow.insert(f.index);
					if (f.rejectSlot != slot) {
						f.rejectSlot = slot;
						f.rejectAt = now;
						event("fragment_rejected", "token=" + ofToString(f.carrier) + " pieza=" + ofToString(f.index) + " lugar=" + ofToString(slot));
					}
				}
			}
			if (!toFix) continue;

			int carrier = toFix->carrier;
			toFix->fixed = true;
			toFix->carrier = -1;
			toFix->fixedAt = ofGetElapsedTimef();
			for (int uid : g.uids)
				spentUnion.insert(uid);
			wrongNow.erase(toFix->index);
			int remaining = (int)std::count_if(fragments.begin(), fragments.end(), [](auto & f) { return !f.fixed; });
			event("fragment_fixed", "token=" + ofToString(carrier) + " restantes=" + ofToString(remaining));
		}

		for (auto & f : fragments)
			if (!wrongNow.count(f.index)) f.rejectSlot = -1;

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

	// Contornos de cada lugar: visibles en los primeros ciclos, se desvanecen después.
	auto & alphas = s.slotOutlineAlpha;
	float outline = alphas.empty() ? 0.0f : alphas[std::min(cycle, (int)alphas.size() - 1)];
	if (outline > 0 && phase == PLAYING) {
		ofSetLineWidth(1.5f);
		ofSetColor(Ui::neutral, 130 * outline);
		for (auto & f : fragments)
			if (!f.fixed) drawPolygon(f.shape, f.slot, false);
	}

	float t = ofGetElapsedTimef();
	for (auto & f : fragments) {
		if (phase == ASSEMBLING || phase == RESULT) {
			float flash = phase == ASSEMBLING ? 1.0f - phaseTime / s.assembleAnimSec : 0.0f;
			ofSetColor(Ui::accentJoined.getLerped(ofColor::white, flash * 0.7f));
			drawPolygon(f.shape, f.pos, true);
		} else if (f.carrier >= 0) {
			// En tránsito: brillo tenue pulsante. Al intentar fijarlo en un lugar incorrecto,
			// tiembla y se tiñe de rojo un instante.
			float glow = 0.5f + 0.5f * sin(t * 5.0f);
			float rk = f.rejectAt >= 0 ? ofClamp((t - f.rejectAt) / 0.5f, 0, 1) : 1.0f;
			glm::vec2 shake(sin((t - f.rejectAt) * 60.0f) * cm(0.4f) * (1 - rk), 0);
			ofColor c = Ui::accent.getLerped(ofColor(235, 80, 80), 1 - rk);
			ofSetColor(c, 60 + 50 * glow);
			ofSetLineWidth(8);
			drawPolygon(f.shape, f.pos + shake, false);
			ofSetColor(c, 220);
			drawPolygon(f.shape, f.pos + shake, true);
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

std::vector<std::string> Ensamblaje::instructions() const {
	auto & s = settings().ensamblaje;
	auto & frag = s.fragmentsPerCycle;
	int lo = frag.empty() ? 0 : *std::min_element(frag.begin(), frag.end());
	int hi = frag.empty() ? 0 : *std::max_element(frag.begin(), frag.end());
	return {
		"OBJETIVO: armar la figura del centro con sus piezas.",
		"",
		"- Toca una pieza con tu token para recogerla (una pieza por token).",
		"- Cada pieza calza en un solo lugar de la figura.",
		"- Con la pieza sobre su lugar, unan los dos tokens: la pieza se fija.",
		"- Lugar incorrecto: la pieza tiembla en rojo. Sin separarse,",
		"  pueden deslizarla hasta el lugar correcto.",
		"- Una pieza por unión: separen y vuelvan a unir los tokens",
		"  para fijar la siguiente.",
		"",
		"GANAR: completar la figura. Se mide el tiempo de cada una.",
		ofToString(frag.size()) + " figuras, de " + ofToString(lo) + " a " + ofToString(hi)
			+ " piezas. Los contornos de guía se van borrando.",
	};
}
