#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <SOIL.h>

#if _WIN32
#   include <Windows.h>
#endif
#if __APPLE__
#   include <OpenGL/gl.h>
#   include <OpenGL/glu.h>
#   include <GLUT/glut.h>
#else
#   include <GL/gl.h>
#   include <GL/glu.h>
#   include <GL/freeglut.h>
#endif

#define ESC 27
#define SPACEBAR 32
#define G -9.8f
#define FORT_R 1.0f
#define FORT_BASE_H 0.25f
#define CANNON_SPEED 12.0f // the initial speed of cannon
#define CANNON_L 0.75f // length of cannon
#define FORT_OFFSET (0.48f + CANNON_L)
#define FORT_H (3.5f + FORT_BASE_H)
#define FORT_CENTER (3.0f + (FORT_H - 3.0f + FORT_OFFSET - CANNON_L) / 2.0f)
#define RANGE 10.0f // the area size of drawing terrian and sea
#define RANGE_SEA 100.0f
#define MAX_TESSELLATION 32
#define MIN_TESSELLATION 4
#define LENGTH_HEIGHT (MAX_TESSELLATION + 1 + 2 * MAX_TESSELLATION / MIN_TESSELLATION)
#define MILLI 1000.0f
#define RATIO 0.4f // define the ratio of unit length drawing normals
#define WIDTH 800
#define HEIGHT 600
#define ASPECT float(WIDTH)/HEIGHT
#define MAX_BOAT_NUM 8
#define ISLAND MAX_BOAT_NUM
#define ISLAND_BALL_X 0.0f
#define ISLAND_BALL_Y FORT_H + FORT_OFFSET * sinf(global.rAngle)
#define ISLAND_BALL_Z -FORT_OFFSET * cosf(global.rAngle)
#define ISLAND_BALL_VX 0.0f
#define ISLAND_BALL_VY CANNON_SPEED * sinf(global.rAngle)
#define ISLAND_BALL_VZ -CANNON_SPEED * cosf(global.rAngle)
#define BOAT_R0 50.0f
#define MAX_BOAT_A 3.0f
#define MAX_BOAT_V 3.0f
#define BOAT_L 2.0f
#define BOAT_W 1.0f
#define BOAT_H 1.0f
#define ANGLE_STEP_BOAT 1.0f
#define ANGLE_STEP_CANNON 5.0f
#define BALL_R 0.2f
#define INITIAL_BOAT_CANNON_ANGLE M_PI / 3.0
#define HEIGHT_DIF_BOAT_ISLAND (FORT_H - BOAT_H / 2.0f)
#define BOAT_BALL_X float(boat[i].p.x - (CANNON_L + BOAT_H / 2.0) * cosf(boat[i].cannonAngle) * sinf(M_PI + boat[i].angle0))
#define BOAT_BALL_Y float(boat[i].p.y + (CANNON_L + BOAT_H / 2.0) * sinf(boat[i].cannonAngle))
#define BOAT_BALL_Z float(boat[i].p.z - (CANNON_L + BOAT_H / 2.0) * cosf(boat[i].cannonAngle) * cosf(M_PI + boat[i].angle0))
#define BOAT_BALL_VX float(CANNON_SPEED * cosf(boat[i].cannonAngle) * sinf(boat[i].cangleY))
#define BOAT_BALL_VY float(CANNON_SPEED * sinf(boat[i].cannonAngle))
#define BOAT_BALL_VZ float(CANNON_SPEED * cosf(boat[i].cannonAngle) * cosf(boat[i].cangleY))
#define PARTICLE_NUM 100
#define PARTICLE_SPEED 100.0f

static GLfloat light_pos[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // Position of light
static GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat cyan[] = { 0.1f, 0.38f, 0.66f, 1.0f };
static GLfloat gray[] = { 0.4f, 0.37f, 0.25f, 1.0f };
static GLfloat seaBlue[] = { 0.18f, 0.46f, 0.68f, 0.8f };
static GLfloat red[] = { 1.0f, 0.0f, 0.0f, 1.0f };
static GLfloat transRed[] = { 1.0f, 0.0f, 0.0f, 0.6f };
static GLfloat green[] = { 0.0f, 1.0f, 0.0f, 1.0f };

GLfloat cylinderSideNormals[(MAX_TESSELLATION + 1) * MAX_TESSELLATION * 3];
GLfloat fortLeftNormals[MAX_TESSELLATION / 2 * 3 + 15], fortRightNormals[MAX_TESSELLATION / 2 * 3 + 15];
GLfloat islandNormals[(MAX_TESSELLATION + 1) * (MAX_TESSELLATION + 1) * 3];
GLfloat seaNormals[(MAX_TESSELLATION * 6 + 1) * (MAX_TESSELLATION * 6 + 1) * 3];
GLfloat seaVertices[(MAX_TESSELLATION * 6 + 1) * (MAX_TESSELLATION * 6 + 1) * 3];
GLuint textureTerrian, textureSkybox[6];

/************************************************************************************************
 * The height map is designed to have MAX_TESSELLATION / MIN_TESSELLATION lines of extra data
 * for the calculation of normal vectors. 
 *
 * For example, the height of left neighbour hL = height[n - global.next]
 * is height[- MAX_TESSELLATION / global.tessellation] for n = 0
 * if no extra data is defined.
 * In another word, for a vector x = 0, z = 0, it doesn't have a left neighbour.
 * Hence, I added height[-1] as its left neighbour and changed the array indentifier for height[-1] to positive.
 * I also added right neighbour for the right most vectors, up neighbour for the uppest vector, down neighbour for the most down vector
 *
 * So when assigning height to terrian, these extra data must be excaped.
 * There are MAX_TESSELLATION / MIN_TESSELLATION lines from the top need to be excaped,
 * which is implemented using (n + MAX_TESSELLATION / MIN_TESSELLATION) * LENGTH_HEIGHT
 * There are MAX_TESSELLATION / MIN_TESSELLATION number of elements from the left need to be excaped,
 * which is implemented using m + MAX_TESSELLATION / MIN_TESSELLATION
 *************************************************************************************************/
static GLfloat height[LENGTH_HEIGHT * LENGTH_HEIGHT] = {
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.8f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.8f, 1.8f, 1.8f, 1.8f, 1.8f, 1.8f, 1.8f, 1.8f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.8f, 1.8f, 1.8f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 1.8f, 1.6f, 1.6f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.8f, 1.8f, 2.5f, 2.5f, 2.5f, 4.0f, 4.0f, 4.0f, 2.5f, 2.5f, 2.5f, 1.6f, 1.6f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.8f, 2.5f, 2.5f, 4.0f, 4.0f, 4.0f, 5.0f, 4.0f, 4.0f, 4.0f, 2.5f, 2.5f, 1.6f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.8f, 2.5f, 2.5f, 4.1f, 4.3f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f, 4.0f, 2.5f, 1.8f, 1.6f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.8f, 2.5f, 4.0f, 4.6f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f, 2.5f, 2.5f, 1.6f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.8f, 2.5f, 2.5f, 4.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f, 2.5f, 2.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.8f, 2.5f, 4.0f, 4.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f, 4.0f, 2.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.2f, 1.4f, 1.6f, 1.8f, 2.0f, 4.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f, 2.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.2f, 1.4f, 1.6f, 1.8f, 2.0f, 4.0f, 4.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f, 4.0f, 2.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.2f, 1.4f, 1.6f, 1.8f, 2.0f, 2.0f, 4.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f, 2.5f, 2.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.2f, 1.4f, 1.4f, 1.6f, 1.8f, 2.0f, 4.0f, 4.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.5f, 4.0f, 2.5f, 1.2f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.2f, 1.4f, 1.4f, 1.6f, 1.8f, 2.0f, 2.0f, 3.0f, 4.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.6f, 4.5f, 2.5f, 2.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.2f, 1.4f, 1.4f, 1.6f, 1.6f, 1.8f, 2.0f, 2.0f, 3.0f, 4.0f, 4.0f, 5.0f, 4.0f, 4.0f, 4.0f, 1.5f, 1.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.8f, 1.0f, 1.0f, 1.0f, 1.2f, 1.4f, 1.4f, 1.4f, 1.6f, 1.6f, 1.8f, 2.0f, 2.0f, 2.0f, 4.0f, 4.0f, 4.0f, 1.5f, 1.5f, 1.5f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.8f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f, 1.4f, 1.4f, 1.4f, 1.6f, 1.6f, 1.8f, 1.8f, 2.0f, 2.0f, 2.0f, 2.0f, 1.8f, 1.6f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.8f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f, 1.4f, 1.4f, 1.4f, 1.6f, 1.6f, 1.8f, 1.8f, 1.8f, 1.8f, 1.8f, 1.6f, 1.6f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f, 1.4f, 1.4f, 1.4f, 1.6f, 1.6f, 1.6f, 1.6f, 1.6f, 1.6f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f, 1.4f, 1.4f, 1.4f, 1.6f, 1.6f, 1.6f, 1.6f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.4f, 1.4f, 1.4f, 1.4f, 1.4f, 1.4f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.8f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f, 1.2f, 1.4f, 1.4f, 1.4f, 1.4f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f, 1.2f, 1.2f, 1.2f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

typedef struct { GLfloat x, y, z; } vec3f;
typedef struct { GLfloat x, z; } vec2f;
typedef struct { vec3f p, v; } vec6f;

vec6f island;
vec6f particle[MAX_BOAT_NUM][PARTICLE_NUM];

typedef struct {
	bool debug;
	float offsetX; // sea wave movement from original place on x axis
	float t;

	bool start; // start animation
	bool go; // resume or stop animation
	float startTime;
	float stopTime; // animation stop time;
	float resumeTime;
	float pauseIntervel; // store all the paused time total

	float lastFrameRateT;
	float frameRateInterval;
	float frameRate;
	int frames;
	int tessellation;

	bool wireframeMode;

	int next; // for tessellation, get the next element from height[]
	float step; // for tessellation, calculate the increment of x and z
	float thetaStep; // the increment of theta for drawing cylinder

	float rAngle; // cannon rotation angle
	float rAngleY; // fort rotation angle
	float rAngleY0; // fort rotation angle when cannonball is fired
	int islandHitCount;
	bool bloodChanged;
	int score;
	float boatStopR; // the distance that the boat cannon will hit the island with inital cannon ratation angle of 60 degrees
	bool win;
	bool loose;
} global_t;

global_t global = { false, 0.0f, 0.0f, false, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0, 16, false, 0, 0.0f, 0.0f, float(M_PI / 6.0), 0.0f, 0.0f, 0, false, 0, 0.0f, false, false };

typedef struct { float A, k, w; } sinewave;
sinewave sw1 = { 0.6f, float(0.15 * M_PI), float(0.8 * M_PI) };
sinewave sw2 = { 0.6f, float(0.1 * M_PI), float(0.25 * M_PI) };
sinewave sw3 = { 0.2f, float(0.2 * M_PI), float(1.0 * M_PI) };
sinewave sw4 = { 0.2f, float(0.15 * M_PI), float(M_PI) };

typedef struct {
	bool btClicked; // save the click down of left and right button
	bool rotateStart;
	bool scaleStart;
	float rAngleY;
	float rAngleYOld;
	int x0;
	int y0;
	float xOld;
	float yOld;
	float x;
	float y;
	float destY;
	float destYOld;
} camera_t;

camera_t camera = { false, false, false, 0.0, 0.0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

typedef struct {
	vec6f pv;
	bool fire;
} cannonball_t;

cannonball_t ball[MAX_BOAT_NUM + 1];

typedef struct {
	vec3f p;
	vec2f v; float vF, vF0;
	/**
	 * angle - the turning angle of the boat on Y axis; changing with step when turning
	 * angleF - the final turing angle of the boat on Y axis
	 * angle0 - the original angle after initialization
	 * cannonAngle the cannon rotation angle on x axis
	 */
	float angle, angleF, angle0;
	float cannonAngle, cangleY, cangleYF; // cangleY is the cannon ratation angle on Y axis; cangleYF is the final rotation angle of cannon on Y axis
	bool isHit;
	bool turning; // start and stop turning boat
	bool predicted; // boat was predicted to be hitted and moved away
	bool changeV; // change vF to -vF
	bool particleUpdated;
	bool initial;
} boat_t;

boat_t boat[MAX_BOAT_NUM];

static GLuint loadTexture(const char *filename) {
	GLuint tex = SOIL_load_OGL_texture(filename, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
	if (!tex)
		return 0;

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

vec3f normalize(vec3f N) {
	float s = sqrtf(N.x * N.x + N.y * N.y + N.z * N.z);
	N.x /= s;
	N.y /= s;
	N.z /= s;
	return N;
}

void calcSineWave(sinewave sw1, sinewave sw2, sinewave sw3, sinewave sw4, float x, float z, float t, float *y, bool dvs, float *dydx, float *dydz, float *dy) {
	*y = sw1.A * sinf(sw1.k * x + sw1.w * t) + sw2.A * sinf(sw2.k * x + 0.2 * M_PI) + sw3.A * sinf(sw3.k * z + sw3.w * t + 0.5 * M_PI) + sw4.A * sinf(sw4.k * z);
	if (dvs) {
		*dydx = -sw1.A * sw1.k * cosf(sw1.k * x + sw1.w * t) - sw2.A * sw2.k * cosf(sw2.k * x + 0.2 * M_PI);
		*dydz = -sw3.A * sw3.k * cosf(sw3.k * z + sw3.w * t + 0.5 * M_PI) - sw4.A * sw4.k * cosf(sw4.k * z);
		float s = sqrt(*dydx * *dydx + *dydz * *dydz + 1.0);
		*dydx /= s;
		*dydz /= s;
		*dy = 1.0 / s;
	}
}

// using numeric formular to calculate y of parabola
vec6f calcParabola(vec6f pv, float dt) {
	pv.p.x += pv.v.x * dt;
	pv.p.z += pv.v.z * dt;
	pv.p.y += pv.v.y * dt;
	pv.v.y += G * dt;
	return pv;
}

// calculate the distance that the boat cannon will hit the island with inital cannon ratation angle of 60 degrees
float calcDistanceBoatHitIsland() {
	vec6f pv0;
	pv0.p = { 0.0, HEIGHT_DIF_BOAT_ISLAND, 0.0 };
	pv0.v = { 0.0, CANNON_SPEED * sinf(INITIAL_BOAT_CANNON_ANGLE), -CANNON_SPEED * cosf(INITIAL_BOAT_CANNON_ANGLE) };
	while (pv0.p.y > 0)
		pv0 = calcParabola(pv0, 0.035);
	return fabsf(pv0.p.z);
}

vec3f calcActualPositionIsland(vec3f p) {
	p.x = -p.z * sinf(global.rAngleY);
	p.z = p.z * cosf(global.rAngleY);
	return p;
}

float calcHeight(vec3f p) {
	for (int i = 0; i < MAX_BOAT_NUM; i++)
		if (fabsf(p.x - boat[i].p.x) < 1.0 && fabsf(p.z - boat[i].p.z) < 1.0 && !boat[i].isHit)
			return boat[i].p.y;
	if (fabsf(p.x) > RANGE / 2.0 || fabsf(p.z) > RANGE / 2.0)
		return -2.0f;
	int i = MAX_TESSELLATION / 2 + (int)floorf(p.x / RANGE * MAX_TESSELLATION);
	int j = MAX_TESSELLATION / 2 + (int)floorf(p.z / RANGE * MAX_TESSELLATION);
	if (p.x * p.x + p.z * p.z < FORT_R * FORT_R) {
		if (p.x * p.x + p.z * p.z < 0.5 * 0.5)
			return -2.0 + FORT_BASE_H + 0.5 + height[(j + MAX_TESSELLATION / MIN_TESSELLATION) * LENGTH_HEIGHT + i + MAX_TESSELLATION / MIN_TESSELLATION];
		return -2.0 + FORT_BASE_H + height[(j + MAX_TESSELLATION / MIN_TESSELLATION) * LENGTH_HEIGHT + i + MAX_TESSELLATION / MIN_TESSELLATION];
	}
	else
		return -2.0 + height[(j + MAX_TESSELLATION / MIN_TESSELLATION) * LENGTH_HEIGHT + i + MAX_TESSELLATION / MIN_TESSELLATION];
}

int checkHit(vec6f pv) {
	if (pv.v.y < 0) {
		if (pv.p.x * pv.p.x + pv.p.z * pv.p.z < (FORT_R + FORT_BASE_H + BALL_R) * (FORT_R + FORT_BASE_H + BALL_R)
			&& fabsf(pv.p.y - FORT_CENTER) < 0.75) {
			return ISLAND;
		} else {
			for (int i = 0; i < MAX_BOAT_NUM; i++) {
				if (fabsf(pv.p.z + sqrtf(boat[i].p.x * boat[i].p.x + boat[i].p.z * boat[i].p.z)) < BOAT_L / 2.0 + BALL_R
					&& (fabsf(fmodf(atan2f(boat[i].p.x, boat[i].p.z) + M_PI + global.rAngleY0, 2.0 * M_PI)) < 0.2
					|| fabsf(fmodf(atan2f(boat[i].p.x, boat[i].p.z) + M_PI + global.rAngleY0, 2.0 * M_PI)) > 2.0 * M_PI - 0.2)
					&& fabsf(pv.p.y - boat[i].p.y) < BOAT_H / 2.0 + BALL_R && !boat[i].isHit)
					return i;
			}
		}
	}
	return -1;
}

int predictHit(vec6f pv) {
	for (int i = 0; i < MAX_BOAT_NUM; i++) {
		if (fabsf(pv.p.z + sqrtf(boat[i].p.x * boat[i].p.x + boat[i].p.z * boat[i].p.z)) < BOAT_L / 2.0 + BALL_R
			&& (fabsf(fmodf(atan2f(boat[i].p.x, boat[i].p.z) + M_PI + global.rAngleY, 2.0 * M_PI)) < 0.2
				|| fabsf(fmodf(atan2f(boat[i].p.x, boat[i].p.z) + M_PI + global.rAngleY, 2.0 * M_PI)) > 2.0 * M_PI - 0.2)
			&& fabsf(pv.p.y - boat[i].p.y) < BOAT_H / 2.0 + BALL_R && !boat[i].isHit)
			return i;
	}
	return -1;
}

vec6f finalPointWhenParabolaTouchObject(vec6f pv) {
	do {
		pv = calcParabola(pv, 0.035);
	} while (pv.p.y > calcHeight(calcActualPositionIsland(pv.p)));
	return pv;
}

void calcNormalCylinderSide() {
	for (int j = 0; j < global.tessellation + 1; j++) {
		for (int i = 0; i < global.tessellation; i++) {
			cylinderSideNormals[j * global.tessellation * 3 + i * 3] = cosf(i * global.thetaStep);
			cylinderSideNormals[j * global.tessellation * 3 + i * 3 + 1] = 0.0;
			cylinderSideNormals[j * global.tessellation * 3 + i * 3 + 2] = sinf(i * global.thetaStep);
		}
	}
}

void calcNormalFort() {
	for (int i = 0; i < global.tessellation / 2 + 3; i++) {
		fortLeftNormals[i * 3] = -1.0;
		fortLeftNormals[i * 3 + 1] = fortLeftNormals[i * 3 + 2] = 0.0;
		fortRightNormals[i * 3] = 1.0;
		fortRightNormals[i * 3 + 1] = fortRightNormals[i * 3 + 2] = 0.0;
	}
}

void calcNormalIsland() {
	vec3f normal = { 0.0, 0.0, 0.0 };
	for (int j = 0; j < global.tessellation + 1; j++) {
		for (int i = 0; i < global.tessellation + 1; i++) {
			int n = (j * global.next + MAX_TESSELLATION / MIN_TESSELLATION) * LENGTH_HEIGHT + i * global.next + MAX_TESSELLATION / MIN_TESSELLATION;
			float hL = height[n - global.next];
			float hR = height[n + global.next];
			float hD = height[n - global.next * LENGTH_HEIGHT];
			float hU = height[n + global.next * LENGTH_HEIGHT];
			normal.x = hL - hR;
			normal.z = hD - hU;
			normal.y = 2.0;
			normal = normalize(normal);
			islandNormals[j * 3 * (global.tessellation + 1) + i * 3] = normal.x * RATIO;
			islandNormals[j * 3 * (global.tessellation + 1) + i * 3 + 1] = normal.y * RATIO;
			islandNormals[j * 3 * (global.tessellation + 1) + i * 3 + 2] = normal.z * RATIO;
		}
	}
}

void calcSea() {
	int tess = global.tessellation * 6;
	float xStep = RANGE_SEA / tess;
	float zStep = RANGE_SEA / tess;
	float x, y, z, dydx, dydz, dy;
	for (int j = 0; j < tess + 1; j++) {
		z = -RANGE_SEA / 2.0 + j * zStep;
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < tess + 1; i++) {
			x = -RANGE_SEA / 2.0 + i * xStep;
			calcSineWave(sw1, sw2, sw3, sw4, x, z, global.t, &y, true, &dydx, &dydz, &dy);
			seaNormals[(j * (tess + 1) + i) * 3] = dydx;
			seaNormals[(j * (tess + 1) + i) * 3 + 1] = dy;
			seaNormals[(j * (tess + 1) + i) * 3 + 2] = dydz;
			seaVertices[(j * (tess + 1) + i) * 3] = x;
			seaVertices[(j * (tess + 1) + i) * 3 + 1] = y;
			seaVertices[(j * (tess + 1) + i) * 3 + 2] = z;
		}
		glEnd();
	}
}

void calc() {
	global.next = MAX_TESSELLATION / global.tessellation;
	global.step = RANGE / global.tessellation;
	global.thetaStep = M_PI * 2.0 / global.tessellation;
	island.p = { ISLAND_BALL_X, ISLAND_BALL_Y, ISLAND_BALL_Z };
	island.v = { ISLAND_BALL_VX, ISLAND_BALL_VY, ISLAND_BALL_VZ };
	global.boatStopR = calcDistanceBoatHitIsland() - 2.0 * FORT_R - FORT_BASE_H;
	calcNormalCylinderSide();
	calcNormalFort();
	calcNormalIsland();
	calcSea();
}

void drawNormalCylinder(float r, float h) {
	glBegin(GL_LINES);
	// side
	float dh = h / global.tessellation;
	for (int j = 0; j < global.tessellation + 1; j++) {
		float y = j * dh;
		for (int i = 0; i < global.tessellation; i++) {
			float x = r * cosf(i * global.thetaStep);
			float z = r * sinf(i * global.thetaStep);
			glVertex3f(x, y, z);
			glVertex3f(x + cylinderSideNormals[j * global.tessellation * 3 + i * 3] * RATIO, y, z + cylinderSideNormals[j * global.tessellation * 3 + i * 3 + 2] * RATIO);
		}
	}
	glEnd();
}

void drawNormalIsland() {
	glTranslatef(-5.0, -1.0, -5.0);
	glBegin(GL_LINES);
	glColor3f(1.0, 1.0, 0.0);
	for (int j = 0; j < global.tessellation + 1; j++) {
		for (int i = 0; i < global.tessellation + 1; i++) {
			float y = height[(j * global.next + MAX_TESSELLATION / MIN_TESSELLATION) * LENGTH_HEIGHT + i * global.next + MAX_TESSELLATION / MIN_TESSELLATION];
			// draw y vector
			glVertex3f(i * global.step, y, j * global.step);
			glVertex3f(i * global.step + islandNormals[j * 3 * (global.tessellation + 1) + i * 3], y + islandNormals[j * 3 * (global.tessellation + 1) + i * 3 + 1], j * global.step + islandNormals[j * 3 * (global.tessellation + 1) + i * 3 + 2]);
		}
	}
	glEnd();
	glTranslatef(5.0, 1.0, 5.0);
}

void initialBoats() {
	srand((unsigned)time(0));
	for (int i = 0; i < MAX_BOAT_NUM; i++) {
		float random = float(rand() % 30) / 15.0 * M_PI;
		for (int j = 0; j < i + 1; j++) {
			if (random == boat[j].angle0) {
				random = float(rand() % 30) / 15.0 * M_PI;
				j = 0;
			}
		}
		boat[i].angle = boat[i].angleF = boat[i].angle0 = boat[i].cangleY = boat[i].cangleYF = random;

		boat[i].p.x = -BOAT_R0 * sinf(boat[i].angle);
		boat[i].p.z = -BOAT_R0 * cosf(boat[i].angle);
		float temp;
		calcSineWave(sw1, sw2, sw3, sw4, boat[i].p.x, boat[i].p.z, global.t, &boat[i].p.y, false, &temp, &temp, &temp);
		boat[i].cannonAngle = INITIAL_BOAT_CANNON_ANGLE;
		random = 0.5 + float(rand() % 5) / 10.0;
		boat[i].v.x = MAX_BOAT_V * random * sinf(boat[i].angle);
		boat[i].v.z = MAX_BOAT_V * random * cosf(boat[i].angle);
		boat[i].vF = MAX_BOAT_V;
		boat[i].turning = false;
		boat[i].predicted = false;
		boat[i].changeV = false;
		boat[i].vF0 = 1.0;
		boat[i].particleUpdated = false;
		boat[i].isHit = false;
		boat[i].initial = true;
	}
}

void initialBall(int i) {
	float temp;
	calcSineWave(sw1, sw2, sw3, sw4, boat[i].p.x, boat[i].p.z, global.t, &boat[i].p.y, false, &temp, &temp, &temp);
	ball[i].pv.p = { BOAT_BALL_X, BOAT_BALL_Y, BOAT_BALL_Z };
	ball[i].pv.v = { BOAT_BALL_VX, BOAT_BALL_VY, BOAT_BALL_VZ };
	ball[i].fire = true;
}

void drawAxes(float amplifyFactor) {
	glBegin(GL_LINES);
	// draw x vector
	glColor3f(1.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(1.0 * amplifyFactor, 0.0, 0.0);
	// draw y vector
	glColor3f(0.0, 1.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 1.0 * amplifyFactor, 0.0);
	// draw z vector
	glColor3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 1.0 * amplifyFactor);
	glEnd();
}

void drawParabola(vec6f pv, float dt) {
	glBegin(GL_LINE_STRIP);
	do {
		glVertex3f(pv.p.x, pv.p.y, pv.p.z);
		pv = calcParabola(pv, 0.035);
	} while (pv.p.y > calcHeight(calcActualPositionIsland(pv.p)));
	glEnd();
}

void drawParabolaAnimated(vec6f pv, float dt) {
	float h;
	glBegin(GL_LINE_STRIP);
	do {
		glVertex3f(pv.p.x, pv.p.y, pv.p.z);
		pv = calcParabola(pv, 0.035);
	} while (pv.p.y > calcHeight(pv.p));
	glEnd();
}

void drawTrajectoryIsland(vec6f island) {
	// draw original speed direction
	glColor3f(1.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(island.p.x, island.p.y, island.p.z);
	glVertex3f(island.p.x, island.p.y + island.v.y * RATIO, island.p.z + island.v.z * RATIO);
	glEnd();

	glColor3f(1.0, 1.0, 0.0);
	drawParabola(island, 0.035);
}

void drawTrajectoryBoat(int i) {
	glColor3f(0.0, 1.0, 1.0);
	drawParabolaAnimated(ball[i].pv, 0.035);
}

void drawParticles(int i) {
	float angBoat = atan2f(boat[i].p.z, boat[i].p.x);
	for (int k = 0; k < PARTICLE_NUM; k++) {
		glBegin(GL_QUADS);
		glNormal3f(1.0 * cosf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY), 0.0, 1.0 * sinf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY));
		glVertex3f(particle[i][k].p.x - 0.1 * sinf(angBoat), particle[i][k].p.y - 0.1, particle[i][k].p.z - 0.1 * cosf(angBoat));
		glNormal3f(1.0 * cosf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY), 0.0, 1.0 * sinf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY));
		glVertex3f(particle[i][k].p.x - 0.1 * sinf(angBoat), particle[i][k].p.y + 0.1, particle[i][k].p.z - 0.1 * cosf(angBoat));
		glNormal3f(1.0 * cosf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY), 0.0, 1.0 * sinf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY));
		glVertex3f(particle[i][k].p.x + 0.1 * sinf(angBoat), particle[i][k].p.y + 0.1, particle[i][k].p.z + 0.1 * cosf(angBoat));
		glNormal3f(1.0 * cosf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY), 0.0, 1.0 * sinf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY));
		glVertex3f(particle[i][k].p.x + 0.1 * sinf(angBoat), particle[i][k].p.y - 0.1, particle[i][k].p.z + 0.1 * cosf(angBoat));
		glEnd();
	}
}

void drawNormalParticles(int i) {
	for (int k = 0; k < PARTICLE_NUM; k++) {
		glBegin(GL_LINES);
		glVertex3f(particle[i][k].p.x + 1.0 * cosf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY), particle[i][k].p.y, particle[i][k].p.z + 1.0 * sinf(3.0 / 5.0 * M_PI + global.rAngleY + camera.rAngleY));
		glVertex3f(particle[i][k].p.x, particle[i][k].p.y, particle[i][k].p.z);
		glEnd();
	}
}

void drawNormalSineWave() {
	glBegin(GL_LINES);
	glColor3f(1.0, 1.0, 0.0);
	float xStep = RANGE_SEA / global.tessellation / 10.0;
	float zStep = RANGE_SEA / global.tessellation / 10.0;
	float x, y, z, dydx, dydz, dy;
	for (int j = 0; j <= global.tessellation * 10; j++) {
		z = -RANGE_SEA / 2.0 + j * zStep;
		for (int i = 0; i < global.tessellation * 10 + 1; i++) {
			x = -RANGE_SEA / 2.0 + i * xStep;
			calcSineWave(sw1, sw2, sw3, sw4, x, z, global.t, &y, true, &dydx, &dydz, &dy);
			glVertex3f(x, y, z);
			glVertex3f(x + dydx * RATIO, y + dy * RATIO, z + dydz * RATIO);
		}
	}
	glEnd();
}

void renderIsland() {
	// create vertex array
	GLfloat islandVertices[(MAX_TESSELLATION + 1) * (MAX_TESSELLATION + 1) * 3];
	for (int j = 0; j < global.tessellation + 1; j++) {
		float z = j * global.step;
		for (int i = 0; i < global.tessellation + 1; i++) {
			float x = i * global.step;
			islandVertices[j * 3 * (global.tessellation + 1) + i * 3] = x;
			islandVertices[j * 3 * (global.tessellation + 1) + i * 3 + 1] = height[(j * global.next + MAX_TESSELLATION / MIN_TESSELLATION) * LENGTH_HEIGHT + i * global.next + MAX_TESSELLATION / MIN_TESSELLATION];
			islandVertices[j * 3 * (global.tessellation + 1) + i * 3 + 2] = z;
		}
	}

	// create texture coordinates array
	GLfloat texcoords[MAX_TESSELLATION * MAX_TESSELLATION * 8];
	for (int i = 0; i < global.tessellation * global.tessellation; i++) {
		texcoords[i] = 0.0; texcoords[i + 1] = 1.0;
		texcoords[i + 2] = 0.0; texcoords[i + 3] = 0.0;
		texcoords[i + 4] = 1.0; texcoords[i + 5] = 0.0;
		texcoords[i + 6] = 1.0; texcoords[i + 7] = 1.0;
	}

	// create indices
	GLuint islandIndices[MAX_TESSELLATION * MAX_TESSELLATION * 6];
	for (int j = 0; j < global.tessellation; j++) {
		for (int i = 0; i < global.tessellation; i++) {
			islandIndices[j * 4 * global.tessellation + i * 4] = j * (global.tessellation + 1) + i;
			islandIndices[j * 4 * global.tessellation + i * 4 + 1] = (j + 1) * (global.tessellation + 1) + i;
			islandIndices[j * 4 * global.tessellation + i * 4 + 2] = (j + 1) * (global.tessellation + 1) + i + 1;
			islandIndices[j * 4 * global.tessellation + i * 4 + 3] = j * (global.tessellation + 1) + i + 1;
		}
	}

	glBindTexture(GL_TEXTURE_2D, textureTerrian);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// activate and specify pointer to vertex array
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glNormalPointer(GL_FLOAT, 0, islandNormals);
	glVertexPointer(3, GL_FLOAT, 0, islandVertices);

	glTranslatef(-5.0, -2.0, -5.0);
	glColor3f(0.4, 0.37, 0.25);
	// draw island
	glDrawElements(GL_QUADS, global.tessellation * global.tessellation * 4, GL_UNSIGNED_INT, islandIndices);
	glTranslatef(5.0, 2.0, 5.0);

	// deactivate vertex arrays after drawing
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void drawBody(float l, float w, float h) {
	float r = w / 2.0;
	// base
	glTranslatef(0.0, -h, -r);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(l, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 2.0 * r);
	glVertex3f(l, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 2.0 * r);
	glVertex3f(l, 0.0, 2.0 * r);
	glEnd();
	glTranslatef(0.0, h, r);
	// left
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 1; i < global.tessellation / 2; i++) {
		glNormal3f(-1.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, r);
		float z = r * cosf(i * global.thetaStep);
		float y = r * sinf(i * global.thetaStep);
		glNormal3f(-1.0, 0.0, 0.0);
		glVertex3f(0.0, y, z);
		z = r * cosf((i + 1) * global.thetaStep);
		y = r * sinf((i + 1) * global.thetaStep);
		glNormal3f(-1.0, 0.0, 0.0);
		glVertex3f(0.0, y, z);
	}
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, r);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, -r);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0.0, -h, -r);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, r);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0.0, -h, -r);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0.0, -h, r);
	glEnd();
	// right
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 1; i < global.tessellation / 2; i++) {
		glNormal3f(1.0, 0.0, 0.0);
		glVertex3f(l, 0.0, r);
		float z = r * cosf(i * global.thetaStep);
		float y = r * sinf(i * global.thetaStep);
		glNormal3f(1.0, 0.0, 0.0);
		glVertex3f(l, y, z);
		z = r * cosf((i + 1) * global.thetaStep);
		y = r * sinf((i + 1) * global.thetaStep);
		glNormal3f(1.0, 0.0, 0.0);
		glVertex3f(l, y, z);
	}
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(l, 0.0, r);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(l, 0.0, -r);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(l, -h, -r);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(l, 0.0, r);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(l, -h, -r);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(l, -h, r);
	glEnd();
	// top
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(l, -h, r);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, -h, r);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 0.0, r);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(l, -h, r);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 0.0, r);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(l, 0.0, r);
	for (int j = 0; j < global.tessellation; j++) {
		float dl = l / global.tessellation;
		float x = j * dl;
		for (int i = 0; i < global.tessellation / 2 + 1; i++) {
			float z = r * cosf(i * global.thetaStep);
			float y = r * sinf(i * global.thetaStep);
			float dz = 1.0 * cosf(i * global.thetaStep);
			float dy = 1.0 * sinf(i * global.thetaStep);
			glNormal3f(0.0, dy, dz);
			glVertex3f(x, y, z);
			glNormal3f(0.0, dy, dz);
			glVertex3f(x + dl, y, z);
		}
	}
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(l, 0.0, -r);
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0.0, 0.0, -r);
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0.0, -h, -r);
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(l, 0.0, -r);
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0.0, -h, -r);
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(l, -h, -r);
	glEnd();
}

void drawNormalBody(float l, float w, float h) {
	float r = w / 2.0;
	glBegin(GL_LINES);
	// left
	for (int i = 1; i < global.tessellation / 2 + 1; i++) {
		if (i % 2 == 1) {
			glVertex3f(-1.0 * RATIO, 0.0, -r);
			glVertex3f(0.0, 0.0, -r);
		}
		float z = r * cosf(i * global.thetaStep);
		float y = r * sinf(i * global.thetaStep);
		glVertex3f(-1.0 * RATIO, y, z);
		glVertex3f(0.0, y, z);
	}
	glVertex3f(-1.0 * RATIO, 0.0, -r);
	glVertex3f(0.0, 0.0, -r);
	glVertex3f(-1.0 * RATIO, 0.0, r);
	glVertex3f(0.0, 0.0, r);
	glVertex3f(-1.0 * RATIO, -h, r);
	glVertex3f(0.0, -h, r);
	glVertex3f(-1.0 * RATIO, 0.0, -r);
	glVertex3f(0.0, 0.0, -r);
	glVertex3f(-1.0 * RATIO, -h, r);
	glVertex3f(0.0, -h, r);
	glVertex3f(-1.0 * RATIO, -h, -r);
	glVertex3f(0.0, -h, -r);
	// right
	for (int i = 1; i < global.tessellation / 2 + 1; i++) {
		if (i % 2 == 1) {
			glVertex3f(l + 1.0 * RATIO, 0.0, r);
			glVertex3f(l, 0.0, r);
		}
		float z = r * cosf(i * global.thetaStep);
		float y = r * sinf(i * global.thetaStep);
		glVertex3f(l + 1.0 * RATIO, y, z);
		glVertex3f(l, y, z);
	}
	glVertex3f(l + 1.0 * RATIO, 0.0, r);
	glVertex3f(l, 0.0, r);
	glVertex3f(l + 1.0 * RATIO, 0.0, -r);
	glVertex3f(l, 0.0, -r);
	glVertex3f(l + 1.0 * RATIO, -h, -r);
	glVertex3f(l, -h, -r);
	glVertex3f(l + 1.0 * RATIO, 0.0, r);
	glVertex3f(l, 0.0, r);
	glVertex3f(l + 1.0 * RATIO, -h, -r);
	glVertex3f(l, -h, -r);
	glVertex3f(l + 1.0 * RATIO, -h, r);
	glVertex3f(l, -h, r);
	// top
	glVertex3f(0.0, 0.0, r + 1.0 * RATIO);
	glVertex3f(0.0, 0.0, r);
	glVertex3f(l, 0.0, r + 1.0 * RATIO);
	glVertex3f(l, 0.0, r);
	glVertex3f(l, -h, r + 1.0 * RATIO);
	glVertex3f(l, -h, r);
	glVertex3f(0.0, 0.0, r + 1.0 * RATIO);
	glVertex3f(0.0, 0.0, r);
	glVertex3f(l, -h, r + 1.0 * RATIO);
	glVertex3f(l, -h, r);
	glVertex3f(0.0, -h, r + 1.0 * RATIO);
	glVertex3f(0.0, -h, r);
	for (int j = 0; j < global.tessellation; j++) {
		float dl = l / global.tessellation;
		float x = j * dl;
		for (int i = 0; i < global.tessellation / 2 + 1; i++) {
			float z = r * cosf(i * global.thetaStep);
			float y = r * sinf(i * global.thetaStep);
			float dz = 1.0 * cosf(i * global.thetaStep);
			float dy = 1.0 * sinf(i * global.thetaStep);
			glVertex3f(x, y + dy * RATIO, z + dz * RATIO);
			glVertex3f(x, y, z);
			glVertex3f(x + dl, y + dy * RATIO, z + dz * RATIO);
			glVertex3f(x + dl, y, z);
		}
	}
	glVertex3f(l, 0.0, -r - 1.0 * RATIO);
	glVertex3f(l, 0.0, -r);
	glVertex3f(0.0, 0.0, -r - 1.0 * RATIO);
	glVertex3f(0.0, 0.0, -r);
	glVertex3f(0.0, -h, -r - 1.0 * RATIO);
	glVertex3f(0.0, -h, -r);
	glVertex3f(l, 0.0, -r - 1.0 * RATIO);
	glVertex3f(l, 0.0, -r);
	glVertex3f(0.0, -h, -r - 1.0 * RATIO);
	glVertex3f(0.0, -h, -r);
	glVertex3f(l, -h, -r - 1.0 * RATIO);
	glVertex3f(l, -h, -r);
	glEnd();
}

void drawNormalHalfCylinder(float r, float h) {
	// left
	glBegin(GL_LINES);
	for (int i = 0; i < global.tessellation / 2 + 1; i++) {
		float z = r * cosf(i * global.thetaStep);
		float y = r * sinf(i * global.thetaStep);
		glVertex3f(-1.0 * RATIO, y, z);
		glVertex3f(0.0, y, z);
	}
	glEnd();
	// right
	glBegin(GL_LINES);
	for (int i = 0; i < global.tessellation / 2 + 1; i++) {
		float z = r * cosf(i * global.thetaStep);
		float y = r * sinf(i * global.thetaStep);
		glVertex3f(h + 1.0 * RATIO, y, z);
		glVertex3f(h, y, z);
	}
	glEnd();
	// top
	glBegin(GL_LINES);
	for (int j = 0; j < global.tessellation; j++) {
		float dh = h / global.tessellation;
		float x = j * dh;
		for (int i = 0; i < global.tessellation / 2 + 1; i++) {
			float z = r * cosf(i * global.thetaStep);
			float y = r * sinf(i * global.thetaStep);
			float dz = 1.0 * cosf(i * global.thetaStep);
			float dy = 1.0 * sinf(i * global.thetaStep);
			glVertex3f(x, y + dy * RATIO, z + dz * RATIO);
			glVertex3f(x, y, z);
			glVertex3f(x + dh, y + dy * RATIO, z + dz * RATIO);
			glVertex3f(x + dh, y, z);
		}
	}
	glEnd();
}

void drawSky(float ratio) {
	// just in case we set all vertices to blue
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	// left (neg x)
	glBindTexture(GL_TEXTURE_2D, textureSkybox[0]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-1.0 * ratio, -1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(1, 0); glVertex3f(-1.0 * ratio, -1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(1, 1); glVertex3f(-1.0 * ratio, 1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(0, 1); glVertex3f(-1.0 * ratio, 1.0 * ratio, 1.0 * ratio);
	glEnd();
	// right (pos x)
	glBindTexture(GL_TEXTURE_2D, textureSkybox[3]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(1.0 * ratio, -1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(1, 0); glVertex3f(1.0 * ratio, -1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(1, 1); glVertex3f(1.0 * ratio, 1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(0, 1); glVertex3f(1.0 * ratio, 1.0 * ratio, -1.0 * ratio);
	glEnd();
	// front (pos z)
	glBindTexture(GL_TEXTURE_2D, textureSkybox[5]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(1.0 * ratio, -1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(1, 0); glVertex3f(-1.0 * ratio, -1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(1, 1); glVertex3f(-1.0 * ratio, 1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(0, 1); glVertex3f(1.0 * ratio, 1.0 * ratio, 1.0 * ratio);
	glEnd();
	// back (neg z)
	glBindTexture(GL_TEXTURE_2D, textureSkybox[2]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-1.0 * ratio, -1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(1, 0); glVertex3f(1.0 * ratio, -1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(1, 1); glVertex3f(1.0 * ratio, 1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(0, 1); glVertex3f(-1.0 * ratio, 1.0 * ratio, -1.0 * ratio);
	glEnd();
	// top (pos y)
	glBindTexture(GL_TEXTURE_2D, textureSkybox[4]);
	glBegin(GL_QUADS); 
	glTexCoord2f(0, 0); glVertex3f(1.0 * ratio, 1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(1, 0); glVertex3f(-1.0 * ratio, 1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(1, 1); glVertex3f(-1.0 * ratio, 1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(0, 1); glVertex3f(1.0 * ratio, 1.0 * ratio, -1.0 * ratio);
	glEnd();
	// bottom (neg y)
	glBindTexture(GL_TEXTURE_2D, textureSkybox[1]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-1.0 * ratio, -1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(1, 0); glVertex3f(1.0 * ratio, -1.0 * ratio, -1.0 * ratio);
	glTexCoord2f(1, 1); glVertex3f(1.0 * ratio, -1.0 * ratio, 1.0 * ratio);
	glTexCoord2f(0, 1); glVertex3f(-1.0 * ratio, -1.0 * ratio, 1.0 * ratio);
	glEnd();
}

void drawNormal() {
	glColor3f(1.0, 1.0, 0.0);
	drawNormalSineWave();
	glTranslatef(0.0, -1.0, 0.0);
	drawNormalIsland();
	// draw normal of fort
	glTranslatef(0.0, 4.0, 0.0);
	drawNormalCylinder(1.0, 0.25);
	glRotatef(-global.rAngleY * 180.0 / M_PI, 0.0, 1.0, 0.0);
	glTranslatef(-0.5, 0.75, 0.0);
	drawNormalBody(1.0, 1.0, 0.5);
	glTranslatef(0.5, 0.48 * sinf(global.rAngle), -0.48 * cosf(global.rAngle));
	glRotatef(-90.0 + global.rAngle * 180.0 / M_PI, 1.0, 0.0, 0.0);
	drawNormalCylinder(0.2, 0.75);
	glRotatef(90.0 - global.rAngle * 180.0 / M_PI, 1.0, 0.0, 0.0);
	glTranslatef(-0.5, -0.48 * sinf(global.rAngle), 0.48 * cosf(global.rAngle));
	glTranslatef(0.5, -0.75, 0.0);
	glRotatef(global.rAngleY * 180.0 / M_PI, 0.0, 1.0, 0.0);
	glTranslatef(0.0, -3.0, 0.0);
	for (int i = 0; i < MAX_BOAT_NUM; i++)
	if (boat[i].isHit)
		drawNormalParticles(i);
}

void renderCylinder(float r, float h) {
	// base
	GLfloat baseVertices[MAX_TESSELLATION * 3];
	for (int i = 0; i < global.tessellation; i++) {
		baseVertices[i * 3] = r * cosf(i * global.thetaStep);
		baseVertices[i * 3 + 1] = 0.0;
		baseVertices[i * 3 + 2] = r * sinf(i * global.thetaStep);
	}

	GLfloat baseNormals[MAX_TESSELLATION * 3];
	for (int i = 0; i < global.tessellation; i++) {
		baseNormals[i * 3] = 0.0;
		baseNormals[i * 3 + 1] = -1.0;
		baseNormals[i * 3 + 2] = 0.0;
	}

	GLuint baseIndices[MAX_TESSELLATION * 3 - 6];
	for (int i = 0; i < global.tessellation / 2 - 1; i++) {
		baseIndices[i * 6] = 0;
		baseIndices[i * 6 + 1] = i * 2 + 1;
		baseIndices[i * 6 + 2] = i * 2 + 2;
		baseIndices[i * 6 + 3] = 0;
		baseIndices[i * 6 + 4] = i * 2 + 2;
		baseIndices[i * 6 + 5] = i * 2 + 3;
	}

	// activate and specify pointer to vertex array
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glNormalPointer(GL_FLOAT, 0, baseNormals);
	glVertexPointer(3, GL_FLOAT, 0, baseVertices);

	// render cylinder base
	glDrawElements(GL_TRIANGLES, global.tessellation * 3 - 6, GL_UNSIGNED_INT, baseIndices);

	// top
	for (int i = 0; i < global.tessellation; i++)
		baseVertices[i * 3 + 1] = h;

	for (int i = 0; i < global.tessellation; i++)
		baseNormals[i * 3 + 1] = 1.0;

	glNormalPointer(GL_FLOAT, 0, baseNormals);
	glVertexPointer(3, GL_FLOAT, 0, baseVertices);
	// render cylinder top
	glDrawElements(GL_TRIANGLES, global.tessellation * 3 - 6, GL_UNSIGNED_INT, baseIndices);

	// side
	GLfloat sideVertices[(MAX_TESSELLATION + 1) * MAX_TESSELLATION * 3];
	float dh = h / global.tessellation;
	for (int j = 0; j < global.tessellation + 1; j++) {
		for (int i = 0; i < global.tessellation; i++) {
			sideVertices[j * global.tessellation * 3 + i * 3] = r * cosf(i * global.thetaStep);
			sideVertices[j * global.tessellation * 3 + i * 3 + 1] = j * dh;
			sideVertices[j * global.tessellation * 3 + i * 3 + 2] = r * sinf(i * global.thetaStep);
		}
	}

	GLuint sideIndices[MAX_TESSELLATION * MAX_TESSELLATION * 6];
	for (int j = 0; j < global.tessellation; j++) {
		for (int i = 0; i < global.tessellation; i++) {
			sideIndices[j * global.tessellation * 6 + i * 6] = global.tessellation * (j + 1) + i;
			sideIndices[j * global.tessellation * 6 + i * 6 + 1] = global.tessellation * j + i;
			if (i == global.tessellation - 1)
				sideIndices[j * global.tessellation * 6 + i * 6 + 2] = sideIndices[j * global.tessellation * 6 + i * 6 + 3] = global.tessellation * (j - 1) + i + 1;
			else
				sideIndices[j * global.tessellation * 6 + i * 6 + 2] = sideIndices[j * global.tessellation * 6 + i * 6 + 3] = global.tessellation * (j + 1) + i + 1;
			sideIndices[j * global.tessellation * 6 + i * 6 + 4] = global.tessellation * j + i;
			sideIndices[j * global.tessellation * 6 + i * 6 + 5] = global.tessellation * j + i + 1;
		}
	}

	glNormalPointer(GL_FLOAT, 0, cylinderSideNormals);
	glVertexPointer(3, GL_FLOAT, 0, sideVertices);
	// render cylinder side
	glDrawElements(GL_TRIANGLES, global.tessellation * global.tessellation * 6, GL_UNSIGNED_INT, sideIndices);
	
	// deactivate vertex arrays after drawing
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void renderCannonball(int i) {
	GLfloat ballVertices[MAX_TESSELLATION * (MAX_TESSELLATION + 1) * 3];
	GLfloat ballNormals[MAX_TESSELLATION * (MAX_TESSELLATION + 1) * 3];
	static float step_theta = 2.0 * M_PI / global.tessellation;
	static float step_phi = M_PI / global.tessellation;
	for (int j = 0; j <= global.tessellation; j++) {
		float phi = j / (float)global.tessellation * M_PI;
		for (int i = 0; i < global.tessellation; i++) {
			float theta = i / (float)global.tessellation * 2.0 * M_PI;
			ballVertices[j * global.tessellation * 3 + i * 3] = BALL_R * sinf(phi) * cosf(theta);
			ballVertices[j * global.tessellation * 3 + i * 3 + 1] = BALL_R * sinf(phi) * sinf(theta);
			ballVertices[j * global.tessellation * 3 + i * 3 + 2] = BALL_R * cosf(phi);
			ballNormals[j * global.tessellation * 3 + i * 3] = sinf(phi) * cosf(theta);;
			ballNormals[j * global.tessellation * 3 + i * 3 + 1] = sinf(phi) * sinf(theta);
			ballNormals[j * global.tessellation * 3 + i * 3 + 2] = cosf(phi);
		}
	}

	GLuint ballIndices[MAX_TESSELLATION * MAX_TESSELLATION * 6];
	for (int j = 0; j < global.tessellation; j++) {
		for (int i = 0; i < global.tessellation; i++) {
			ballIndices[j * global.tessellation * 6 + i * 6] = j * global.tessellation + i;
			ballIndices[j * global.tessellation * 6 + i * 6 + 2] = (j + 1) * global.tessellation + i;
			ballIndices[j * global.tessellation * 6 + i * 6 + 4] = (j + 1) * global.tessellation + i;
			if (i == global.tessellation - 1) {
				ballIndices[j * global.tessellation * 6 + i * 6 + 1] = j - 1 * global.tessellation + i + 1;
				ballIndices[j * global.tessellation * 6 + i * 6 + 3] = j - 1 * global.tessellation + i + 1;
				ballIndices[j * global.tessellation * 6 + i * 6 + 5] = j * global.tessellation + i + 1;
			} else {
				ballIndices[j * global.tessellation * 6 + i * 6 + 1] = j * global.tessellation + i + 1;
				ballIndices[j * global.tessellation * 6 + i * 6 + 3] = j * global.tessellation + i + 1;
				ballIndices[j * global.tessellation * 6 + i * 6 + 5] = (j + 1) * global.tessellation + i + 1;
			}
		}
	}

	// activate and specify pointer to vertex array
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glNormalPointer(GL_FLOAT, 0, ballNormals);
	glVertexPointer(3, GL_FLOAT, 0, ballVertices);
	// render cannonball
	glTranslatef(ball[i].pv.p.x, ball[i].pv.p.y, ball[i].pv.p.z);
	glDrawElements(GL_TRIANGLES, global.tessellation * global.tessellation * 6, GL_UNSIGNED_INT, ballIndices);
	glTranslatef(-ball[i].pv.p.x, -ball[i].pv.p.y, -ball[i].pv.p.z);

	// deactivate vertex arrays after drawing
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void drawBoat(int i) {
	float temp;
	calcSineWave(sw1, sw2, sw3, sw4, boat[i].p.x, boat[i].p.z, global.t, &boat[i].p.y, false, &temp, &temp, &temp);
	glTranslatef(boat[i].p.x, boat[i].p.y, boat[i].p.z);
	glRotatef(boat[i].angle * 180.0 / M_PI, 0.0, 1.0, 0.0);
	// left side
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 2.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, -BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, -BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, -BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, -BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 2.0);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, -BOAT_H / 2.0, -BOAT_L / 4.0);
	glEnd();
	// right side
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, -BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 2.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, -BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, -BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, -BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, -BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 2.0);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 4.0);
	glEnd();
	// top
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 2.0);
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 2.0);
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 2.0);
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 2.0);
	glEnd();
	// bottom
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), 1.0 / sqrtf(2.0));
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 2.0);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), 1.0 / sqrtf(2.0));
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, BOAT_L / 2.0);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), 1.0 / sqrtf(2.0));
	glVertex3f(-BOAT_W / 2.0, -BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), 1.0 / sqrtf(2.0));
	glVertex3f(BOAT_W / 2.0, -BOAT_H / 2.0, BOAT_L / 4.0);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), -1.0 / sqrtf(2.0));
	glVertex3f(-BOAT_W / 2.0, -BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), -1.0 / sqrtf(2.0));
	glVertex3f(BOAT_W / 2.0, -BOAT_H / 2.0, -BOAT_L / 4.0);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), -1.0 / sqrtf(2.0));
	glVertex3f(-BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 2.0);
	glNormal3f(0.0, -1.0 / sqrtf(2.0), -1.0 / sqrtf(2.0));
	glVertex3f(BOAT_W / 2.0, BOAT_H / 2.0, -BOAT_L / 2.0);
	glEnd();
	glRotatef(-boat[i].angle * 180.0 / M_PI, 0.0, 1.0, 0.0);
	// cannon
	glRotatef(boat[i].cangleY * 180 / M_PI, 0.0, 1.0, 0.0);
	glRotatef(90.0 - boat[i].cannonAngle * 180.0 / M_PI, 1.0, 0.0, 0.0);
	renderCylinder(BALL_R, CANNON_L + BOAT_H / 2.0);
	glRotatef(-90.0 + boat[i].cannonAngle * 180.0 / M_PI, 1.0, 0.0, 0.0);
	glRotatef(-boat[i].cangleY * 180 / M_PI, 0.0, 1.0, 0.0);
	glTranslatef(-boat[i].p.x, -boat[i].p.y, -boat[i].p.z);
}

void drawFort() {
	glTranslatef(0.0, 3.0, 0.0);
	// draw base
	renderCylinder(FORT_R, FORT_BASE_H);
	glRotatef(-global.rAngleY * 180.0 / M_PI, 0.0, 1.0, 0.0);
	glTranslatef(-0.5, 0.75, 0.0);
	// draw body
	drawBody(1.0, 1.0, 0.5);
	// draw cannon
	glTranslatef(0.5, 0.48 * sinf(global.rAngle), -0.48 * cosf(global.rAngle));
	glRotatef(-90.0 + global.rAngle * 180.0 / M_PI, 1.0, 0.0, 0.0);
	renderCylinder(0.2, 0.75);
	glRotatef(90.0 - global.rAngle * 180.0 / M_PI, 1.0, 0.0, 0.0);
	glTranslatef(-0.5, -0.48 * sinf(global.rAngle), 0.48 * cosf(global.rAngle));
	glTranslatef(0.5, -0.75, 0.0);
	glRotatef(global.rAngleY * 180.0 / M_PI, 0.0, 1.0, 0.0);
	glTranslatef(0.0, -3.0, 0.0);
}

void renderSea() {
	int tess = global.tessellation * 6;
	// create indices
	GLuint seaIndices[MAX_TESSELLATION * MAX_TESSELLATION * 6 * 36];
	for (int j = 0; j < tess; j++) {
		for (int i = 0; i < tess; i++) {
			seaIndices[j * 6 * tess + i * 6] = seaIndices[j * 6 * tess + i * 6 + 3] = j * (tess + 1) + i;
			seaIndices[j * 6 * tess + i * 6 + 1] = (j + 1) * (tess + 1) + i;
			seaIndices[j * 6 * tess + i * 6 + 2] = seaIndices[j * 6 * tess + i * 6 + 4] = (j + 1) * (tess + 1) + i + 1;
			seaIndices[j * 6 * tess + i * 6 + 5] = j * (tess + 1) + i + 1;
		}
	}

	// activate and specify pointer to vertex array
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glNormalPointer(GL_FLOAT, 0, seaNormals);
	glVertexPointer(3, GL_FLOAT, 0, seaVertices);
	// render sea
	glDrawElements(GL_TRIANGLES, tess * tess * 6, GL_UNSIGNED_INT, seaIndices);
	// deactivate vertex arrays after drawing
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void updateParticles(float dt, int i) {
	for (int k = 0; k < PARTICLE_NUM; k++) {
		particle[i][k].p.x += particle[i][k].v.x * dt;
		particle[i][k].p.y += particle[i][k].v.y * dt;
		particle[i][k].p.z += particle[i][k].v.z * dt;
		particle[i][k].v.y += G * dt;
	}
}

void updateBoatV(float dt, int i) {
	if (boat[i].vF > MAX_BOAT_V)
		boat[i].vF = MAX_BOAT_V;
	if (boat[i].vF < -MAX_BOAT_V)
		boat[i].vF = -MAX_BOAT_V;
	if (powf(boat[i].v.x, 2) + powf(boat[i].v.z, 2) < powf(boat[i].vF - 0.1, 2)) {
		boat[i].v.x += MAX_BOAT_A * sinf(boat[i].angle) * dt;
		boat[i].v.z += MAX_BOAT_A * cosf(boat[i].angle) * dt;
	} else {
		boat[i].v.x = boat[i].vF * sinf(boat[i].angle);
		boat[i].v.z = boat[i].vF * cosf(boat[i].angle);
	}
	if (powf(boat[i].v.x, 2) + powf(boat[i].v.z, 2) > powf(boat[i].vF + 0.1, 2)) {
		boat[i].v.x -= MAX_BOAT_A * sinf(boat[i].angle) * dt;
		boat[i].v.z -= MAX_BOAT_A * cosf(boat[i].angle) * dt;
	} else {
		boat[i].v.x = boat[i].vF * sinf(boat[i].angle);
		boat[i].v.z = boat[i].vF * cosf(boat[i].angle);
	}
}

void moveBoat(float dt, int i) {
	updateBoatV(dt, i);
	boat[i].p.x += boat[i].v.x * dt;
	boat[i].p.z += boat[i].v.z * dt;
	float temp;
	calcSineWave(sw1, sw2, sw3, sw4, boat[i].p.x, boat[i].p.z, global.t, &boat[i].p.y, false, &temp, &temp, &temp);
 }

void turnBoat(float dt, int i) {
	if (boat[i].angle < boat[i].angleF) {
		boat[i].angle += ANGLE_STEP_BOAT * dt;
	}
	if (boat[i].angle > boat[i].angleF) {
		boat[i].angle -= ANGLE_STEP_BOAT * dt;
	}
}

void turnCannon(float dt, int i) {
	if (boat[i].cangleY < boat[i].cangleYF) {
		boat[i].cangleY += ANGLE_STEP_CANNON * dt;
	}
	if (boat[i].cangleY > boat[i].cangleYF) {
		boat[i].cangleY -= ANGLE_STEP_CANNON * dt;
	}
}

void updateBoat(float dt, int i) {
	moveBoat(dt, i);
	turnCannon(dt, i);
	if (boat[i].v.x == 0.0 && boat[i].v.z == 0.0) {
		turnBoat(dt, i);
	}
}

float normalizeAngle(float angle) {
	while (angle < 0)
		angle += 2.0 * M_PI;
	while (angle > 2.0 * M_PI)
		angle -= 2.0 * M_PI;
	return angle;
}

void boatMoveAI(int i) {
	if (boat[i].initial) {
		if (powf(boat[i].p.x, 2) + powf(boat[i].p.z, 2) < powf(global.boatStopR, 2)) {
			boat[i].vF = 0.0;
		}
		if (powf(boat[i].p.x, 2) + powf(boat[i].p.z, 2) <= powf(global.boatStopR, 2) && !boat[i].turning) {
			boat[i].angleF = normalizeAngle(boat[i].angleF + M_PI / 2.0);
			boat[i].angle = normalizeAngle(boat[i].angle);
			if (boat[i].angleF - boat[i].angle > M_PI)
				boat[i].angleF -= 2.0 * M_PI;
			if (boat[i].angleF - boat[i].angle < -M_PI)
				boat[i].angleF += 2.0 * M_PI;
			boat[i].turning = true;
		}
		if (fabsf(boat[i].angle - boat[i].angleF) < ANGLE_STEP_BOAT * 0.1) {
			boat[i].angle = boat[i].angleF;
			if (boat[i].turning)
				boat[i].initial = false;
		}
	}
	else {
		if (predictHit(finalPointWhenParabolaTouchObject(island)) == i) {
			boat[i].vF = boat[i].vF0;
			if (powf(boat[i].p.x, 2) + powf(boat[i].p.z, 2) > powf(global.boatStopR + 0.2, 2) && !boat[i].changeV) {
				boat[i].vF0 = -boat[i].vF0;
				boat[i].changeV = true;
			}
			boat[i].predicted = true;
		}
		if (predictHit(finalPointWhenParabolaTouchObject(island)) != i && boat[i].predicted) {
			boat[i].vF = 0.0;
			boat[i].cangleYF = normalizeAngle(atan2f(boat[i].p.x, boat[i].p.z) + M_PI);
			boat[i].cangleY = normalizeAngle(boat[i].cangleY);
			if (boat[i].cangleYF - boat[i].cangleY > M_PI)
				boat[i].cangleYF -= 2.0 * M_PI;
			if (boat[i].cangleYF - boat[i].cangleY < -M_PI)
				boat[i].cangleYF += 2.0 * M_PI;
			if (fabsf(boat[i].cangleY - boat[i].cangleYF) < ANGLE_STEP_BOAT * 0.1) {
				boat[i].cangleY = boat[i].cangleYF;
			}
			if (powf(boat[i].p.x, 2) + powf(boat[i].p.z, 2) < powf(global.boatStopR + 0.2, 2))
				boat[i].changeV = false;
		}
	}
}

// numerical method
void updateCannonball(float dt, int i) {
	ball[i].pv.p.x += ball[i].pv.v.x * dt;
	ball[i].pv.p.z += ball[i].pv.v.z * dt;
	ball[i].pv.p.y += ball[i].pv.v.y * dt;
	ball[i].pv.v.y += G * dt;
}

// Idle callback for animation
void update() {
	static float lastT = -1.0;
	float dt;

	if (!global.go)
		return;
	global.t = glutGet(GLUT_ELAPSED_TIME) / MILLI - global.pauseIntervel - global.startTime;

	if (lastT < 0.0) {
		lastT = global.t;
		return;
	}

	dt = global.t - lastT;
	lastT = global.t;
	if (global.debug)
		printf("%f %f\n", global.t, dt);

	// update island cannonball
	if (ball[ISLAND].fire)
		updateCannonball(dt, ISLAND);
	// when island cannonball hits anything except the sea, it disappears
	if (ball[ISLAND].pv.p.y < calcHeight(calcActualPositionIsland(ball[ISLAND].pv.p))) {
		ball[ISLAND].fire = false;
	}
	for (int i = 0; i < MAX_BOAT_NUM; i++) {
		if (!boat[i].isHit) {
			// moving from the distant position towards the island
			boatMoveAI(i);
			updateBoat(dt, i);
			// fire the cannonball
			if (!ball[i].fire)
				initialBall(i);
		}
		if (ball[i].fire)
			updateCannonball(dt, i);
		// when boat cannonball hits anything except the sea, it disappears
		if (ball[i].pv.p.y < calcHeight(ball[i].pv.p))
			ball[i].fire = false;
		// check if the boat cannonball hits the island
		if (checkHit(ball[i].pv) == ISLAND) {
			if (global.islandHitCount <= 4000)
				global.islandHitCount++;
			else
				global.loose = true;
		}
		if (boat[i].isHit) {
			if (!boat[i].particleUpdated) {
				for (int k = 0; k < PARTICLE_NUM; k++)
					particle[i][k].p = boat[i].p;
				boat[i].particleUpdated = true;
			}
			updateParticles(dt, i);
		}
		for (int k = 0; k < PARTICLE_NUM; k++)
			if (particle[i][k].p.y < calcHeight(particle[i][k].p))
				particle[i][k].p.y = -100000.0;
	}
	global.bloodChanged = true;

	// frame rate
	dt = global.t - global.lastFrameRateT;
	if (dt > global.frameRateInterval) {
		global.frameRate = global.frames / dt;
		global.lastFrameRateT = global.t;
		global.frames = 0;
	}

	glutPostRedisplay();
}

void renderOSD() {
	char buffer[60];
	char *bufp;
	int w, h;

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	/* Set up orthographic coordinate system to match the
	window, i.e. (0,0)-(w,h) */
	w = glutGet(GLUT_WINDOW_WIDTH);
	h = glutGet(GLUT_WINDOW_HEIGHT);
	glOrtho(0.0, w, 0.0, h, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// blood bar
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_QUADS);
	glVertex2i(30, 560);
	glVertex2i(30, 580);
	int bloodlength = int(floorf(float(global.islandHitCount / 10)));
	glVertex2i(430 - bloodlength, 580);
	glVertex2i(430 - bloodlength, 560);
	glEnd();

	// score
	glColor3f(1.0, 1.0, 0.0);
	glRasterPos2i(30, 530);
	snprintf(buffer, sizeof buffer, "Score: %d", global.score);
	for (bufp = buffer; *bufp; bufp++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *bufp);

	if (!global.start) {
		glColor3f(1.0, 1.0, 0.0);
		glRasterPos2i(300, 300);
		snprintf(buffer, sizeof buffer, "Press any key to start...");
		for (bufp = buffer; *bufp; bufp++)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *bufp);
	} else if (!global.go) {
		glColor3f(1.0, 1.0, 0.0);
		glRasterPos2i(300, 300);
		snprintf(buffer, sizeof buffer, "Press key 'g' to resume...");
		for (bufp = buffer; *bufp; bufp++)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *bufp);
	}

	if (global.win || global.loose) {
		global.go = !global.go;
		glColor3f(1.0, 1.0, 0.0);
		glRasterPos2i(350, 300);
		if (global.win)
			snprintf(buffer, sizeof buffer, " You win!");
		if (global.loose)
			snprintf(buffer, sizeof buffer, "You loose!");
		for (bufp = buffer; *bufp; bufp++)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *bufp);
	}

	/* Pop modelview */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);

	/* Pop projection */
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	/* Pop attributes */
	glPopAttrib();
}

void display() {
	GLenum error;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (global.wireframeMode) {
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	/* clear the matrix */
	glLoadIdentity();
	/* viewing transformation  */
	gluLookAt((12.0 + camera.x) * cosf(3.0 / 5.0 * M_PI + camera.rAngleY + global.rAngleY), 9.0 + camera.y, (12.0 + camera.x) * sinf(3.0 / 5.0 * M_PI + camera.rAngleY + global.rAngleY), 0.0, 7.0 + camera.destY, 0.0, 0.0, 1.0, 0.0);
	/* modeling transformation */
	glScalef(1.0, 1.0, 1.0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glPushMatrix();

	calc();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glTranslatef(0.0, 20.0, 0.0);
	drawSky(50.0f);
	glTranslatef(0.0, -20.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gray);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);	
	renderIsland();
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	if (global.start) {
		int boatHit = checkHit(ball[ISLAND].pv);
		if (boatHit >= 0 && boatHit != ISLAND) {
			boat[boatHit].isHit = true;
			global.score++;
			if (global.score == MAX_BOAT_NUM)
				global.win = true;
		}
		int boatWillHit = predictHit(finalPointWhenParabolaTouchObject(island));
		for (int i = 0; i < MAX_BOAT_NUM; i++) {
			if (!boat[i].isHit) {
				if (boatWillHit == i)
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
				else
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);
				drawBoat(i);
			} else {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, transRed);
				drawParticles(i);
			}
			if (ball[i].fire) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cyan);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);
				renderCannonball(i);
				glDisable(GL_LIGHTING);
				drawTrajectoryBoat(i);
				glEnable(GL_LIGHTING);
			}
		}
	}
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cyan);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);
	drawFort();
	if (ball[ISLAND].fire) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cyan);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);
		glRotatef(-global.rAngleY0 * 180.0 / M_PI, 0.0, 1.0, 0.0);
		renderCannonball(ISLAND);
		glRotatef(global.rAngleY0 * 180.0 / M_PI, 0.0, 1.0, 0.0);
	}

	glDisable(GL_LIGHTING);
	glRotatef(-global.rAngleY * 180.0 / M_PI, 0.0, 1.0, 0.0);
	// draw cannon projectile prediction line
	drawTrajectoryIsland(island);
	glRotatef(global.rAngleY * 180.0 / M_PI, 0.0, 1.0, 0.0);
	glEnable(GL_LIGHTING);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, seaBlue);
	renderSea();

	if (global.wireframeMode) {
		glDisable(GL_LIGHTING);
		drawAxes(30.0);
		drawNormal();
	}

	renderOSD();

	glPopMatrix();
	global.frames++;
	glutSwapBuffers();

	/* check OpenGL error */
	while ((error = glGetError()) != GL_NO_ERROR)
		printf("%s\n", gluErrorString(error));
}

void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		camera.rotateStart = true;
		camera.btClicked = true;
	}
	else
		camera.rotateStart = false;
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		camera.scaleStart = true;
		camera.btClicked = true;
	}
	else
		camera.scaleStart = false;
}

void mouseMotion(int x, int y) {
	if (camera.rotateStart && global.start) {
		if (camera.btClicked) {
			camera.x0 = x;
			camera.y0 = y;
			camera.yOld = camera.y;
			camera.rAngleYOld = camera.rAngleY;
			camera.btClicked = false;
		}
		camera.rAngleY = (x - camera.x0) * M_PI / 60.0 + camera.rAngleYOld;
		camera.y = camera.yOld + (y - camera.y0) / 60.0;
	}
	if (camera.scaleStart) {
		if (camera.btClicked) {
			camera.x0 = x;
			camera.y0 = y;
			camera.xOld = camera.x;
			camera.destYOld = camera.destY;
			camera.btClicked = false;
		}
		camera.x = camera.xOld + (x - camera.x0) / 60.0;
		camera.destY = camera.destYOld + (y - camera.y0) / 60.0;
	}
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
	if (key == ESC)
		exit(EXIT_SUCCESS);
	else if(!global.win && !global.loose) {
		if (!global.start) {
			global.start = true;
			global.go = !global.go;
			global.startTime = glutGet(GLUT_ELAPSED_TIME) / MILLI;
			initialBoats();
		} else {
			switch (key) {
			case 'g':
				global.go = !global.go;
				if (!global.go)
					global.stopTime = glutGet(GLUT_ELAPSED_TIME) / MILLI;
				else {
					global.resumeTime = glutGet(GLUT_ELAPSED_TIME) / MILLI;
					global.pauseIntervel += global.resumeTime - global.stopTime;
				}
			case 'w':
				if (global.rAngle < M_PI)
					global.rAngle += M_PI / 180.0;
				break;
			case 's':
				if (global.rAngle > 0)
					global.rAngle -= M_PI / 180.0;
				break;
			case 'a':
				global.rAngleY -= M_PI / 60.0;
				break;
			case 'd':
				global.rAngleY += M_PI / 60.0;
				break;
			case '=':
				if (global.tessellation < MAX_TESSELLATION)
					global.tessellation *= 2;
				break;
			case '-':
				if (global.tessellation > MIN_TESSELLATION)
					global.tessellation /= 2;
				break;
			case SPACEBAR: // island fire
				if (!ball[ISLAND].fire) {
					ball[ISLAND].fire = true;
					ball[ISLAND].pv.p = { ISLAND_BALL_X, ISLAND_BALL_Y, ISLAND_BALL_Z };
					ball[ISLAND].pv.v = { ISLAND_BALL_VX, ISLAND_BALL_VY, ISLAND_BALL_VZ };
					// save the rotation angle Y of fort when cannonball fires as a saperate value for calculating if cannonball hit boat
					global.rAngleY0 = global.rAngleY;
				}
				break;
			default:
				break;
			}
		}
		glutPostRedisplay();
	}
}

void specialKey(int key, int x, int y) {
	if (key == GLUT_KEY_F1)
		global.wireframeMode = !global.wireframeMode;
	else if (!global.win && !global.loose) {
		if (!global.start) {
			global.start = true;
			global.go = !global.go;
			global.startTime = glutGet(GLUT_ELAPSED_TIME) / MILLI;
			initialBoats();
		}
		else {
			switch (key) {
			case GLUT_KEY_UP:
				if (global.rAngle < M_PI)
					global.rAngle += M_PI / 180.0;
				break;
			case GLUT_KEY_DOWN:
				if (global.rAngle > 0)
					global.rAngle -= M_PI / 180.0;
				break;
			case GLUT_KEY_LEFT:
				global.rAngleY -= M_PI / 60.0;
				break;
			case GLUT_KEY_RIGHT:
				global.rAngleY += M_PI / 60.0;
				break;
			default:
				break;
			}
		}
		glutPostRedisplay();
	}
}

bool init() {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_SMOOTH);

	srand((unsigned)time(0));
	for (int i = 0; i < MAX_BOAT_NUM; i++) {
		for (int k = 0; k < PARTICLE_NUM; k++) {
			particle[i][k].p.y = -100000.0;
			float angleX = float(rand() % PARTICLE_NUM) / PARTICLE_NUM * M_PI;
			float angleY = float(rand() % PARTICLE_NUM) / PARTICLE_NUM * 2.0 * M_PI;
			float v = PARTICLE_SPEED * float(10 + rand() % (PARTICLE_NUM - 10)) / PARTICLE_NUM;
			particle[i][k].v.x = v * cosf(angleX) * sinf(angleY);
			particle[i][k].v.y = v * sinf(angleX);
			particle[i][k].v.z = v * cosf(angleX) * cosf(angleY);
		}
	}

	glGenTextures(1, &textureTerrian);
	textureTerrian = loadTexture("terrian.jpg");
	if (!textureTerrian) {
		printf("No terrian texture created; exiting.\n");
		return false;
	}
	glGenTextures(6, textureSkybox);
	textureSkybox[0] = loadTexture("negx.jpg");
	textureSkybox[1] = loadTexture("negy.jpg");
	textureSkybox[2] = loadTexture("negz.jpg");
	textureSkybox[3] = loadTexture("posx.jpg");
	textureSkybox[4] = loadTexture("posy.jpg");
	textureSkybox[5] = loadTexture("posz.jpg");
	for (int i = 0; i < 6; i++) {
		if (!textureSkybox[i]) {
			printf("No skybox texture created; exiting.\n");
			return false;
		}
	}
	return true;
}

void reshape(int w, int h) {
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 100.0);
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Island Defender v1.0");
	if (!init())
		return EXIT_FAILURE;
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKey);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(update);
	glutMainLoop();
	return EXIT_SUCCESS;
}
