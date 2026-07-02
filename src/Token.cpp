#include "Token.hpp"

// ---------------- constructor / destructor ----------------
Token::Token() {
	tuio.setup(new ofxTuioUdpReceiver(3333));
	ofAddListener(tuio.AddTuioCursor, this, &Token::tuioAdded);
	ofAddListener(tuio.UpdateTuioCursor, this, &Token::tuioUpdated);
	ofAddListener(tuio.RemoveTuioCursor, this, &Token::tuioRemoved);
	tuio.connect(false);
}

Token::~Token() {
	ofRemoveListener(tuio.AddTuioCursor, this, &Token::tuioAdded);
	ofRemoveListener(tuio.UpdateTuioCursor, this, &Token::tuioUpdated);
	ofRemoveListener(tuio.RemoveTuioCursor, this, &Token::tuioRemoved);
	tuio.disconnect();
}

void Token::setup(const ofVec2f & pos) {
	cursors.clear();
	scaledCursors.clear();
	lockedPoints.clear();
	locked = false;
	detectedId = '?';
}

void Token::update() {
	// actualizar scaledCursors desde cursors
	scaledCursors.clear();
	for (auto & t : cursors) {
		scaledCursors.emplace_back(t.pos.x * ofGetWidth(), t.pos.y * ofGetHeight());
	}

	// LOCKING LOGIC:
	// - Si no estamos locked y aparece >= 3 puntos -> hacemos snapshot (lock) usando los puntos actuales
	// - Mientras locked = true, no re-clasificamos (la clasificación queda con la snapshot)
	// - Cuando locked==true y no hay puntos (scaledCursors.empty()) -> reset completo
	if (!locked) {
		if (scaledCursors.size() >= 3) {
			// tomar snapshot (primeros puntos detectados en este instante)
			locked = true;
			lockedPoints = orderByAngle(scaledCursors); // ordenados para clasificación estable
			detectedId = classifyShape(lockedPoints);
		} else {
			// no hay token (o no suficientes puntos) -> nada
			detectedId = '?';
		}
	} else {
		// locked == true
		if (scaledCursors.empty()) {
			// token removido -> reset
			locked = false;
			lockedPoints.clear();
			detectedId = '?';
		}
		// else locked stays true and classification remains the same
	}
}

void Token::draw() {
	// Dibujar puntos actuales (círculos)
	ofSetLineWidth(2);
	ofFill();
	ofSetColor(200, 100, 255);
	for (auto & p : scaledCursors) {
		ofDrawCircle(p, 15);
	}

	// Si hay snapshot locked, dibujar centroid del snapshot y texto desplazado
	if (locked && !lockedPoints.empty()) {
		ofVec2f c = centroid(lockedPoints);
		ofNoFill();
		ofSetLineWidth(2);
		ofSetColor(120, 120, 120);
		ofDrawCircle(c, 4); // marca del centro del token (snapshot)

		// texto desplazado a la derecha y abajo para evitar colisiones con ejes
		ofSetColor(255);
		float offsetX = 40;
		float offsetY = 20;
		ofDrawBitmapString(string("Token: ") + detectedId, c.x + offsetX, c.y + offsetY);
	} else {
		// si no hay snapshot pero hay puntos actuales, dibujar una etiqueta temporal cerca del primer punto
		if (!scaledCursors.empty()) {
			ofVec2f base = scaledCursors[0];
			ofSetColor(255);
			ofDrawBitmapString(string("Token: ?"), base.x + 25, base.y + 20);
		}
	}
}

// setPoints: opcional (debug)
void Token::setPoints(const vector<ofVec2f> & newPts) {
	// sustituye scaledCursors (útil para debug sin TUIO)
	scaledCursors = newPts;
	// no tocamos cursors ni locked; si quieres forzar lock, usa la lógica en update()
}

vector<ofVec2f> Token::getPoints() const {
	return scaledCursors;
}

vector<ofVec2f> Token::getLockedPoints() const {
	return locked ? lockedPoints : vector<ofVec2f> {}; // devuelve vacío si no hay lock
}

bool Token::isLocked() const {
	return locked;
}

char Token::getId() const {
	return detectedId;
}

// ----------------- TUIO callbacks -----------------
void Token::tuioAdded(ofxTuioCursor & c) {
	cursors.push_back({ ofVec2f(c.getX(), c.getY()), c.getSessionID() });
}

void Token::tuioUpdated(ofxTuioCursor & c) {
	for (auto & t : cursors) {
		if (t.id == c.getSessionID()) {
			t.pos = ofVec2f(c.getX(), c.getY());
			return;
		}
	}
}

void Token::tuioRemoved(ofxTuioCursor & c) {
	long id = c.getSessionID();
	cursors.erase(
		std::remove_if(cursors.begin(), cursors.end(),
			[&](auto & t) { return t.id == id; }),
		cursors.end());
}

// ----------------- utilidades geométricas -----------------
ofVec2f Token::centroid(const vector<ofVec2f> & pts) const {
	ofVec2f c(0, 0);
	for (auto & p : pts)
		c += p;
	c /= (float)pts.size();
	return c;
}

vector<ofVec2f> Token::orderByAngle(const vector<ofVec2f> & pts) const {
	if (pts.size() <= 1) return pts;
	auto c = centroid(pts);
	vector<pair<float, ofVec2f>> tmp;
	tmp.reserve(pts.size());
	for (auto & p : pts) {
		float a = atan2(p.y - c.y, p.x - c.x);
		tmp.emplace_back(a, p);
	}
	sort(tmp.begin(), tmp.end(), [](auto & a, auto & b) { return a.first < b.first; });
	vector<ofVec2f> out;
	out.reserve(tmp.size());
	for (auto & t : tmp)
		out.push_back(t.second);
	return out;
}

float Token::angleBetween(const ofVec2f & a, const ofVec2f & b, const ofVec2f & c) const {
	ofVec2f ab = (a - b).getNormalized();
	ofVec2f cb = (c - b).getNormalized();
	float dot = ab.dot(cb);
	return acos(ofClamp(dot, -1.f, 1.f)) * RAD_TO_DEG;
}

bool Token::approxEqual(float a, float b, float tolerance) const {
	return fabs(a - b) < tolerance;
}

// ----------------- clasificación -----------------
char Token::classifyShape(const vector<ofVec2f> & pts) const {
	int n = pts.size();
	if (n < 3) return '?';

	// calcular lados (distancias entre consecutivos)
	vector<float> sides;
	sides.reserve(n);
	for (int i = 0; i < n; ++i)
		sides.push_back(pts[i].distance(pts[(i + 1) % n]));

	// calcular ángulos internos
	vector<float> angles;
	angles.reserve(n);
	for (int i = 0; i < n; ++i)
		angles.push_back(angleBetween(pts[(i - 1 + n) % n], pts[i], pts[(i + 1) % n]));

	float avgSide = 0;
	for (auto s : sides)
		avgSide += s;
	avgSide /= sides.size();
	float sideTol = max(12.0f, avgSide * 0.15f);
	float angleTol = 15.0f;

	auto allSidesEqual = [&](float tol) {
		for (auto s : sides)
			if (!approxEqual(s, sides[0], tol)) return false;
		return true;
	};
	auto anglesAllNear = [&](float target) {
		for (auto a : angles)
			if (!approxEqual(a, target, angleTol)) return false;
		return true;
	};

	if (n == 3) {
		if (allSidesEqual(sideTol) && anglesAllNear(60.0f)) return 'A'; // equilátero
		int eqCount = 0;
		if (approxEqual(sides[0], sides[1], sideTol)) ++eqCount;
		if (approxEqual(sides[1], sides[2], sideTol)) ++eqCount;
		if (approxEqual(sides[2], sides[0], sideTol)) ++eqCount;
		if (eqCount >= 1) {
			if (angles[0] < 50.0f)
				return 'B';
			else
				return 'C'; // isósceles variantes
		}
		for (auto a : angles)
			if (approxEqual(a, 90.0f, 12.0f)) return 'D'; // rectángulo
		return 'E'; // escaleno
	}

	if (n == 4) {
		if (allSidesEqual(sideTol) && anglesAllNear(90.0f)) return 'F'; // cuadrado
		if (approxEqual(sides[0], sides[2], sideTol) && approxEqual(sides[1], sides[3], sideTol)
			&& anglesAllNear(90.0f)) return 'G'; // rectángulo
		if (allSidesEqual(sideTol) && !anglesAllNear(90.0f)) return 'H'; // rombo
		if (approxEqual(angles[0], angles[2], angleTol) && !allSidesEqual(sideTol)) return 'I'; // trapecio isósceles
		return 'J';
	}

	if (n == 5) return 'K';
	if (n == 6) return 'L';

	return '?';
}
