#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include <vector>
#include <cmath>

#if IBM
#include <windows.h>
#include <GL/gl.h>
#elif LIN
#include <GL/gl.h>
#elif APL
#include <OpenGL/gl.h>
#endif

// Costanti della fune
const int NUM_NODES = 20;
const float ROPE_LENGTH = 15.0f; // 15 metri
const float SEGMENT_LENGTH = ROPE_LENGTH / (NUM_NODES - 1);
const float LOAD_MASS = 800.0f; // kg

struct RopeNode {
    float x, y, z;
    float old_x, old_y, old_z;
    bool isFixed;
};

std::vector<RopeNode> ropeNodes;

// DataRefs
XPLMDataRef heli_x, heli_y, heli_z;
XPLMDataRef force_axil, force_side, force_nrml;

// Variabili per il rendering
XPLMWindowID myWindow;

void InitRope() {
    ropeNodes.clear();
    float startX = XPLMGetDataf(heli_x);
    float startY = XPLMGetDataf(heli_y);
    float startZ = XPLMGetDataf(heli_z);

    for (int i = 0; i < NUM_NODES; ++i) {
        RopeNode node = {startX, startY - (i * SEGMENT_LENGTH), startZ,
                         startX, startY - (i * SEGMENT_LENGTH), startZ,
                         (i == 0)};
        ropeNodes.push_back(node);
    }
}

// Callback Fisico - Eseguito ad ogni frame
float PhysicsLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void * inRefcon) {
    if (ropeNodes.empty()) return -1.0f;

    float dt = inElapsedSinceLastCall;
    if (dt > 0.1f) dt = 0.1f; // Previene esplosioni fisiche se il sim lagga

    // Il primo nodo è ancorato all'elicottero (baricentro semplificato)
    ropeNodes[0].x = XPLMGetDataf(heli_x);
    ropeNodes[0].y = XPLMGetDataf(heli_y);
    ropeNodes[0].z = XPLMGetDataf(heli_z);

    // Integrazione Verlet
    for (int i = 1; i < NUM_NODES; i++) {
        RopeNode& node = ropeNodes[i];
        float vx = node.x - node.old_x;
        float vy = node.y - node.old_y;
        float vz = node.z - node.old_z;

        node.old_x = node.x;
        node.old_y = node.y;
        node.old_z = node.z;

        // Aggiunta gravità
        node.x += vx;
        node.y += vy - (9.81f * dt * dt);
        node.z += vz;
    }

    // Risoluzione Vincoli (15 iterazioni per rigidità)
    for (int iter = 0; iter < 15; iter++) {
        for (int i = 0; i < NUM_NODES - 1; i++) {
            RopeNode& p1 = ropeNodes[i];
            RopeNode& p2 = ropeNodes[i + 1];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            float dz = p2.z - p1.z;
            float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (distance == 0.0f) continue;

            float difference = SEGMENT_LENGTH - distance;
            float percent = (difference / distance) / 2.0f;

            float offsetX = dx * percent;
            float offsetY = dy * percent;
            float offsetZ = dz * percent;

            if (!p1.isFixed) { p1.x -= offsetX; p1.y -= offsetY; p1.z -= offsetZ; }
            if (!p2.isFixed) { p2.x += offsetX; p2.y += offsetY; p2.z += offsetZ; }
        }
    }

    // Iniezione forze sull'elicottero
    // Calcolo semplificato della trazione sull'asse Y per far sentire il peso
    float pullForceY = -LOAD_MASS * 9.81f; 
    XPLMSetDataf(force_nrml, pullForceY);

    return -1.0f; // Esegui al prossimo frame
}

// Callback Grafico - Disegna la fune nello spazio 3D OpenGL di X-Plane
int DrawRopeCallback(XPLMDrawingPhase inPhase, int inIsBefore, void * inRefcon) {
    if (ropeNodes.empty()) return 1;

    XPLMSetGraphicsState(0, 0, 0, 0, 1, 1, 0);
    glColor3f(1.0f, 0.5f, 0.0f); // Cavo arancione alta visibilità
    glLineWidth(3.0f);

    glBegin(GL_LINE_STRIP);
    for (const auto& node : ropeNodes) {
        glVertex3f(node.x, node.y, node.z);
    }
    glEnd();

    return 1;
}

PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc) {
    strcpy(outName, "Realistic HESLO Physics");
    strcpy(outSig, "com.bladebrazzers.heslo");
    strcpy(outDesc, "Fisica Verlet per funi di carico appeso.");

    heli_x = XPLMFindDataRef("sim/flightmodel/position/local_x");
    heli_y = XPLMFindDataRef("sim/flightmodel/position/local_y");
    heli_z = XPLMFindDataRef("sim/flightmodel/position/local_z");
    
    // Forze esterne da applicare all'elicottero
    force_axil = XPLMFindDataRef("sim/flightmodel/forces/faxil_plug_acf");
    force_side = XPLMFindDataRef("sim/flightmodel/forces/fside_plug_acf");
    force_nrml = XPLMFindDataRef("sim/flightmodel/forces/fnrml_plug_acf");

    XPLMRegisterFlightLoopCallback(PhysicsLoopCallback, -1.0f, NULL);
    XPLMRegisterDrawCallback(DrawRopeCallback, xplm_Phase_Objects, 0, NULL);

    return 1;
}

PLUGIN_API void XPluginStop(void) {
    XPLMUnregisterFlightLoopCallback(PhysicsLoopCallback, NULL);
    XPLMUnregisterDrawCallback(DrawRopeCallback, xplm_Phase_Objects, 0, NULL);
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { InitRope(); return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam) { }