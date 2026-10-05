#include "Settings.h"

Settings & settings() {
	static Settings s;
	return s;
}

// Lee `key` de `j` si existe; si no, deja el valor por defecto.
template <typename T>
static void read(const ofJson & j, const char * key, T & value) {
	if (j.contains(key)) {
		try {
			value = j.at(key).get<T>();
		} catch (std::exception & e) {
			ofLogWarning("Settings") << "Valor inválido para '" << key << "': " << e.what();
		}
	}
}

void Settings::load(const std::string & path) {
	ofJson j;
	if (ofFile::doesFileExist(path)) j = ofLoadJson(path);

	if (j.contains("pantalla")) {
		read(j["pantalla"], "pxPerCm", pxPerCm);
	}
	if (j.contains("deteccion")) {
		auto & d = j["deteccion"];
		read(d, "clusterThresholdPx", clusterThresholdPx);
		read(d, "joinedMinPoints", joinedMinPoints);
		read(d, "joinDistanceCm", joinDistanceCm);
		read(d, "tokenGraceFrames", tokenGraceFrames);
		read(d, "joinConfirmFrames", joinConfirmFrames);
		read(d, "separateConfirmFrames", separateConfirmFrames);
		read(d, "labelConfirmFrames", labelConfirmFrames);
		read(d, "trackMaxJumpCm", trackMaxJumpCm);
		read(d, "tokenLetters", tokenLetters);
	}
	if (j.contains("menu")) {
		read(j["menu"], "dwellSec", menuDwellSec);
	}
	if (j.contains("ensamblaje")) {
		auto & e = j["ensamblaje"];
		read(e, "fragmentsPerCycle", ensamblaje.fragmentsPerCycle);
		read(e, "zoneRadiusCm", ensamblaje.zoneRadiusCm);
		read(e, "pickupRadiusCm", ensamblaje.pickupRadiusCm);
		read(e, "minJoinedTokens", ensamblaje.minJoinedTokens);
		read(e, "figureRadiusCm", ensamblaje.figureRadiusCm);
		read(e, "matchMaxDistCm", ensamblaje.matchMaxDistCm);
		read(e, "slotOutlineAlpha", ensamblaje.slotOutlineAlpha);
		read(e, "edgeMarginCm", ensamblaje.edgeMarginCm);
		read(e, "assembleAnimSec", ensamblaje.assembleAnimSec);
		read(e, "resultShowSec", ensamblaje.resultShowSec);
	}
	if (j.contains("resonancia")) {
		auto & r = j["resonancia"];
		read(r, "cycles", resonancia.cycles);
		read(r, "individualPhaseSec", resonancia.individualPhaseSec);
		read(r, "doublePhaseSec", resonancia.doublePhaseSec);
		read(r, "minIndividualNodes", resonancia.minIndividualNodes);
		read(r, "maxIndividualNodes", resonancia.maxIndividualNodes);
		read(r, "spacingsCm", resonancia.spacingsCm);
		read(r, "nodeRadiusCm", resonancia.nodeRadiusCm);
		read(r, "activationRadiusCm", resonancia.activationRadiusCm);
		read(r, "fillSec", resonancia.fillSec);
		read(r, "doubleFillSec", resonancia.doubleFillSec);
		read(r, "drainPerSec", resonancia.drainPerSec);
		read(r, "edgeMarginCm", resonancia.edgeMarginCm);
		read(r, "seed", resonancia.seed);
	}
	if (j.contains("marea")) {
		auto & m = j["marea"];
		read(m, "sessionSec", marea.sessionSec);
		read(m, "maxActivePoints", marea.maxActivePoints);
		read(m, "spawnMinSec", marea.spawnMinSec);
		read(m, "spawnMaxSec", marea.spawnMaxSec);
		read(m, "pointLifetimeSec", marea.pointLifetimeSec);
		read(m, "startFill", marea.startFill);
		read(m, "decayPerSec", marea.decayPerSec);
		read(m, "lowFillPerSec", marea.lowFillPerSec);
		read(m, "highFillPerSec", marea.highFillPerSec);
		read(m, "lowScorePerSec", marea.lowScorePerSec);
		read(m, "highScorePerSec", marea.highScorePerSec);
		read(m, "pointRadiusCm", marea.pointRadiusCm);
		read(m, "holdRadiusCm", marea.holdRadiusCm);
		read(m, "edgeMarginCm", marea.edgeMarginCm);
		read(m, "minSpawnDistanceCm", marea.minSpawnDistanceCm);
		read(m, "fusionDistanceCm", marea.fusionDistanceCm);
		read(m, "fusionMinTokens", marea.fusionMinTokens);
		read(m, "seed", marea.seed);
	}
	if (j.contains("simulador")) {
		read(j["simulador"], "tokenSideCm", simTokenSideCm);
	}

	// Reescribe el archivo para que contenga todas las claves (incluidas las nuevas).
	save(path);
}

// Redondea los float a 4 decimales para que el archivo sea legible al editarlo a mano.
static void tidy(ofJson & j) {
	if (j.is_number_float()) {
		j = std::round(j.get<double>() * 10000.0) / 10000.0;
	} else if (j.is_structured()) {
		for (auto & v : j)
			tidy(v);
	}
}

void Settings::save(const std::string & path) const {
	ofJson j;
	j["pantalla"] = { { "pxPerCm", pxPerCm } };
	j["deteccion"] = {
		{ "clusterThresholdPx", clusterThresholdPx },
		{ "joinedMinPoints", joinedMinPoints },
		{ "joinDistanceCm", joinDistanceCm },
		{ "tokenGraceFrames", tokenGraceFrames },
		{ "joinConfirmFrames", joinConfirmFrames },
		{ "separateConfirmFrames", separateConfirmFrames },
		{ "labelConfirmFrames", labelConfirmFrames },
		{ "trackMaxJumpCm", trackMaxJumpCm },
		{ "tokenLetters", tokenLetters },
	};
	j["menu"] = { { "dwellSec", menuDwellSec } };
	j["ensamblaje"] = {
		{ "fragmentsPerCycle", ensamblaje.fragmentsPerCycle },
		{ "zoneRadiusCm", ensamblaje.zoneRadiusCm },
		{ "pickupRadiusCm", ensamblaje.pickupRadiusCm },
		{ "minJoinedTokens", ensamblaje.minJoinedTokens },
		{ "figureRadiusCm", ensamblaje.figureRadiusCm },
		{ "matchMaxDistCm", ensamblaje.matchMaxDistCm },
		{ "slotOutlineAlpha", ensamblaje.slotOutlineAlpha },
		{ "edgeMarginCm", ensamblaje.edgeMarginCm },
		{ "assembleAnimSec", ensamblaje.assembleAnimSec },
		{ "resultShowSec", ensamblaje.resultShowSec },
	};
	j["resonancia"] = {
		{ "cycles", resonancia.cycles },
		{ "individualPhaseSec", resonancia.individualPhaseSec },
		{ "doublePhaseSec", resonancia.doublePhaseSec },
		{ "minIndividualNodes", resonancia.minIndividualNodes },
		{ "maxIndividualNodes", resonancia.maxIndividualNodes },
		{ "spacingsCm", resonancia.spacingsCm },
		{ "nodeRadiusCm", resonancia.nodeRadiusCm },
		{ "activationRadiusCm", resonancia.activationRadiusCm },
		{ "fillSec", resonancia.fillSec },
		{ "doubleFillSec", resonancia.doubleFillSec },
		{ "drainPerSec", resonancia.drainPerSec },
		{ "edgeMarginCm", resonancia.edgeMarginCm },
		{ "seed", resonancia.seed },
	};
	j["marea"] = {
		{ "sessionSec", marea.sessionSec },
		{ "maxActivePoints", marea.maxActivePoints },
		{ "spawnMinSec", marea.spawnMinSec },
		{ "spawnMaxSec", marea.spawnMaxSec },
		{ "pointLifetimeSec", marea.pointLifetimeSec },
		{ "startFill", marea.startFill },
		{ "decayPerSec", marea.decayPerSec },
		{ "lowFillPerSec", marea.lowFillPerSec },
		{ "highFillPerSec", marea.highFillPerSec },
		{ "lowScorePerSec", marea.lowScorePerSec },
		{ "highScorePerSec", marea.highScorePerSec },
		{ "pointRadiusCm", marea.pointRadiusCm },
		{ "holdRadiusCm", marea.holdRadiusCm },
		{ "edgeMarginCm", marea.edgeMarginCm },
		{ "minSpawnDistanceCm", marea.minSpawnDistanceCm },
		{ "fusionDistanceCm", marea.fusionDistanceCm },
		{ "fusionMinTokens", marea.fusionMinTokens },
		{ "seed", marea.seed },
	};
	j["simulador"] = { { "tokenSideCm", simTokenSideCm } };
	tidy(j);
	ofSavePrettyJson(path, j);
}
