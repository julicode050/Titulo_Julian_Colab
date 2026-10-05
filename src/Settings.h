#pragma once
#include "ofMain.h"

// Todos los parámetros calibrables viven aquí y se leen de bin/data/settings.json.
// Si el archivo no existe (o le faltan claves), se escriben los valores por defecto,
// así el archivo siempre muestra todos los parámetros disponibles.
// Las distancias de juego están en cm; las de detección en px (valores ya validados en hardware).
struct Settings {
	// --- Pantalla ---
	float pxPerCm = 36.43f; // ViewSonic TD2455: pixel pitch 0,2745 mm, área visible 527 x 296,5 mm

	// --- Detección ---
	float clusterThresholdPx = 150.0f; // puntos más cercanos que esto pertenecen al mismo grupo
	int joinedMinPoints = 6; // un grupo con >= estos puntos es un par de tokens unidos
	float joinDistanceCm = 5.0f; // dos tokens separados cuyos centros están más cerca que esto cuentan como unidos (0 = desactivado)
	int tokenGraceFrames = 15; // frames que un token puede desaparecer sin considerarse retirado
	int joinConfirmFrames = 5; // frames seguidos para confirmar una unión
	int separateConfirmFrames = 10; // frames seguidos para confirmar una separación
	int labelConfirmFrames = 10; // frames seguidos para cambiar la etiqueta de un token
	float trackMaxJumpCm = 8.0f; // salto máximo por frame para seguir siendo el mismo token
	// Etiqueta de cada token físico -> letras que el clasificador puede devolver para él.
	// (En el juego anterior, un mismo token se leía como 'B' o 'C'.)
	std::map<std::string, std::string> tokenLetters = { { "A", "A" }, { "B", "BC" } };

	// --- Menú ---
	float menuDwellSec = 1.5f;

	// --- Ensamblaje Progresivo ---
	struct {
		std::vector<int> fragmentsPerCycle = { 3, 3, 4, 4, 5, 5 };
		float zoneRadiusCm = 10.5f;
		float pickupRadiusCm = 2.5f;
		int minJoinedTokens = 2; // tokens unidos que deben estar en la zona para fijar un fragmento
		float figureRadiusCm = 8.5f;
		float matchMaxDistCm = 3.5f; // distancia máxima del token a su lugar para fijar el fragmento
		// Opacidad de los contornos de cada lugar, por ciclo (1 = visible, 0 = solo la silueta).
		std::vector<float> slotOutlineAlpha = { 1.0f, 1.0f, 0.6f, 0.3f, 0.0f, 0.0f };
		float edgeMarginCm = 3.5f;
		float assembleAnimSec = 1.2f;
		float resultShowSec = 4.0f;
	} ensamblaje;

	// --- Resonancia Dividida ---
	struct {
		int cycles = 7;
		float individualPhaseSec = 27.0f;
		float doublePhaseSec = 18.0f;
		int minIndividualNodes = 2;
		int maxIndividualNodes = 3;
		std::vector<float> spacingsCm = { 6.0f, 14.0f, 24.0f }; // cerca / medio / lejos
		float nodeRadiusCm = 2.2f;
		float activationRadiusCm = 3.5f;
		float fillSec = 2.0f;
		float doubleFillSec = 2.5f;
		float drainPerSec = 0.25f;
		float edgeMarginCm = 4.0f;
		int seed = 1234;
	} resonancia;

	// --- Marea de Presión ---
	struct {
		float sessionSec = 330.0f;
		int maxActivePoints = 3;
		float spawnMinSec = 6.0f;
		float spawnMaxSec = 12.0f;
		float pointLifetimeSec = 40.0f;
		float startFill = 0.6f;
		float decayPerSec = 0.04f; // vacío total en ~25 s sin tokens
		float lowFillPerSec = 0.05f; // ganancia bruta con 1 token (neto ~ +0.01/s)
		float highFillPerSec = 0.15f; // ganancia bruta con tokens unidos
		float lowScorePerSec = 1.0f;
		float highScorePerSec = 4.0f;
		float pointRadiusCm = 2.6f;
		float holdRadiusCm = 3.5f;
		float edgeMarginCm = 4.0f;
		float minSpawnDistanceCm = 5.0f;
		float fusionDistanceCm = 8.0f;
		int fusionMinTokens = 4; // con 2 personas nunca se alcanza: esperado
		int seed = 4321;
	} marea;

	// --- Simulador ---
	float simTokenSideCm = 2.5f;

	float cm(float v) const { return v * pxPerCm; }

	void load(const std::string & path);
	void save(const std::string & path) const;
};

// Instancia global de configuración (se carga en ofApp::setup).
Settings & settings();
