#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <cmath>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>

// Camera position
GLfloat camX = 0.0f, camY = 20.0f, camZ = 60.0f;
GLfloat camLookX = 0.0f, camLookY = 0.0f, camLookZ = 0.0f;
enum ViewMode { TOP_VIEW, SIDE_VIEW, FRONT_VIEW };
ViewMode currentView = FRONT_VIEW;
bool animate = false;
float translationOffset = 0.0f;
float rotationAngle = 0.0f;
float scaleAmount = 1.0f;
float colorIntensity = 1.0f;
float colorDirection = 1.0f;
struct Player {
    GLfloat x, y, z;  // Player's position
    GLfloat size;
    GLfloat rotationAngle;
};
const int totalPucks = 5;  // Total number of pucks
int collectedPucks = 0;  // Track number of collected pucks
float timeRemaining = 90.0f;  // Total time in seconds for the game
bool gameWon = false;  // Track if the game is won
bool gameLost = false;
bool gameOver = false;

struct Puck {
    GLfloat x, y, z;  // Position
    GLfloat size;     // Radius (for collision)
    bool active;      // Whether the puck is active (visible or not)
    float animationPhase;
};
Player player = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f};  // Example: player at origin
Puck pucks[5];

void initializePucks() {
    srand(static_cast<unsigned int>(time(0)));  // Seed random number generator
    
    // Define the spawn boundaries within the rink's dimensions
    const float rinkMargin = 2.0f;  // Small margin to avoid spawning too close to edges
    const float xMin = -15.0f + rinkMargin;  // Rink's left boundary + margin
    const float xMax = 15.0f - rinkMargin;   // Rink's right boundary - margin
    const float zMin = -30.0f + rinkMargin;  // Rink's back boundary + margin
    const float zMax = 30.0f - rinkMargin;   // Rink's front boundary - margin
    
    for (int i = 0; i < 5; ++i) {
        pucks[i].x = xMin + static_cast<float>(rand()) / RAND_MAX * (xMax - xMin);
        pucks[i].z = zMin + static_cast<float>(rand()) / RAND_MAX * (zMax - zMin);
        pucks[i].y = 0.05f;        // Slightly above the ground to avoid z-fighting
        pucks[i].size = 0.6f;      // Set size of the puck
        pucks[i].active = true;    // Puck is initially active
        pucks[i].animationPhase = static_cast<float>(rand() % 360);
    }
    player.x = 0.0f;
    player.y = 0.0f;  // Player is on the ground
    player.z = 0.0f;
    player.size = 1.0f;
}

float leftWallBoundary = -15.5f + 1.0f;   // Right edge of the left wall
float rightWallBoundary = 15.5f - 1.0f;   // Left edge of the right wall
float backWallBoundary = -30.0f + 0.5f;   // Front edge of the back wall
float frontWallBoundary = 30.0f - 1.0f;

void toggleAnimation() {
    animate = !animate;
}
void updateAnimation() {
    if (animate) {
        // Update translation offset
        translationOffset = sin(glutGet(GLUT_ELAPSED_TIME) * 0.001f) * 2.0f;

        // Update rotation angle
        rotationAngle += 0.5f;
        if (rotationAngle > 360.0f) rotationAngle -= 360.0f;

        // Update scaling factor
        scaleAmount = 1.0f + 0.5f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.001f);

        // Update color intensity
        colorIntensity += colorDirection * 0.01f;
        if (colorIntensity > 1.0f || colorIntensity < 0.5f) colorDirection *= -1;
    }
    glutPostRedisplay();
}

void setCameraView() {


    if (gameOver) {
        // Set orthogonal projection when rendering the game over screen
        gluOrtho2D(0, 800, 600, 0);  // Ensure this is for 2D rendering
    } else {
        // Set perspective projection for normal views
        switch (currentView) {
            case TOP_VIEW:
                gluLookAt(0.0f, 80.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f); // Looking down from above
                break;
            case SIDE_VIEW:
                gluLookAt(80.0f, 10.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 1.0f, 0.0f); // Looking from the side
                break;
            case FRONT_VIEW:
                gluLookAt(camX, camY, camZ, camLookX, camLookY, camLookZ, 0.0f, 1.0f, 0.0f); // Custom camera
                break;
        }
    }

}

void drawCuboid(GLfloat width, GLfloat height, GLfloat depth) {
    glPushMatrix();
    glScalef(width, height, depth);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawCylinder(GLfloat radius, GLfloat height, GLint slices, GLint stacks) {
    GLUquadricObj *quadric = gluNewQuadric();
    gluCylinder(quadric, radius, radius, height, slices, stacks);
    gluDeleteQuadric(quadric);
}


void drawPuck(Puck& puck) {
    glColor3f(0.2f, 0.2f, 0.2f);  // Dark gray color for the puck
    GLUquadricObj *quadric = gluNewQuadric();

    glPushMatrix();
    glTranslatef(puck.x, puck.y, puck.z);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    // 1. Bottom face (disk)
    gluDisk(quadric, 0.0, puck.size, 30, 1);

    // 2. Cylindrical body
    gluCylinder(quadric, puck.size, puck.size, 0.2f, 30, 1);

    // 3. Top face (disk)
    glTranslatef(0.0f, 0.0f, 0.2f);  // Move to the top of the cylinder
    gluDisk(quadric, 0.0, puck.size, 30, 1);

    glPopMatrix();
    gluDeleteQuadric(quadric);
}

void drawRink() {
    glColor3f(0.8f, 0.9f, 1.0f);  // Light blue for ice
    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);
    drawCuboid(30.0f, 0.2f, 60.0f);
    glPopMatrix();
}
void drawWalls() {
    // Apply color animation
    glColor3f(0.8f * colorIntensity, 0.8f * colorIntensity, 0.8f);

    // Left wall (split into 2 parts)
    glPushMatrix();
    glTranslatef(-15.5f + translationOffset, 1.0f, 0.0f); // Apply translation animation
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);           // Apply rotation animation
    glScalef(scaleAmount, 1.0f, 1.0f);                    // Apply scaling animation
    drawCuboid(0.5f, 1.0f, 60.0f);                        // Upper part
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-15.5f + translationOffset, -1.0f, 0.0f);
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
    glScalef(scaleAmount, 1.0f, 1.0f);
    drawCuboid(0.5f, 1.0f, 60.0f);                        // Lower part
    glPopMatrix();

    // Right wall (split into 2 parts)
    glPushMatrix();
    glTranslatef(15.5f - translationOffset, 1.0f, 0.0f);
    glRotatef(-rotationAngle, 0.0f, 1.0f, 0.0f);          // Inverse rotation for variety
    glScalef(scaleAmount, 1.0f, 1.0f);
    drawCuboid(0.5f, 1.0f, 60.0f);                        // Upper part
    glPopMatrix();

    glPushMatrix();
    glTranslatef(15.5f - translationOffset, -1.0f, 0.0f);
    glRotatef(-rotationAngle, 0.0f, 1.0f, 0.0f);
    glScalef(scaleAmount, 1.0f, 1.0f);
    drawCuboid(0.5f, 1.0f, 60.0f);                        // Lower part
    glPopMatrix();

    // Back wall (split into 2 parts)
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, -30.0f + translationOffset); // Translation along Z-axis
    glRotatef(rotationAngle, 1.0f, 0.0f, 0.0f);           // Rotate around X-axis
    glScalef(1.0f, scaleAmount, 1.0f);                    // Scale vertically
    drawCuboid(30.0f, 1.0f, 0.5f);                        // Upper part
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -1.0f, -30.0f + translationOffset);
    glRotatef(rotationAngle, 1.0f, 0.0f, 0.0f);
    glScalef(1.0f, scaleAmount, 1.0f);
    drawCuboid(30.0f, 1.0f, 0.5f);                        // Lower part
    glPopMatrix();

    // Front wall (split into 2 parts)
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 30.0f - translationOffset);
    glRotatef(-rotationAngle, 1.0f, 0.0f, 0.0f);          // Inverse rotation
    glScalef(1.0f, scaleAmount, 1.0f);
    drawCuboid(30.0f, 1.0f, 0.5f);                        // Upper part
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 30.0f - translationOffset);
    glRotatef(-rotationAngle, 1.0f, 0.0f, 0.0f);
    glScalef(1.0f, scaleAmount, 1.0f);
    drawCuboid(30.0f, 1.0f, 0.5f);                        // Lower part
    glPopMatrix();
}

void drawIceMarkings() {
    // Red color for markings with animated color intensity
    glColor3f(1.0f * colorIntensity, 0.0f, 0.0f);
    
    // Center line with translation, rotation, and scaling
    glPushMatrix();
    glTranslatef(0.0f + translationOffset, 0.01f, 0.0f);  // Translation along X
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);           // Rotate along Y-axis
    glScalef(scaleAmount, 1.0f, 1.0f);                    // Scale along X-axis
    drawCuboid(30.0f, 0.02f, 0.3f);
    glPopMatrix();
    
    // Blue lines with color intensity, translation, and rotation
    glColor3f(0.0f, 0.0f, 1.0f * colorIntensity);  // Blue with animated intensity
    glPushMatrix();
    glTranslatef(0.0f, 0.01f, 15.0f + translationOffset);  // Translation along Z
    glRotatef(-rotationAngle, 1.0f, 0.0f, 0.0f);          // Rotate along X-axis
    glScalef(1.0f, scaleAmount, 1.0f);                    // Scale along Y-axis
    drawCuboid(30.0f, 0.02f, 0.3f);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.0f, 0.01f, -15.0f + translationOffset);
    glRotatef(-rotationAngle, 1.0f, 0.0f, 0.0f);
    glScalef(1.0f, scaleAmount, 1.0f);
    drawCuboid(30.0f, 0.02f, 0.3f);
    glPopMatrix();
    
    // Face-off circles with translation, rotation, and color changes
    glColor3f(1.0f * colorIntensity, 0.0f, 0.0f);  // Red with animated intensity
    for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
            glPushMatrix();
            glTranslatef(i * (10.0f + translationOffset), 0.01f, j * (20.0f + translationOffset)); // Translate both X and Z axes
            glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f); // Rotate along Y-axis
            glScalef(scaleAmount, scaleAmount, scaleAmount); // Uniform scaling
            glutSolidTorus(0.1f * scaleAmount, 3.0f * scaleAmount, 50, 50);
            glPopMatrix();
        }
    }
}


void drawNet(GLfloat xOffset, GLfloat yOffset, GLfloat zOffset) {
    glColor3f(1.0f, 1.0f, 1.0f);  // White for the net

    glPushMatrix();
    glTranslatef(xOffset, yOffset, zOffset);

    // Draw vertical lines for the net
    for (float x = -2.5f; x <= 2.5f; x += 0.5f) {
        glBegin(GL_LINES);
        glVertex3f(x, 0.0f, -0.1f);
        glVertex3f(x, 2.0f, -2.0f);
        glEnd();
    }

    // Draw horizontal lines for the net
    for (float y = 0.0f; y <= 2.0f; y += 0.5f) {
        glBegin(GL_LINES);
        glVertex3f(-2.5f, y, -0.1f);
        glVertex3f(2.5f, y, -2.0f);
        glEnd();
    }

    glPopMatrix();
}

void drawHockeyStick(float x, float y, float z, float angle) {
    GLUquadricObj *quadric = gluNewQuadric();
    
    // Apply animated translation, rotation, and scaling for the position and appearance of the stick
    glPushMatrix();
    glTranslatef(x + translationOffset, y, z);           // Apply translation animation
    glRotatef(angle + rotationAngle, 0.0f, 1.0f, 0.0f); // Apply rotation animation along Y-axis
    glScalef(scaleAmount, scaleAmount, scaleAmount);      // Apply scaling to adjust the size of the stick

    // Draw the stick shaft (with a slight variation in color intensity over time)
    glColor3f(0.6f, 0.3f, 0.1f * colorIntensity);  // Brown color with animated intensity for the shaft
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  // Rotate cylinder to stand along Y-axis
    gluCylinder(quadric, 0.1f, 0.1f, 4.0f, 16, 1);  // Shaft height set to 4.0
    glPopMatrix();

    // Draw the enlarged blade horizontally at the top of the shaft (with scaling and rotation animations)
    glColor3f(0.1f, 0.1f, 0.1f * colorIntensity);  // Dark color with animated intensity for the blade
    glPushMatrix();
    glTranslatef(0.0f, 4.0f, 0.3f);  // Move blade to the top of the shaft
    glRotatef(90.0f + rotationAngle, 1.0f, 0.0f, 0.0f);  // Rotate blade to be horizontal
    
    // Draw the main blade with increased width and length (animated scaling)
    glBegin(GL_QUADS);
    glVertex3f(-0.5f * scaleAmount, 0.0f, 0.0f);
    glVertex3f(0.5f * scaleAmount, 0.0f, 0.0f);
    glVertex3f(0.5f * scaleAmount, 0.0f, 0.2f);
    glVertex3f(-0.5f * scaleAmount, 0.0f, 0.2f);
    glEnd();

    // Draw an extended face on the blade for added prominence (animated scaling)
    glBegin(GL_QUADS);
    glVertex3f(-0.5f * scaleAmount, 0.0f, 0.0f);
    glVertex3f(-0.5f * scaleAmount, 0.2f, 0.0f);
    glVertex3f(-0.5f * scaleAmount, 0.2f, 0.2f);
    glVertex3f(-0.5f * scaleAmount, 0.0f, 0.2f);

    glVertex3f(0.5f * scaleAmount, 0.0f, 0.0f);
    glVertex3f(0.5f * scaleAmount, 0.2f, 0.0f);
    glVertex3f(0.5f * scaleAmount, 0.2f, 0.2f);
    glVertex3f(0.5f * scaleAmount, 0.0f, 0.2f);

    glVertex3f(-0.5f * scaleAmount, 0.2f, 0.0f);
    glVertex3f(0.5f * scaleAmount, 0.2f, 0.0f);
    glVertex3f(0.5f * scaleAmount, 0.2f, 0.2f);
    glVertex3f(-0.5f * scaleAmount, 0.2f, 0.2f);
    glEnd();

    glPopMatrix();
    glPopMatrix();

    gluDeleteQuadric(quadric);
}

void drawHockeySticks() {            // Stick at origin
    drawHockeyStick(-10.0f + translationOffset, 0.1f, -15.0f, 20.0f + rotationAngle);  // Additional stick with animated position
    drawHockeyStick(12.0f + translationOffset, 0.1f, 18.0f, -30.0f - rotationAngle);
    drawHockeyStick(-15.0f + translationOffset, 0.1f, 10.0f, 45.0f + rotationAngle);
}



void drawGoals() {
    glColor3f(colorIntensity, 0.0f, 0.0f);   // Red for goal frame
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, -29.0f + translationOffset);
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);           // Rotation
    glScalef(scaleAmount, 2.0f, 2.0f);
    drawCuboid(6.0f, 2.0f, 2.0f);  // Main frame
    drawNet(0.0f, -1.0f, 3.0f);  // Draw net in the back of the goal
    glPopMatrix();

    // Left and right posts and crossbar for the second goal
    glColor3f(colorIntensity, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 29.0f);
    drawCuboid(6.0f, 2.0f, 2.0f);  // Main frame
    drawNet(0.0f, -1.0f, 3.0f);  // Draw net in the back of the goal
    glPopMatrix();
}

// Function to draw a bench
void drawBench() {
    // Seat
    glColor3f(0.6f*colorIntensity, 0.4f*colorIntensity, 0.2f);  // Brown for the seat
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.0f);
    drawCuboid(3.0f, 0.2f, 1.0f);
    glPopMatrix();

    // Backrest
    glColor3f(0.6f*colorIntensity, 0.4f*colorIntensity, 0.2f);  // Brown for the backrest
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, -0.4f);
    drawCuboid(3.0f, 0.5f, 0.2f);
    glPopMatrix();

    // Supports
    glColor3f(0.3f*colorIntensity, 0.2f*colorIntensity, 0.1f);  // Darker color for the supports
    glPushMatrix();
    glTranslatef(-1.4f, 0.25f, 0.0f);
    drawCuboid(0.2f, 0.5f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.4f, 0.25f, 0.0f);
    drawCuboid(0.2f, 0.5f, 1.0f);
    glPopMatrix();
}
// Function to draw benches around the rink
void drawSeatedPerson() {
    // Draw head
    glColor3f(1.0f, 0.8f, 0.6f*colorIntensity);  // Skin color for head
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f+translationOffset);  // Position head on top of body
    glutSolidSphere(0.3f, 10, 10);  // Small sphere for head
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);           // Rotation
    glScalef(scaleAmount, 2.0f, 2.0f);
    glPopMatrix();

    // Draw torso (shorter to simulate seated position)
    glColor3f(0.0f, 0.0f, 1.0f*colorIntensity);  // Blue for shirt
    glPushMatrix();
    glTranslatef(0.0f, 0.8f, 0.0f+translationOffset);  // Position torso below head
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
    glScalef(0.4f+scaleAmount, 0.6f, 0.3f);  // Scale for a shorter torso
    glutSolidCube(1.0f);  // Cuboid for torso
    glPopMatrix();

    // Draw upper legs (angled to show sitting)
    glColor3f(0.3f, 0.3f, 0.3f*colorIntensity);  // Dark color for pants
    glPushMatrix();
    glTranslatef(-0.2f, 0.4f, -0.3f+translationOffset);  // Left upper leg position
    glRotatef(60.0f, 1.0f, 0.0f, 0.0f+rotationAngle);  // Rotate to simulate bent leg
    glScalef(0.15f+scaleAmount, 0.4f, 0.15f);
    glutSolidCube(1.0f);  // Left upper leg
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, 0.4f, -0.3f+translationOffset);  // Right upper leg position
    glRotatef(60.0f, 1.0f, 0.0f, 0.0f+rotationAngle);  // Rotate to simulate bent leg
    glScalef(0.15f+scaleAmount, 0.4f, 0.15f);
    glutSolidCube(1.0f);  // Right upper leg
    glPopMatrix();

    // Draw lower legs (pointing down)
    glPushMatrix();
    glTranslatef(-0.2f, 0.1f, -0.7f+translationOffset);  // Left lower leg position
    glScalef(0.15f+scaleAmount, 0.4f, 0.15f);
    glutSolidCube(1.0f);  // Left lower leg
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, 0.1f, -0.7f+translationOffset);  // Right lower leg position
    glScalef(0.15f+scaleAmount, 0.4f, 0.15f);
    glutSolidCube(1.0f);  // Right lower leg
    glPopMatrix();
}

void drawCrowdBenches() {

    for (float z = -25.0f; z <= 25.0f; z += 6.0f) {
        glPushMatrix();
        glTranslatef(-18.0f+translationOffset, 0.0f, z);  // Place to the left of the rink
        glRotatef(90.0f+rotationAngle, 0.0f, 1.0f, 0.0f);  // Rotate person to face the rink
        glScalef(scaleAmount, 1.0f, 1.0f);
        drawBench();
        glTranslatef(0.0f, 0.5f, 0.5f);
        drawSeatedPerson();
        glPopMatrix();
    }

    for (float z = -25.0f; z <= 25.0f; z += 6.0f) {
        glPushMatrix();
        glTranslatef(18.0f+translationOffset, 0.0f, z);  // Place to the right of the rink
        glRotatef(-90.0f+rotationAngle, 0.0f, 1.0f, 0.0f);  // Rotate person to face the rink
        glScalef(scaleAmount, 1.0f, 1.0f);
        drawBench();
        glTranslatef(0.0f, 0.5f, -0.5f);  // Position person on the bench
        drawSeatedPerson();
        glPopMatrix();
    }

    // Back side benches with seated people facing the rink
    for (float x = -12.0f; x <= 12.0f; x += 6.0f) {
        glPushMatrix();
        glTranslatef(x+translationOffset, 0.0f, -32.0f);  // Place at the back of the rink
        glRotatef(180.0f+rotationAngle, 0.0f, 1.0f, 0.0f);  // Rotate person to face the rink
        glScalef(scaleAmount, 1.0f, 1.0f);
        drawBench();
        glTranslatef(0.0f, 0.5f, 0.5f);  // Position person on the bench
        drawSeatedPerson();
        glPopMatrix();
    }

    // Front side benches with seated people facing the rink
    for (float x = -12.0f; x <= 12.0f; x += 6.0f) {
        glPushMatrix();
        glTranslatef(x+translationOffset, 0.0f, 32.0f);  // Place at the front of the rink
        glRotatef(0.0f,rotationAngle, 0.0f, 0.0f);  // Rotate person to face the rink
        glScalef(scaleAmount, 1.0f, 1.0f);
        drawBench();
        glTranslatef(0.0f, 0.5f, -0.5f);  // Position person on the bench
        drawSeatedPerson();
        glPopMatrix();
    }
}




// Function to draw a player
void drawPlayer() {
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);  // Position the player
    glRotatef(player.rotationAngle, 0.0f, 1.0f, 0.0f);

    // Torso
    glColor3f(1.0f, 0.0f, 0.0f);  // Red for jersey
    glPushMatrix();
    drawCuboid(1.0f, 1.0f, 0.5f);  // Torso
    glPopMatrix();

    // Head
    glColor3f(0.8f, 0.6f, 0.4f);  // Skin tone
    glPushMatrix();
    glTranslatef(0.0f, 0.8f, 0.0f);  // Adjusted translation to connect to torso
    glutSolidSphere(0.3f, 20, 20);  // Head as a sphere
    glPopMatrix();

    // Upper Arms
    glColor3f(1.0f, 0.0f, 0.0f);  // Red for upper arms
    glPushMatrix();
    glTranslatef(0.7f, 0.5f, 0.0f);
    glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
    drawCuboid(0.2f, 0.5f, 0.2f);  // Right upper arm
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.7f, 0.5f, 0.0f);
    glRotatef(-45.0f, 0.0f, 0.0f, 1.0f);
    drawCuboid(0.2f, 0.5f, 0.2f);  // Left upper arm
    glPopMatrix();

    // Lower Arms
    glColor3f(0.8f, 0.6f, 0.4f);  // Skin tone for forearms
    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 0.0f);
    glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
    drawCuboid(0.15f, 0.5f, 0.15f);  // Right forearm
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.0f, 0.0f, 0.0f);
    glRotatef(-45.0f, 0.0f, 0.0f, 1.0f);
    drawCuboid(0.15f, 0.5f, 0.15f);  // Left forearm
    glPopMatrix();

    // Upper Legs
    glColor3f(0.0f, 0.0f, 0.0f);  // Black for pants
    glPushMatrix();
    glTranslatef(0.3f, -1.0f, 0.0f);
    drawCuboid(0.3f, 0.5f, 0.3f);  // Right upper leg
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.3f, -1.0f, 0.0f);
    drawCuboid(0.3f, 0.5f, 0.3f);  // Left upper leg
    glPopMatrix();

    // Lower Legs
    glColor3f(0.8f, 0.8f, 0.8f);  // Gray for lower legs
    glPushMatrix();
    glTranslatef(0.3f, -1.8f, 0.0f);
    drawCuboid(0.25f, 0.5f, 0.25f);  // Right lower leg
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.3f, -1.8f, 0.0f);
    drawCuboid(0.25f, 0.5f, 0.25f);  // Left lower leg
    glPopMatrix();

    glPopMatrix();  // End player model

}


void drawCeiling() {
    glColor3f(0.7f, 0.7f, 0.7f);  // Light gray for ceiling
    glPushMatrix();
    glTranslatef(0.0f, 15.0f, 0.0f);
    drawCuboid(50.0f, 0.5f, 80.0f);
    glPopMatrix();
}

void drawWall(float width, float height, float depth) {
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float red = (sin(time) + 1.0f) / 2.0f;  // Generate a value between 0 and 1 for the red component
    float green = (sin(time + 2.0f) + 1.0f) / 2.0f;  // Slight phase shift for green
    float blue = (sin(time + 4.0f) + 1.0f) / 2.0f;  // Slight phase shift for blue

    // Set the color based on time
    glColor3f(red, green, blue);

    // Create a large cuboid to represent a wall
    glPushMatrix();
    glScalef(width, height, depth);  // Scale the cuboid to create a wall
    drawCuboid(1.0f, 1.0f, 1.0f);  // Draw the wall with the changing color
    glPopMatrix();
}


void drawFloor(float width, float height, float depth) {
    glColor3f(0.0f, 0.5f, 1.0f);  // Bright blue for floor
    glPushMatrix();
    glTranslatef(0.0f, -0.2f, 0.0f);  // Slightly lower position
    drawCuboid(50.0f, 0.2f, 70.0f);  // Larger floor size (bigger than rink)
    glPopMatrix();
}


void drawRoomWithFloor() {
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, 10.0f, -35.0f);  // Position the back wall
    drawWall(50.0f, 20.0f, 1.0f);  // Width of 50, height of 20, depth of 1
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-25.0f, 10.0f, 0.0f);  // Position the left wall
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);  // Rotate to face the left
    drawWall(50.0f, 20.0f, 1.0f);  // Width of 50, height of 20, depth of 1
    glPopMatrix();
    glPushMatrix();
    glTranslatef(25.0f, 10.0f, 0.0f);  // Position the right wall
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);  // Rotate to face the right
    drawWall(40.0f, 20.0f, 1.0f);  // Width of 40, height of 20, depth of 1
    glPopMatrix();
    glColor3f(0.0f, 0.5f, 1.0f);  // Bright blue color
    
    // Draw the floor under the rink (connected to the walls)
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);  // Position the floor slightly below the walls
    drawFloor(50.0f, 1.0f, 50.0f);  // Width and depth match the size of the room
    glPopMatrix();
}



void updatePucks(float time) {
    float bounceAmplitude = 0.9f;  // Maximum up-and-down distance
    float bounceFrequency = 1.0f;

    // Loop through all pucks
    for (int i = 0; i < totalPucks; ++i) {
        if (pucks[i].active &&
            (abs(player.x - pucks[i].x) < player.size + pucks[i].size) &&
            (abs(player.z - pucks[i].z) < player.size + pucks[i].size)) {
            // Player has collided with the puck
            pucks[i].active = false;  // Make the puck disappear
            collectedPucks++;  // Increment the collected puck count
            std::cout << "Puck " << i + 1 << " collected!" << std::endl;

            // Check if all pucks are collected
            if (collectedPucks == totalPucks) {
                gameWon = true;  // Player wins when all pucks are collected
                std::cout << "You collected all the pucks! You win!" << std::endl;
                gameOver=true;
            }
        }

        if (pucks[i].active) {
            // Calculate the new Y position based on sine wave for bounce effect
            pucks[i].y = 0.05f + bounceAmplitude * sin(bounceFrequency * time + pucks[i].animationPhase);
            if (pucks[i].y < 0.05f) {
                pucks[i].y = 0.05f;  // Prevent puck from going below the rink
            }
        }
    }

    // Check if time runs out and game is lost
    if (timeRemaining <= 0 && collectedPucks < totalPucks && !gameWon) {
        gameLost = true;  // Game lost if time runs out and not all pucks are collected
        std::cout << "Time's up! You lose!" << std::endl;
        gameOver=true;
    }
}


void idle() {
    updatePucks(0.0f);
    glutPostRedisplay();
}




void drawGameOver() {
    glDisable(GL_DEPTH_TEST);  // Disable depth testing for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);  // Set text color to white
    glRasterPos2f(400 - 50, 300);  // Center the text

    std::string gameOverStr;
    if (gameWon) {
        gameOverStr = "GAME WIN";
    } else if (gameLost) {
        gameOverStr = "GAME LOST";
    } else {
        gameOverStr = "GAME OVER";
    }

    for (char c : gameOverStr) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Display final score
    glRasterPos2f(400 - 70, 270);
    std::string scoreStr = "Collected Pucks: " + std::to_string(collectedPucks);
    for (char c : scoreStr) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);  // Re-enable depth testing
}

void renderTimer() {
    glDisable(GL_DEPTH_TEST);  // Disable depth testing for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);  // White color for the timer text
    glRasterPos2f(10, 580);  // Position at the top-left corner

    std::stringstream ss;
    ss << "Time: " << std::fixed << std::setprecision(1) << timeRemaining;
    std::string timerStr = ss.str();

    for (char c : timerStr) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);  // Re-enable depth testing
}





void display() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        static float time = 0.0f;
        setCameraView();
        time += 0.05f;  // Increment time to create motion; adjust for speed
        if (gameOver) {
            glDisable(GL_DEPTH_TEST);  // Disable depth testing for 2D rendering
            drawGameOver();
            glEnable(GL_DEPTH_TEST);
        }
        else {

            updatePucks(glutGet(GLUT_ELAPSED_TIME) / 1000.0f);  // Update pucks with current time
            updatePucks(time);
            updateAnimation();
            drawRink();
            drawRoomWithFloor();
            drawWalls();
            drawIceMarkings();
            drawGoals();
            drawHockeySticks();
            drawCrowdBenches();
            glPushMatrix();
            glTranslatef(player.x, player.y, player.z);  // Move player based on current position
            drawPlayer();  // Draw the player
            glPopMatrix();
            for (int i = 0; i < 5; ++i) {
                if (pucks[i].active) {
                    drawPuck(pucks[i]);
                }
            }
            renderTimer();
        }
    
        glutSwapBuffers();
    }



// Reshape function
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)w / (GLfloat)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    updateAnimation();
    glutTimerFunc(16, timer, 0); // ~60 FPS
    if (!gameWon && !gameLost) {
        timeRemaining -= 0.1f;  // Decrease time
        if (timeRemaining <= 0.0f) {
            timeRemaining = 0.0f;
        }
        glutPostRedisplay();  // Request a redraw of the screen
        glutTimerFunc(100, timer, 0);  // Call timer every 100ms
    }
}

// Keyboard function for camera movement
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case '1':  // Press '1' for Top View
            currentView = TOP_VIEW;
            break;
        case '2':  // Press '2' for Side View
            currentView = SIDE_VIEW;
            break;
        case '3':  // Press '3' for Front View
            currentView = FRONT_VIEW;
            break;
        case 'w':
            camZ -= 0.5f;
            break;
        case 's':
            camZ += 0.5f;
            break;
        case 'a':
            camX -= 0.5f;
            break;
        case 'd':
            camX += 0.5f;
            break;
        case 'q':
            camY += 0.5f;
            break;
        case 'e':
            camY -= 0.5f;
            break;
        case 'p':
            toggleAnimation();
            break;
    }

    glutPostRedisplay();
}

void movePlayer(int key, int x, int y) {
    const float moveSpeed = 0.9f; // Movement speed

    if (key == GLUT_KEY_UP) {
        player.z -= moveSpeed;  // Move forward (increase Z)
        player.rotationAngle = 0.0f;
    }
    if (key == GLUT_KEY_DOWN) {
        player.z += moveSpeed;  // Move backward (decrease Z)
        player.rotationAngle = 180.0f;
    }
    if (key == GLUT_KEY_LEFT) {
        player.x -= moveSpeed;  // Move left (decrease X)
        player.rotationAngle = 90.0f;
    }
    if (key == GLUT_KEY_RIGHT) {
        player.x += moveSpeed;  // Move right (increase X)
        player.rotationAngle = -90.0f;
    }
    player.x = std::max(leftWallBoundary, std::min(rightWallBoundary, player.x));
    player.z = std::max(backWallBoundary, std::min(frontWallBoundary, player.z));

    glutPostRedisplay();  // Trigger redraw after movement
}





// Main function
int main(int argc, char** argv) {
    srand(time(NULL));  // Seed the random number generator
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Ice Hockey Rink Scene");
    glEnable(GL_DEPTH_TEST);
    initializePucks();

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutTimerFunc(0, timer, 0);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(movePlayer);
    
    glutMainLoop();
    return 0;
}
