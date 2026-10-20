/**
 * ============================================================================
 * PROJECT : Cyber-Runner: Walking Robot vs Monsters
 *           (Chrome Dino Style 2D Runner Game)
 * SUBJECT : Computer Graphics Mini Project (B.Tech / BCA / MCA / B.Sc CS)
 * STACK   : C++ and OpenGL with GLUT / Native Win32 Compatibility
 * ============================================================================
 * 
 * GAMEPLAY MECHANICS:
 *  - Endless runner where the robot sprints horizontally across terrain.
 *  - Monsters & obstacles scroll toward the robot from right to left!
 *  - Player presses [SPACE] or [UP ARROW] to jump over incoming threats.
 *  - Smooth realistic gravity physics (variable height jumping).
 *  - Procedural obstacle spawning with multiple monster/hazard types:
 *      1. Ground Alien Monster (chomping jaws, glowing eyes, animated tentacles)
 *      2. Cyber-Bat Drone (low-altitude flying enemy)
 *      3. Neon Hazard Spikes (steel trap)
 *      4. Energy Crate (caution barrier)
 *  - Accurate AABB collision detection with particle crash effects.
 *  - Live Score & High Score tracking with speed scaling difficulty.
 *  - Start Screen, Game Over Screen, and Pause Screen.
 * 
 * COMPUTER GRAPHICS CONCEPTS:
 *  - 2D Primitives: GL_QUADS, GL_POLYGON, GL_TRIANGLES, GL_LINES, GL_TRIANGLE_FAN
 *  - Hierarchical Robot Modeling: Scene graph with glPushMatrix / glPopMatrix
 *  - Kinematics & Gravity Physics: Acceleration, velocity, and jump mechanics
 *  - Parallax Scrolling: Multi-layered background scrolling at varied speeds
 *  - Particle System: Spark particles upon collision and jump dust
 *  - Double Buffering & Alpha Blending: Fluid 60 FPS animation & transparency
 * ============================================================================
 */

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Platform-specific OpenGL and GLUT header inclusions
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
#else
    #ifdef _WIN32
        #include <windows.h>
    #endif
    #include <GL/glut.h>
#endif

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// ============================================================================
// GLOBAL CONFIGURATION & GAME CONSTANTS
// ============================================================================
const int   WINDOW_WIDTH   = 1000;
const int   WINDOW_HEIGHT  = 600;
const float GROUND_Y       = 130.0f; // Surface level of the ground

// Jump Physics Constants
const float GRAVITY        = 0.68f;  // Gravitational pull per frame
const float JUMP_VELOCITY  = 14.5f;  // Initial jump launch impulse
const float ROBOT_SCREEN_X = 140.0f; // Fixed horizontal screen position of runner

// Game States
enum GameState {
    STATE_START = 0,
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_PAUSED
};

GameState gameState = STATE_START;

// ============================================================================
// PARTICLE SYSTEM FOR JUMP DUST & EXPLOSIONS
// ============================================================================
struct Particle {
    float x, y;
    float vx, vy;
    float life;    // Remaining lifetime [0.0, 1.0]
    float decay;   // Fade rate per frame
    float r, g, b;
    float size;
};

std::vector<Particle> particles;

void spawnJumpDust(float x, float y) {
    for (int i = 0; i < 12; ++i) {
        Particle p;
        p.x = x + ((float)rand() / (float)RAND_MAX * 20.0f - 10.0f);
        p.y = y + 2.0f;
        p.vx = -((float)rand() / (float)RAND_MAX * 2.5f + 1.0f);
        p.vy = ((float)rand() / (float)RAND_MAX * 2.5f + 0.5f);
        p.life = 1.0f;
        p.decay = 0.05f + ((float)rand() / (float)RAND_MAX * 0.04f);
        p.r = 0.8f; p.g = 0.85f; p.b = 0.9f;
        p.size = 2.5f + ((float)rand() / (float)RAND_MAX * 2.5f);
        particles.push_back(p);
    }
}

void spawnCrashExplosion(float x, float y) {
    for (int i = 0; i < 40; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * (float)M_PI;
        float speed = 2.0f + ((float)rand() / (float)RAND_MAX * 7.0f);
        p.vx = cosf(angle) * speed;
        p.vy = sinf(angle) * speed + 1.5f;
        p.life = 1.0f;
        p.decay = 0.025f + ((float)rand() / (float)RAND_MAX * 0.025f);
        // Bright fire / spark colors (red, orange, yellow, cyan)
        if (rand() % 3 == 0) {
            p.r = 0.0f; p.g = 0.9f; p.b = 1.0f; // Electric cyan spark
        } else {
            p.r = 1.0f; p.g = 0.4f + ((float)rand() / (float)RAND_MAX * 0.5f); p.b = 0.1f;
        }
        p.size = 3.0f + ((float)rand() / (float)RAND_MAX * 3.5f);
        particles.push_back(p);
    }
}

// ============================================================================
// OBSTACLE & MONSTER STRUCTURE
// ============================================================================
enum ObstacleType {
    OBS_MONSTER_GROUND = 0, // Chomping Alien Beast
    OBS_FLYING_DRONE   = 1, // Cyber-Bat Drone (air hazard)
    OBS_SPIKES         = 2, // Neon Ground Spikes
    OBS_CRATE          = 3  // Power Crate
};

struct Obstacle {
    float x;
    float y;
    float width;
    float height;
    ObstacleType type;
    float animTimer;
    bool passed;
};

std::vector<Obstacle> obstacles;
float spawnCooldown = 0.0f;

// ============================================================================
// ROBOT & GAMEPLAY VARIABLES
// ============================================================================
float robotY       = GROUND_Y;
float robotVy      = 0.0f;     // Vertical velocity
bool  isGrounded   = true;     // Whether robot is touching the ground
float runPhase     = 0.0f;     // Phase for walking/running limb oscillation

// Game Speed & Progression
float gameSpeed    = 6.5f;     // World scroll speed (pixels per frame)
float baseSpeed    = 6.5f;
float score        = 0.0f;
float highScore    = 0.0f;
int   monstersDodged = 0;
float screenShake  = 0.0f;     // Shake effect when crashing

// Parallax Background Offsets
float bgOffsetFar  = 0.0f;
float bgOffsetMid  = 0.0f;
float groundOffset = 0.0f;

// Decorative Background Elements
struct Star { float x, y, brightness; };
std::vector<Star> stars;

struct Cloud { float x, y, speed, scale; };
std::vector<Cloud> clouds;

// ============================================================================
// HELPER DRAWING PRIMITIVES
// ============================================================================

void drawCircle(float cx, float cy, float r, int segments = 24) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * (float)M_PI * (float)i / (float)segments;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

void drawCircleOutline(float cx, float cy, float r, int segments = 24) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float theta = 2.0f * (float)M_PI * (float)i / (float)segments;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawRectOutline(float x, float y, float w, float h) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawRoundedRect(float x, float y, float w, float h, float r) {
    drawRect(x + r, y, w - 2.0f * r, h);
    drawRect(x, y + r, w, h - 2.0f * r);
    drawCircle(x + r, y + r, r, 12);
    drawCircle(x + w - r, y + r, r, 12);
    drawCircle(x + w - r, y + h - r, r, 12);
    drawCircle(x + r, y + h - r, r, 12);
}

void renderBitmapText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < text.length(); ++i) {
        glutBitmapCharacter(font, text[i]);
    }
}

// ============================================================================
// ENVIRONMENT & SCENERY RENDERING
// ============================================================================

void drawSky() {
    // Cyberpunk Dusk Sky Gradient
    glBegin(GL_QUADS);
    glColor3f(0.04f, 0.05f, 0.12f); // Deep Space Navy
    glVertex2f(0.0f, (float)WINDOW_HEIGHT);
    glVertex2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    glColor3f(0.12f, 0.18f, 0.30f); // Atmospheric Cyan Dusk
    glVertex2f((float)WINDOW_WIDTH, GROUND_Y);
    glVertex2f(0.0f, GROUND_Y);
    glEnd();

    // Twinkling Stars
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < stars.size(); ++i) {
        float b = stars[i].brightness * (0.7f + 0.3f * sinf(runPhase * 2.0f + (float)i));
        glColor3f(b, b, b * 1.2f);
        glVertex2f(stars[i].x, stars[i].y);
    }
    glEnd();

    // Neon Cyber Moon with Halo
    float moonX = 850.0f, moonY = 500.0f, moonR = 36.0f;
    glColor4f(0.0f, 0.85f, 0.95f, 0.12f);
    drawCircle(moonX, moonY, moonR + 14.0f, 32);
    glColor3f(0.92f, 0.96f, 1.0f);
    drawCircle(moonX, moonY, moonR, 32);
    glColor3f(0.05f, 0.07f, 0.15f);
    drawCircle(moonX - 12.0f, moonY + 8.0f, moonR - 4.0f, 32);
}

void drawMountains() {
    // Distant mountain range scrolling at 15% world speed (Parallax)
    glColor3f(0.08f, 0.12f, 0.20f);
    float offset = fmodf(bgOffsetFar, 600.0f);
    for (int k = -1; k <= 2; ++k) {
        float base = (float)k * 600.0f - offset;
        glBegin(GL_TRIANGLES);
        glVertex2f(base, GROUND_Y);
        glVertex2f(base + 200.0f, 320.0f);
        glVertex2f(base + 400.0f, GROUND_Y);

        glVertex2f(base + 300.0f, GROUND_Y);
        glVertex2f(base + 480.0f, 360.0f);
        glVertex2f(base + 650.0f, GROUND_Y);
        glEnd();
    }

    // Midground hills scrolling at 40% world speed
    glColor3f(0.12f, 0.18f, 0.26f);
    float offsetMid = fmodf(bgOffsetMid, 500.0f);
    for (int k = -1; k <= 2; ++k) {
        float base = (float)k * 500.0f - offsetMid;
        glBegin(GL_TRIANGLES);
        glVertex2f(base, GROUND_Y);
        glVertex2f(base + 160.0f, 240.0f);
        glVertex2f(base + 350.0f, GROUND_Y);

        glVertex2f(base + 260.0f, GROUND_Y);
        glVertex2f(base + 420.0f, 260.0f);
        glVertex2f(base + 580.0f, GROUND_Y);
        glEnd();
    }
}

void drawClouds() {
    for (size_t i = 0; i < clouds.size(); ++i) {
        float cx = clouds[i].x;
        float cy = clouds[i].y;
        float s  = clouds[i].scale;

        glColor4f(0.85f, 0.90f, 0.98f, 0.22f);
        drawCircle(cx, cy, 26.0f * s, 20);
        drawCircle(cx + 22.0f * s, cy + 6.0f * s, 20.0f * s, 20);
        drawCircle(cx - 20.0f * s, cy + 4.0f * s, 18.0f * s, 20);
        drawRect(cx - 30.0f * s, cy - 12.0f * s, 60.0f * s, 12.0f * s);
    }
}

void drawGround() {
    // Deep ground foundation slab
    glColor3f(0.07f, 0.08f, 0.11f);
    drawRect(0.0f, 0.0f, (float)WINDOW_WIDTH, GROUND_Y);

    // Tech runway surface band
    glColor3f(0.13f, 0.15f, 0.20f);
    drawRect(0.0f, GROUND_Y - 26.0f, (float)WINDOW_WIDTH, 26.0f);

    // Fast-scrolling runway markers (gives sensation of speed!)
    float dashOffset = fmodf(groundOffset, 70.0f);
    glColor3f(0.22f, 0.27f, 0.35f);
    for (float x = -70.0f; x < (float)WINDOW_WIDTH + 70.0f; x += 70.0f) {
        float px = x - dashOffset;
        drawRect(px, GROUND_Y - 18.0f, 32.0f, 5.0f);
    }

    // Glowing Neon Guide Line along top of ground
    glColor3f(0.0f, 0.90f, 0.95f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, GROUND_Y);
    glVertex2f((float)WINDOW_WIDTH, GROUND_Y);
    glEnd();

    // Subtle neon under-glow
    glColor3f(0.0f, 0.45f, 0.55f);
    drawRect(0.0f, GROUND_Y - 4.0f, (float)WINDOW_WIDTH, 3.0f);
}

// ============================================================================
// MONSTERS & OBSTACLES RENDERING
// ============================================================================

/**
 * Monster 1: Chomping Alien Cyber Beast (Chrome Dino Monster style!)
 * Animated snapping teeth, glowing menace eyes, and wriggling limbs.
 */
void drawAlienMonster(float x, float y, float w, float h, float anim) {
    float bob = sinf(anim * 8.0f) * 3.0f;
    float jawOpen = fabsf(sinf(anim * 6.0f)) * 14.0f; // Jaws opening & closing!

    // Monster main body (Purple/Violet alien flesh)
    glColor3f(0.72f, 0.15f, 0.45f);
    drawRoundedRect(x + 4.0f, y + 10.0f + bob, w - 8.0f, h - 14.0f, 6.0f);

    // Dark under-belly
    glColor3f(0.48f, 0.08f, 0.30f);
    drawRect(x + 8.0f, y + 10.0f + bob, w - 16.0f, 8.0f);

    // Monster Snapping Mouth (Upper & Lower Jaws)
    glColor3f(0.12f, 0.02f, 0.08f); // Mouth cavity
    drawRect(x - 4.0f, y + 18.0f + bob - jawOpen * 0.5f, 22.0f, 10.0f + jawOpen);

    // Sharp White Teeth (Top & Bottom)
    glColor3f(0.98f, 0.98f, 0.98f);
    glBegin(GL_TRIANGLES);
    // Upper sharp teeth
    glVertex2f(x - 4.0f, y + 28.0f + bob + jawOpen * 0.5f);
    glVertex2f(x + 2.0f, y + 20.0f + bob + jawOpen * 0.5f);
    glVertex2f(x + 8.0f, y + 28.0f + bob + jawOpen * 0.5f);

    glVertex2f(x + 8.0f, y + 28.0f + bob + jawOpen * 0.5f);
    glVertex2f(x + 13.0f, y + 20.0f + bob + jawOpen * 0.5f);
    glVertex2f(x + 18.0f, y + 28.0f + bob + jawOpen * 0.5f);

    // Lower sharp teeth
    glVertex2f(x - 2.0f, y + 18.0f + bob - jawOpen * 0.5f);
    glVertex2f(x + 4.0f, y + 25.0f + bob - jawOpen * 0.5f);
    glVertex2f(x + 9.0f, y + 18.0f + bob - jawOpen * 0.5f);
    glEnd();

    // Glowing Menacing Monster Eyes (Yellow/Red)
    float eyePulse = 0.85f + 0.15f * sinf(anim * 10.0f);
    glColor3f(1.0f * eyePulse, 0.1f, 0.1f);
    drawCircle(x + w * 0.55f, y + h - 10.0f + bob, 5.5f, 16);
    glColor3f(1.0f, 0.9f, 0.2f);
    drawCircle(x + w * 0.55f, y + h - 10.0f + bob, 3.0f, 12);
    // Evil pupil
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(x + w * 0.55f - 1.0f, y + h - 13.0f + bob, 2.0f, 6.0f);

    // Back Spikes / Dorsal Fins
    glColor3f(0.95f, 0.2f, 0.2f);
    glBegin(GL_TRIANGLES);
    for (int s = 0; s < 3; ++s) {
        float sx = x + 12.0f + (float)s * 10.0f;
        glVertex2f(sx, y + h - 5.0f + bob);
        glVertex2f(sx + 5.0f, y + h + 8.0f + bob);
        glVertex2f(sx + 10.0f, y + h - 5.0f + bob);
    }
    glEnd();

    // Animated Walking Scuttling Legs
    glColor3f(0.35f, 0.06f, 0.22f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    float legSwing1 = sinf(anim * 14.0f) * 10.0f;
    float legSwing2 = -sinf(anim * 14.0f) * 10.0f;
    // Front Leg
    glVertex2f(x + 12.0f, y + 12.0f + bob);
    glVertex2f(x + 6.0f + legSwing1, y);
    // Middle Leg
    glVertex2f(x + w * 0.5f, y + 12.0f + bob);
    glVertex2f(x + w * 0.5f + legSwing2, y);
    // Back Leg
    glVertex2f(x + w - 10.0f, y + 12.0f + bob);
    glVertex2f(x + w - 4.0f + legSwing1, y);
    glEnd();
}

/**
 * Monster 2: Cyber-Bat Drone (Low Flying Aerial Threat!)
 * Flapping mechanical cyber-wings and pulsing thruster glow.
 */
void drawFlyingDrone(float x, float y, float w, float h, float anim) {
    float wingFlap = sinf(anim * 16.0f) * 18.0f; // Rapid wing flap
    float floatY   = sinf(anim * 6.0f) * 6.0f;

    float cy = y + h * 0.5f + floatY;
    float cx = x + w * 0.5f;

    // Center metallic drone orb
    glColor3f(0.20f, 0.24f, 0.30f);
    drawCircle(cx, cy, 14.0f, 20);

    // Glowing Cyclops Eye / Sensor
    glColor3f(1.0f, 0.15f, 0.25f);
    drawCircle(cx - 3.0f, cy, 6.0f, 16);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(cx - 4.0f, cy + 1.5f, 2.0f, 10);

    // Articulated Cyber-Wings (Left and Right)
    glColor3f(0.85f, 0.25f, 0.15f); // Red danger wings
    glBegin(GL_TRIANGLES);
    // Left Wing (Facing forward direction of drone)
    glVertex2f(cx - 8.0f, cy + 2.0f);
    glVertex2f(cx - 28.0f, cy + 12.0f + wingFlap);
    glVertex2f(cx - 16.0f, cy - 6.0f);

    // Right Wing
    glVertex2f(cx + 8.0f, cy + 2.0f);
    glVertex2f(cx + 28.0f, cy + 12.0f - wingFlap);
    glVertex2f(cx + 16.0f, cy - 6.0f);
    glEnd();

    // Rear Jet Engine Thruster Glow
    float flamePulse = 0.7f + 0.3f * sinf(anim * 25.0f);
    glColor3f(0.0f, 0.85f * flamePulse, 1.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + 12.0f, cy + 4.0f);
    glVertex2f(cx + 24.0f + 6.0f * flamePulse, cy);
    glVertex2f(cx + 12.0f, cy - 4.0f);
    glEnd();
}

/**
 * Obstacle 3: Neon Hazard Caution Spikes
 */
void drawSpikes(float x, float y, float w, float h) {
    glColor3f(0.25f, 0.28f, 0.32f);
    drawRect(x, y, w, 8.0f);

    int numSpikes = 3;
    float sw = w / (float)numSpikes;
    for (int i = 0; i < numSpikes; ++i) {
        float sx = x + (float)i * sw;
        glColor3f(0.95f, 0.30f, 0.20f);
        glBegin(GL_TRIANGLES);
        glVertex2f(sx, y + 8.0f);
        glVertex2f(sx + sw * 0.5f, y + h);
        glVertex2f(sx + sw * 0.5f, y + 8.0f);
        glEnd();

        glColor3f(0.65f, 0.15f, 0.12f);
        glBegin(GL_TRIANGLES);
        glVertex2f(sx + sw * 0.5f, y + 8.0f);
        glVertex2f(sx + sw * 0.5f, y + h);
        glVertex2f(sx + sw, y + 8.0f);
        glEnd();
    }
}

/**
 * Obstacle 4: High-Tech Caution Crate
 */
void drawCrate(float x, float y, float w, float h) {
    glColor3f(0.22f, 0.25f, 0.29f);
    drawRect(x, y, w, h);

    int stripes = 4;
    float sw = w / (float)stripes;
    for (int i = 0; i < stripes; ++i) {
        if (i % 2 == 0) glColor3f(0.95f, 0.80f, 0.10f); // Caution Yellow
        else           glColor3f(0.12f, 0.12f, 0.12f); // Black
        drawRect(x + (float)i * sw, y + 4.0f, sw, h - 8.0f);
    }

    glColor3f(0.40f, 0.45f, 0.50f);
    glLineWidth(2.5f);
    drawRectOutline(x, y, w, h);

    // Flashing red LED on top
    float blink = 0.5f + 0.5f * sinf(runPhase * 6.0f);
    glColor3f(0.95f * blink, 0.1f, 0.1f);
    drawCircle(x + w * 0.5f, y + h + 6.0f, 4.5f, 14);
}

void drawObstacles() {
    for (size_t i = 0; i < obstacles.size(); ++i) {
        const Obstacle& obs = obstacles[i];
        switch (obs.type) {
            case OBS_MONSTER_GROUND:
                drawAlienMonster(obs.x, obs.y, obs.width, obs.height, obs.animTimer);
                break;
            case OBS_FLYING_DRONE:
                drawFlyingDrone(obs.x, obs.y, obs.width, obs.height, obs.animTimer);
                break;
            case OBS_SPIKES:
                drawSpikes(obs.x, obs.y, obs.width, obs.height);
                break;
            case OBS_CRATE:
                drawCrate(obs.x, obs.y, obs.width, obs.height);
                break;
        }
    }
}

// ============================================================================
// HIERARCHICAL RUNNING ROBOT MODELING
// ============================================================================

void drawShadow(float heightAboveGround) {
    float elevationFraction = heightAboveGround / 140.0f;
    if (elevationFraction > 1.0f) elevationFraction = 1.0f;

    float rx = 28.0f * (1.0f - 0.5f * elevationFraction);
    float ry = 6.0f  * (1.0f - 0.5f * elevationFraction);
    float alpha = 0.45f * (1.0f - 0.7f * elevationFraction);

    glColor4f(0.01f, 0.02f, 0.04f, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, -heightAboveGround);
    for (int i = 0; i <= 24; ++i) {
        float theta = 2.0f * (float)M_PI * (float)i / 24.0f;
        glVertex2f(rx * cosf(theta), -heightAboveGround + ry * sinf(theta));
    }
    glEnd();
}

void drawRobotLeg(float hipAngle, bool isLeft) {
    glPushMatrix();
    glRotatef(hipAngle, 0.0f, 0.0f, 1.0f);

    // Hip joint
    if (isLeft) glColor3f(0.32f, 0.38f, 0.46f);
    else        glColor3f(0.20f, 0.24f, 0.30f);
    drawCircle(0.0f, 0.0f, 6.0f, 16);

    // Thigh
    float thighW = 8.0f, thighH = 22.0f;
    drawRect(-thighW * 0.5f, -thighH, thighW, thighH);

    // Knee
    glTranslatef(0.0f, -thighH, 0.0f);
    drawCircle(0.0f, 0.0f, 5.0f, 16);

    // Running knee bend articulation
    float kneeAngle = 0.0f;
    if (gameState == STATE_GAMEOVER) {
        kneeAngle = 60.0f;
    } else if (!isGrounded) {
        kneeAngle = 35.0f; // Tuck legs while jumping
    } else {
        kneeAngle = (hipAngle < 0.0f) ? fabsf(hipAngle) * 1.1f : 10.0f;
    }
    glRotatef(kneeAngle, 0.0f, 0.0f, 1.0f);

    // Shin
    float shinW = 7.0f, shinH = 20.0f;
    drawRect(-shinW * 0.5f, -shinH, shinW, shinH);

    // Foot boot
    glTranslatef(0.0f, -shinH, 0.0f);
    if (isLeft) glColor3f(0.90f, 0.45f, 0.15f); // Orange boot
    else        glColor3f(0.65f, 0.28f, 0.10f);
    drawRoundedRect(-5.0f, -7.0f, 18.0f, 7.0f, 2.0f);

    glPopMatrix();
}

void drawRobotArm(float shoulderAngle, bool isLeft) {
    glPushMatrix();
    glRotatef(shoulderAngle, 0.0f, 0.0f, 1.0f);

    // Shoulder joint cap
    if (isLeft) glColor3f(0.0f, 0.90f, 0.95f); // Cyan accent
    else        glColor3f(0.0f, 0.55f, 0.60f);
    drawCircle(0.0f, 0.0f, 6.0f, 16);

    // Upper arm
    if (isLeft) glColor3f(0.34f, 0.40f, 0.48f);
    else        glColor3f(0.22f, 0.26f, 0.32f);
    float armW = 7.0f, armH = 20.0f;
    drawRect(-armW * 0.5f, -armH, armW, armH);

    // Elbow
    glTranslatef(0.0f, -armH, 0.0f);
    drawCircle(0.0f, 0.0f, 4.5f, 14);

    float elbowAngle = (gameState == STATE_GAMEOVER) ? -60.0f : -35.0f;
    glRotatef(elbowAngle, 0.0f, 0.0f, 1.0f);

    // Forearm
    float forearmW = 6.0f, forearmH = 18.0f;
    drawRect(-forearmW * 0.5f, -forearmH, forearmW, forearmH);

    // Claw Gripper Hand
    glTranslatef(0.0f, -forearmH, 0.0f);
    if (isLeft) glColor3f(0.95f, 0.80f, 0.20f);
    else        glColor3f(0.70f, 0.55f, 0.15f);
    drawCircle(0.0f, 0.0f, 4.0f, 12);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(-3.0f, 0.0f);
    glVertex2f(-5.0f, -6.0f);
    glVertex2f(3.0f, 0.0f);
    glVertex2f(5.0f, -6.0f);
    glEnd();

    glPopMatrix();
}

void drawRobotTorso() {
    float torsoW = 46.0f, torsoH = 54.0f;

    glColor3f(0.26f, 0.32f, 0.39f);
    drawRoundedRect(-torsoW * 0.5f, 0.0f, torsoW, torsoH, 5.0f);

    glColor3f(0.18f, 0.22f, 0.28f);
    glLineWidth(2.0f);
    drawRectOutline(-torsoW * 0.5f, 0.0f, torsoW, torsoH);

    // Chest Reactor Core (Pulsing Glow)
    float pulse = 0.75f + 0.25f * sinf(runPhase * 5.0f);
    glColor3f(0.12f, 0.15f, 0.20f);
    drawCircle(0.0f, torsoH * 0.55f, 13.0f, 24);

    if (gameState == STATE_GAMEOVER) {
        // Red dead reactor core on crash
        glColor3f(0.8f, 0.1f, 0.1f);
    } else {
        glColor3f(0.0f * pulse, 0.95f * pulse, 1.0f * pulse);
    }
    drawCircle(0.0f, torsoH * 0.55f, 9.0f, 24);
    glColor3f(0.85f, 1.0f, 1.0f);
    drawCircle(0.0f, torsoH * 0.55f, 4.0f, 16);

    // Battery meter
    for (int i = 0; i < 3; ++i) {
        if (gameState == STATE_GAMEOVER) glColor3f(0.5f, 0.1f, 0.1f);
        else if (i == 0) glColor3f(0.2f, 0.9f, 0.3f);
        else if (i == 1) glColor3f(0.9f, 0.8f, 0.1f);
        else             glColor3f(0.0f, 0.85f, 0.95f);
        drawRect(-12.0f + (float)i * 9.0f, 9.0f, 6.0f, 4.0f);
    }
}

void drawRobotHead() {
    glColor3f(0.20f, 0.24f, 0.30f);
    drawRect(-6.0f, 54.0f, 12.0f, 7.0f); // Neck

    float headY = 61.0f;
    float headW = 38.0f, headH = 28.0f;

    glColor3f(0.32f, 0.38f, 0.46f);
    drawRoundedRect(-headW * 0.5f, headY, headW, headH, 4.0f);

    // Ear bolts
    glColor3f(0.16f, 0.20f, 0.26f);
    drawCircle(-headW * 0.5f - 2.0f, headY + headH * 0.5f, 4.5f, 14);
    drawCircle( headW * 0.5f + 2.0f, headY + headH * 0.5f, 4.5f, 14);

    // Digital Visor
    glColor3f(0.08f, 0.10f, 0.14f);
    drawRect(-headW * 0.5f + 4.0f, headY + 11.0f, headW - 8.0f, 11.0f);

    if (gameState == STATE_GAMEOVER) {
        // Red 'X' 'X' eyes upon game over crash!
        glColor3f(1.0f, 0.2f, 0.2f);
        glLineWidth(2.5f);
        glBegin(GL_LINES);
        // Left eye X
        glVertex2f(-10.0f, headY + 13.0f); glVertex2f(-4.0f, headY + 19.0f);
        glVertex2f(-4.0f,  headY + 13.0f); glVertex2f(-10.0f, headY + 19.0f);
        // Right eye X
        glVertex2f(4.0f,  headY + 13.0f); glVertex2f(10.0f, headY + 19.0f);
        glVertex2f(10.0f, headY + 13.0f); glVertex2f(4.0f,  headY + 19.0f);
        glEnd();
    } else {
        // Glowing Cyan Visor
        float glow = 0.85f + 0.15f * sinf(runPhase * 4.0f);
        glColor3f(0.0f, 0.95f * glow, 1.0f * glow);
        drawRect(-headW * 0.5f + 6.0f, headY + 13.0f, headW - 12.0f, 7.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawRect(-5.0f, headY + 15.5f, 10.0f, 2.0f);
    }

    // Antenna
    glColor3f(0.20f, 0.24f, 0.30f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, headY + headH);
    glVertex2f(0.0f, headY + headH + 14.0f);
    glEnd();

    // Antenna Beacon Bulb
    float blink = 0.5f + 0.5f * sinf(runPhase * 10.0f);
    if (gameState == STATE_GAMEOVER) {
        glColor3f(1.0f * blink, 0.1f, 0.1f);
    } else if (!isGrounded) {
        glColor3f(1.0f, 0.85f * blink, 0.1f); // Yellow in jump
    } else {
        glColor3f(0.1f, 0.95f * blink, 0.2f); // Green normal run
    }
    drawCircle(0.0f, headY + headH + 16.0f, 4.5f, 14);
}

void drawRobot() {
    glPushMatrix();
    glTranslatef(ROBOT_SCREEN_X, robotY, 0.0f);

    float heightAboveGround = robotY - GROUND_Y;
    drawShadow(heightAboveGround);

    // Natural running forward tilt & vertical bobbing
    float tiltAngle = (isGrounded && gameState == STATE_PLAYING) ? -8.0f : 0.0f;
    float bobOffset = (isGrounded && gameState == STATE_PLAYING) ? fabsf(sinf(runPhase * 2.0f)) * 4.0f : 0.0f;

    if (gameState == STATE_GAMEOVER) {
        // Fallen crash pose
        tiltAngle = -45.0f;
        bobOffset = -10.0f;
    }

    glTranslatef(0.0f, bobOffset, 0.0f);
    glRotatef(tiltAngle, 0.0f, 0.0f, 1.0f);

    // Calculate leg & arm swing angles
    float legMaxSwing = 36.0f;
    float armMaxSwing = 34.0f;

    float leftLegAngle  =  sinf(runPhase) * legMaxSwing;
    float rightLegAngle = -sinf(runPhase) * legMaxSwing;
    float leftArmAngle  = -sinf(runPhase) * armMaxSwing;
    float rightArmAngle =  sinf(runPhase) * armMaxSwing;

    if (!isGrounded) {
        // Jump pose: tuck knees, arms angled up
        leftLegAngle  = -25.0f;
        rightLegAngle =  20.0f;
        leftArmAngle  =  40.0f;
        rightArmAngle = -40.0f;
    } else if (gameState == STATE_START) {
        leftLegAngle = rightLegAngle = leftArmAngle = rightArmAngle = 0.0f;
    }

    // 1. Back Limbs
    glPushMatrix();
    glTranslatef(12.0f, 40.0f, 0.0f);
    drawRobotLeg(rightLegAngle, false);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(20.0f, 82.0f, 0.0f);
    drawRobotArm(rightArmAngle, false);
    glPopMatrix();

    // 2. Torso & Head
    glPushMatrix();
    glTranslatef(0.0f, 38.0f, 0.0f);
    drawRobotTorso();
    drawRobotHead();
    glPopMatrix();

    // 3. Front Limbs
    glPushMatrix();
    glTranslatef(-12.0f, 40.0f, 0.0f);
    drawRobotLeg(leftLegAngle, true);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-20.0f, 82.0f, 0.0f);
    drawRobotArm(leftArmAngle, true);
    glPopMatrix();

    glPopMatrix();
}

// ============================================================================
// PARTICLE RENDERING
// ============================================================================

void drawParticles() {
    glPointSize(3.5f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < particles.size(); ++i) {
        const Particle& p = particles[i];
        glColor4f(p.r, p.g, p.b, p.life);
        glVertex2f(p.x, p.y);
    }
    glEnd();
}

// ============================================================================
// HEADS-UP DISPLAY (HUD) & GAME OVER SCREENS
// ============================================================================

void drawHUD() {
    // Top Score Board (Retro Arcade / Chrome Dino Style)
    glColor4f(0.06f, 0.09f, 0.15f, 0.85f);
    drawRoundedRect(WINDOW_WIDTH - 380.0f, WINDOW_HEIGHT - 65.0f, 360.0f, 50.0f, 6.0f);
    glColor3f(0.0f, 0.85f, 0.95f);
    glLineWidth(1.5f);
    drawRectOutline(WINDOW_WIDTH - 380.0f, WINDOW_HEIGHT - 65.0f, 360.0f, 50.0f);

    std::stringstream ssScore;
    int curScore = (int)score;
    int bestScore = (int)highScore;
    ssScore << "HI: " << bestScore << "    SCORE: " << curScore;

    glColor3f(1.0f, 1.0f, 1.0f);
    renderBitmapText(WINDOW_WIDTH - 355.0f, WINDOW_HEIGHT - 35.0f, ssScore.str(), GLUT_BITMAP_HELVETICA_18);

    // Speed multiplier & Dodge counter
    std::stringstream ssMeta;
    ssMeta << "SPEED: " << (int)(gameSpeed * 10) / 10.0f << "x  |  MONSTERS DODGED: " << monstersDodged;
    glColor3f(0.2f, 0.95f, 0.45f);
    renderBitmapText(WINDOW_WIDTH - 355.0f, WINDOW_HEIGHT - 54.0f, ssMeta.str(), GLUT_BITMAP_HELVETICA_10);

    // Top Left Game Title Banner
    glColor4f(0.06f, 0.09f, 0.15f, 0.80f);
    drawRoundedRect(20.0f, WINDOW_HEIGHT - 65.0f, 330.0f, 50.0f, 6.0f);
    glColor3f(0.18f, 0.28f, 0.40f);
    drawRectOutline(20.0f, WINDOW_HEIGHT - 65.0f, 330.0f, 50.0f);

    glColor3f(0.0f, 0.95f, 1.0f);
    renderBitmapText(35.0f, WINDOW_HEIGHT - 36.0f, "CYBER-ROBOT RUNNER", GLUT_BITMAP_HELVETICA_18);
    glColor3f(0.70f, 0.78f, 0.88f);
    renderBitmapText(35.0f, WINDOW_HEIGHT - 54.0f, "Press [SPACE] or [UP] to Jump over Monsters!", GLUT_BITMAP_HELVETICA_10);

    // Bottom Controls Bar
    float barH = 30.0f;
    glColor4f(0.05f, 0.07f, 0.10f, 0.92f);
    drawRect(0.0f, 0.0f, (float)WINDOW_WIDTH, barH);
    glColor3f(0.0f, 0.80f, 0.90f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, barH);
    glVertex2f((float)WINDOW_WIDTH, barH);
    glEnd();

    glColor3f(0.85f, 0.90f, 0.98f);
    renderBitmapText(25.0f, 10.0f, "CONTROLS:  [SPACE / UP] Jump    [P] Pause    [R] Restart    [Esc] Exit", GLUT_BITMAP_HELVETICA_12);

    // ------------------------------------------------------------------------
    // POPUP MODALS FOR START, GAME OVER, AND PAUSE
    // ------------------------------------------------------------------------
    if (gameState == STATE_START) {
        // Dark translucent overlay
        glColor4f(0.04f, 0.06f, 0.10f, 0.75f);
        drawRect(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

        // Center Start Card
        float cw = 500.0f, ch = 240.0f;
        float cx = (WINDOW_WIDTH - cw) * 0.5f;
        float cy = (WINDOW_HEIGHT - ch) * 0.5f + 30.0f;

        glColor4f(0.08f, 0.12f, 0.18f, 0.95f);
        drawRoundedRect(cx, cy, cw, ch, 10.0f);
        glColor3f(0.0f, 0.90f, 0.95f);
        glLineWidth(2.5f);
        drawRectOutline(cx, cy, cw, ch);

        glColor3f(0.0f, 0.95f, 1.0f);
        renderBitmapText(cx + 80.0f, cy + 185.0f, "ROBOT MONSTER RUNNER", GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.85f, 0.90f, 0.95f);
        renderBitmapText(cx + 60.0f, cy + 145.0f, "Jump over approaching alien beasts and drones!", GLUT_BITMAP_HELVETICA_12);
        renderBitmapText(cx + 60.0f, cy + 120.0f, "Game accelerates as your score increases.", GLUT_BITMAP_HELVETICA_12);

        // Blinking Press Space prompt
        float blink = 0.5f + 0.5f * sinf(runPhase * 8.0f);
        glColor3f(1.0f * blink, 0.85f * blink, 0.2f);
        renderBitmapText(cx + 95.0f, cy + 60.0f, ">>> PRESS [SPACE] TO START <<<", GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.6f, 0.7f, 0.8f);
        renderBitmapText(cx + 140.0f, cy + 25.0f, "Press [P] to Pause at any time", GLUT_BITMAP_HELVETICA_10);
    }
    else if (gameState == STATE_GAMEOVER) {
        // Red-tinted crash overlay
        glColor4f(0.15f, 0.03f, 0.03f, 0.75f);
        drawRect(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

        float cw = 480.0f, ch = 250.0f;
        float cx = (WINDOW_WIDTH - cw) * 0.5f;
        float cy = (WINDOW_HEIGHT - ch) * 0.5f + 30.0f;

        glColor4f(0.14f, 0.08f, 0.10f, 0.95f);
        drawRoundedRect(cx, cy, cw, ch, 10.0f);
        glColor3f(1.0f, 0.25f, 0.25f);
        glLineWidth(3.0f);
        drawRectOutline(cx, cy, cw, ch);

        glColor3f(1.0f, 0.20f, 0.20f);
        renderBitmapText(cx + 170.0f, cy + 195.0f, "GAME OVER", GLUT_BITMAP_HELVETICA_18);

        std::stringstream ssFinal, ssBest;
        ssFinal << "FINAL SCORE : " << (int)score;
        ssBest  << "HIGH SCORE  : " << (int)highScore;

        glColor3f(1.0f, 1.0f, 1.0f);
        renderBitmapText(cx + 160.0f, cy + 145.0f, ssFinal.str(), GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.2f, 0.95f, 0.45f);
        renderBitmapText(cx + 160.0f, cy + 115.0f, ssBest.str(), GLUT_BITMAP_HELVETICA_18);

        float blink = 0.5f + 0.5f * sinf(runPhase * 8.0f);
        glColor3f(1.0f * blink, 0.9f * blink, 0.3f);
        renderBitmapText(cx + 90.0f, cy + 50.0f, ">>> PRESS [R] OR [SPACE] TO RETRY <<<", GLUT_BITMAP_HELVETICA_18);
    }
    else if (gameState == STATE_PAUSED) {
        glColor4f(0.04f, 0.06f, 0.10f, 0.65f);
        drawRect(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

        glColor3f(1.0f, 0.85f, 0.2f);
        renderBitmapText((float)WINDOW_WIDTH * 0.5f - 80.0f, (float)WINDOW_HEIGHT * 0.5f + 20.0f, "PAUSED", GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.8f, 0.85f, 0.9f);
        renderBitmapText((float)WINDOW_WIDTH * 0.5f - 130.0f, (float)WINDOW_HEIGHT * 0.5f - 15.0f, "Press [P] to Resume Playing", GLUT_BITMAP_HELVETICA_12);
    }
}

// ============================================================================
// GAME INITIALIZATION & SPAWNING
// ============================================================================

void spawnNewObstacle() {
    Obstacle obs;
    obs.x = (float)WINDOW_WIDTH + 50.0f + ((float)rand() / (float)RAND_MAX * 120.0f);
    obs.animTimer = 0.0f;
    obs.passed = false;

    // Pick obstacle type with balanced difficulty
    int pick = rand() % 10;
    if (pick < 4) {
        // Ground Chomping Alien Beast
        obs.type = OBS_MONSTER_GROUND;
        obs.y = GROUND_Y;
        obs.width = 48.0f;
        obs.height = 46.0f;
    } else if (pick < 6) {
        // Low Flying Drone (must jump with careful timing!)
        obs.type = OBS_FLYING_DRONE;
        obs.y = GROUND_Y + 45.0f; // Flying elevated off the ground
        obs.width = 46.0f;
        obs.height = 34.0f;
    } else if (pick < 8) {
        // Ground Spikes
        obs.type = OBS_SPIKES;
        obs.y = GROUND_Y;
        obs.width = 46.0f;
        obs.height = 36.0f;
    } else {
        // High-Tech Crate
        obs.type = OBS_CRATE;
        obs.y = GROUND_Y;
        obs.width = 44.0f;
        obs.height = 42.0f;
    }

    obstacles.push_back(obs);
}

void resetGame() {
    robotY         = GROUND_Y;
    robotVy        = 0.0f;
    isGrounded     = true;
    runPhase       = 0.0f;
    gameSpeed      = baseSpeed;
    score          = 0.0f;
    monstersDodged = 0;
    screenShake    = 0.0f;
    spawnCooldown  = 60.0f; // Initial calm period before first monster

    obstacles.clear();
    particles.clear();
    gameState = STATE_PLAYING;
}

void initGameEnvironment() {
    srand((unsigned int)time(NULL));

    // Initialize Stars
    stars.clear();
    for (int i = 0; i < 50; ++i) {
        Star s;
        s.x = (float)(rand() % WINDOW_WIDTH);
        s.y = GROUND_Y + 100.0f + (float)(rand() % (WINDOW_HEIGHT - (int)GROUND_Y - 120));
        s.brightness = 0.5f + 0.5f * ((float)rand() / (float)RAND_MAX);
        stars.push_back(s);
    }

    // Initialize Clouds
    clouds.clear();
    Cloud c1 = { 150.0f, 490.0f, 0.45f, 1.1f };
    Cloud c2 = { 500.0f, 450.0f, 0.35f, 0.9f };
    Cloud c3 = { 850.0f, 510.0f, 0.55f, 1.3f };
    clouds.push_back(c1);
    clouds.push_back(c2);
    clouds.push_back(c3);
}

// ============================================================================
// COLLISION DETECTION (AABB WITH GENEROUS HITBOXES)
// ============================================================================

bool checkCollision(const Obstacle& obs) {
    // Robot Hitbox (Center-based, slightly indented for player fairness)
    float rw = 26.0f;
    float rh = 62.0f;
    float rx = ROBOT_SCREEN_X - rw * 0.5f;
    float ry = robotY + 4.0f;

    // Obstacle Hitbox
    float ow = obs.width * 0.85f;
    float oh = obs.height * 0.85f;
    float ox = obs.x + (obs.width - ow) * 0.5f;
    float oy = obs.y;

    // Axis-Aligned Bounding Box (AABB) Overlap Test
    bool overlapX = (rx < ox + ow) && (rx + rw > ox);
    bool overlapY = (ry < oy + oh) && (ry + rh > oy);

    return overlapX && overlapY;
}

// ============================================================================
// JUMP MECHANIC
// ============================================================================

void doJump() {
    if (gameState == STATE_START) {
        resetGame();
        return;
    }
    if (gameState == STATE_GAMEOVER) {
        resetGame();
        return;
    }
    if (gameState != STATE_PLAYING) return;

    if (isGrounded) {
        robotVy = JUMP_VELOCITY;
        isGrounded = false;
        spawnJumpDust(ROBOT_SCREEN_X, GROUND_Y);
    }
}

// ============================================================================
// ANIMATION & PHYSICS UPDATE (TIMER CALLBACK - 60 FPS)
// ============================================================================

void update(int value) {
    if (gameState == STATE_PLAYING) {
        // 1. Jump Physics Integration (Gravitational Acceleration)
        if (!isGrounded || robotY > GROUND_Y) {
            robotY += robotVy;
            robotVy -= GRAVITY;

            if (robotY <= GROUND_Y) {
                robotY = GROUND_Y;
                robotVy = 0.0f;
                isGrounded = true;
                spawnJumpDust(ROBOT_SCREEN_X, GROUND_Y); // Landing dust
            }
        }

        // 2. Advance Run Phase proportional to speed
        runPhase += 0.035f * gameSpeed;
        if (runPhase > 2.0f * (float)M_PI) {
            runPhase -= 2.0f * (float)M_PI;
        }

        // 3. Parallax Background Scrolling
        bgOffsetFar  += gameSpeed * 0.15f;
        bgOffsetMid  += gameSpeed * 0.40f;
        groundOffset += gameSpeed;

        // 4. Update Score & Gradual Speed Escalation
        score += 0.15f * (gameSpeed / baseSpeed);
        if (score > highScore) {
            highScore = score;
        }

        // Gradually increase game speed every 100 points (Max 13.0f)
        gameSpeed = baseSpeed + (score / 150.0f);
        if (gameSpeed > 13.0f) gameSpeed = 13.0f;

        // 5. Update Obstacles (Scroll left & check collisions)
        for (size_t i = 0; i < obstacles.size(); ++i) {
            Obstacle& obs = obstacles[i];
            obs.x -= gameSpeed;
            obs.animTimer += 0.04f;

            // Check if player jumped over and passed obstacle
            if (!obs.passed && obs.x + obs.width < ROBOT_SCREEN_X) {
                obs.passed = true;
                monstersDodged++;
            }

            // Check Collision
            if (checkCollision(obs)) {
                gameState = STATE_GAMEOVER;
                screenShake = 15.0f;
                spawnCrashExplosion(ROBOT_SCREEN_X, robotY + 35.0f);
                std::cout << "[GAME OVER] Collided with obstacle! Final Score: " << (int)score << std::endl;
                break;
            }
        }

        // Remove off-screen obstacles
        for (size_t i = 0; i < obstacles.size();) {
            if (obstacles[i].x < -100.0f) {
                obstacles.erase(obstacles.begin() + i);
            } else {
                ++i;
            }
        }

        // 6. Obstacle Spawner Timer
        spawnCooldown -= 1.0f;
        if (spawnCooldown <= 0.0f) {
            spawnNewObstacle();
            // Variable spawn interval based on current speed
            float minGap = 65.0f - (gameSpeed - baseSpeed) * 2.0f;
            if (minGap < 35.0f) minGap = 35.0f;
            spawnCooldown = minGap + ((float)rand() / (float)RAND_MAX * 55.0f);
        }

        // 7. Drifting Clouds
        for (size_t i = 0; i < clouds.size(); ++i) {
            clouds[i].x -= clouds[i].speed;
            if (clouds[i].x < -100.0f) {
                clouds[i].x = (float)WINDOW_WIDTH + 100.0f;
            }
        }
    } else {
        // Run phase still animates UI elements in start / gameover screen
        runPhase += 0.05f;
    }

    // 8. Update Particles
    for (size_t i = 0; i < particles.size();) {
        Particle& p = particles[i];
        p.x += p.vx;
        p.y += p.vy;
        p.vy -= 0.12f; // Particle gravity
        p.life -= p.decay;
        if (p.life <= 0.0f || p.y < GROUND_Y) {
            particles.erase(particles.begin() + i);
        } else {
            ++i;
        }
    }

    // Decay screen shake
    if (screenShake > 0.1f) {
        screenShake *= 0.88f;
    } else {
        screenShake = 0.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // ~60 FPS
}

// ============================================================================
// GLUT DISPLAY & RESHAPE CALLBACKS
// ============================================================================

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Apply Screen Shake if crashing
    if (screenShake > 0.0f) {
        float shakeX = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * screenShake;
        float shakeY = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * screenShake;
        glTranslatef(shakeX, shakeY, 0.0f);
    }

    // 1. Draw Parallax Background
    drawSky();
    drawMountains();
    drawClouds();
    drawGround();

    // 2. Draw Obstacles & Monsters
    drawObstacles();

    // 3. Draw Robot
    drawRobot();

    // 4. Draw Particles (Jump dust & crash sparks)
    drawParticles();

    // 5. Draw Heads-Up Display Telemetry & Overlays
    drawHUD();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)WINDOW_WIDTH, 0.0, (GLdouble)WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// ============================================================================
// KEYBOARD CONTROLS
// ============================================================================

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case ' ': // Spacebar: Jump or Start/Retry
            doJump();
            break;

        case 'w':
        case 'W': // W key alternative jump
            doJump();
            break;

        case 'p':
        case 'P': // Pause / Resume
            if (gameState == STATE_PLAYING) {
                gameState = STATE_PAUSED;
                std::cout << "[GAME] Paused." << std::endl;
            } else if (gameState == STATE_PAUSED) {
                gameState = STATE_PLAYING;
                std::cout << "[GAME] Resumed." << std::endl;
            }
            break;

        case 'r':
        case 'R': // Instant Restart
            resetGame();
            std::cout << "[GAME] Restarted." << std::endl;
            break;

        case 27: // Escape key
            std::cout << "[GAME] Exiting. Thanks for playing!" << std::endl;
            exit(0);
            break;

        default:
            break;
    }
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: // Up arrow key jumps
            doJump();
            break;
        default:
            break;
    }
}

// ============================================================================
// INITIALIZATION & MAIN ENTRY POINT
// ============================================================================

void initGL() {
    glClearColor(0.04f, 0.05f, 0.12f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    initGameEnvironment();
}

int main(int argc, char** argv) {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  CYBER-ROBOT RUNNER (CHROME DINO STYLE GAME)            " << std::endl;
    std::cout << "  Computer Graphics Mini Project (C++ & OpenGL / GLUT)    " << std::endl;
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Controls:                                               " << std::endl;
    std::cout << "    [SPACE] / [UP] / [W] : JUMP over monsters & hazards   " << std::endl;
    std::cout << "    [P]                  : Pause / Resume                 " << std::endl;
    std::cout << "    [R]                  : Restart game                   " << std::endl;
    std::cout << "    [Esc]                : Exit                           " << std::endl;
    std::cout << "==========================================================" << std::endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(150, 80);
    glutCreateWindow("Cyber-Robot Runner (Chrome Dino Monster Game) - OpenGL");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, update, 0);

    glutMainLoop();

    return 0;
}
