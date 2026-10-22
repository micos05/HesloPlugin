#define _CRT_SECURE_NO_WARNINGS
#include "XPLMPlugin.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include <vector>
#include <cmath>
#include <cstring>

const int NUM_NODES = 20;
const float SEGMENT_LENGTH = 15.0f / (NUM_NODES - 1);
const float LOAD_MASS = 800.0f; // 800kg

struct RopeNode { float x, y, z, old_x, old_y, old_z; bool isFixed; };
std::vector<RopeNode> ropeNodes;
XPLMDataRef heli_x, heli_y, heli_z, force_nrml;

void InitRope() {
    ropeNodes.clear();
    // Evita il crash se il simulatore non ha caricato l'elicottero
    if (!heli_x || !heli_y || !heli_z) {
        XPLMDebugString("HESLO ERROR: DataRef posizione non trovati.\n");
        return;
    }
    float startX = XPLMGetDataf(heli_x);
    float startY = XPLMGetDataf(heli_y);
    float startZ = XPLMGetDataf(heli_z);

    for (int i = 0; i < NUM_NODES; ++i) {
        ropeNodes.push_back({startX, startY - (i * SEGMENT_LENGTH), startZ, 
                             startX, startY - (i * SEGMENT_LENGTH), startZ, i == 0});
    }
    XPLMDebugString("HESLO: Inizializzazione fune completata con successo.\n");
}

float PhysicsLoopCallback(float inElapsedSinceLastCall, float, int, void *) {
    if (ropeNodes.empty() || !heli_x || !heli_y || !heli_z || !force_nrml) return -1.0f;
    
    // Evita crash per divisione per zero se il simulatore e' in pausa
    if (inElapsedSinceLastCall <= 0.0f) return -1.0f; 

    float dt = (inElapsedSinceLastCall > 0.1f) ? 0.1f : inElapsedSinceLastCall;

    ropeNodes[0].x = XPLMGetDataf(heli_x);
    ropeNodes[0].y = XPLMGetDataf(heli_y);
    ropeNodes[0].z = XPLMGetDataf(heli_z);

    for (int i = 1; i < NUM_NODES; i++) {
        RopeNode& node = ropeNodes[i];
        float vx = node.x - node.old_x, vy = node.y - node.old_y, vz = node.z - node.old_z;
        node.old_x = node.x; node.old_y = node.y; node.old_z = node.z;
        node.x += vx; node.y += vy - (9.81f * dt * dt); node.z += vz;
    }

    for (int iter = 0; iter < 15; iter++) {
        for (int i = 0; i < NUM_NODES - 1; i++) {
            RopeNode& p1 = ropeNodes[i]; RopeNode& p2 = ropeNodes[i + 1];
            float dx = p2.x - p1.x, dy = p2.y - p1.y, dz = p2.z - p1.z;
            float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (distance == 0.0f) continue;
            float percent = ((SEGMENT_LENGTH - distance) / distance) / 2.0f;
            if (!p1.isFixed) { p1.x -= dx * percent; p1.y -= dy * percent; p1.z -= dz * percent; }
            if (!p2.isFixed) { p2.x += dx * percent; p2.y += dy * percent; p2.z += dz * percent; }
        }
    }

    // Iniettare la forza in modo sicuro leggendo prima lo stato del simulatore
    float current_force = XPLMGetDataf(force_nrml);
    XPLMSetDataf(force_nrml, current_force - (LOAD_MASS * 9.81f));
    return -1.0f;
}

PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc) {
    strncpy(outName, "Realistic HESLO Physics", 255);
    strncpy(outSig, "com.bladebrazzers.heslo", 255);
    strncpy(outDesc, "Motore fisico per carichi appesi", 255);

    heli_x = XPLMFindDataRef("sim/flightmodel/position/local_x");
    heli_y = XPLMFindDataRef("sim/flightmodel/position/local_y");
    heli_z = XPLMFindDataRef("sim/flightmodel/position/local_z");
    force_nrml = XPLMFindDataRef("sim/flightmodel/forces/fnrml_plug_acf");

    if (!heli_x || !force_nrml) XPLMDebugString("HESLO ERRORE: Dataref principali mancanti.\n");

    XPLMRegisterFlightLoopCallback(PhysicsLoopCallback, -1.0f, NULL);
    return 1;
}

PLUGIN_API void XPluginStop(void) { XPLMUnregisterFlightLoopCallback(PhysicsLoopCallback, NULL); }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { InitRope(); return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int, void *) { }
