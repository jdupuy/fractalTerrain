////////////////////////////////////////////////////////////////////////////////
// \author   Jonathan Dupuy
//
////////////////////////////////////////////////////////////////////////////////

#define _ANT_ENABLE

// GL libraries
#include "glew.hpp"
#include "GL/freeglut.h"

#ifdef _ANT_ENABLE
#	include "AntTweakBar.h"
#endif // _ANT_ENABLE

// Custom libraries
#include "Framework.hpp"    // utility classes/functions for OpenGL
#include "Algebra.hpp"      // basic math library
#include "Transform.hpp"    // transformations for graphics

// Standard librabries
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>


////////////////////////////////////////////////////////////////////////////////
// Global variables
//
////////////////////////////////////////////////////////////////////////////////

const float PI = 3.14159265;

enum GLNames
{
	BUFFER_PLANE_VERTICES = 0,
	BUFFER_PLANE_INDEXES,
	BUFFER_COUNT,

	VERTEX_ARRAY_PLANE = 0,
	VERTEX_ARRAY_COUNT,

	PROGRAM_TERRAIN = 0,
	PROGRAM_COUNT
};

GLuint *buffers       = NULL;
GLuint *vertexArrays  = NULL;
GLuint *programs      = NULL;

Affine invCameraWorld       = Affine::Translation(Vector3(0,-1,-8));
Projection cameraProjection = Projection::Perspective(PI*0.25f,
                                                      1.0f,
                                                      0.125f,
                                                      8192.0f);

GLuint gridIndexCount = 0;
GLuint gridTessLevel  = 255;
GLfloat gridScale     = 2.0f;
GLfloat lightTheta    = 45.0f;
GLfloat lightRadius   = 3.0f;
GLfloat fBmH = 1.0f; // h param 
GLfloat fBmL = 2.0f; // lacunarity
GLint fBmO   = 10;    // octaves

bool wireframe = false;
bool normals   = false;

bool mouseLeft  = false;
bool mouseRight = false;

#ifdef _ANT_ENABLE

#endif // _ANT_ENABLE



////////////////////////////////////////////////////////////////////////////////
// Callbacks
//
////////////////////////////////////////////////////////////////////////////////

void create_plane_mesh() {
	const GLuint sVertexCnt = gridTessLevel+1;
	std::vector<Vector2> vertices;
	std::vector<GLushort> indexes;

	// allocate memory
	vertices.reserve(sVertexCnt*sVertexCnt);
	indexes.reserve(gridTessLevel*gridTessLevel*6);

#define GRID_VERTEX(x,y) Vector2(-0.5f+GLfloat(x)/GLfloat(gridTessLevel), \
                                 -0.5f+GLfloat(y)/GLfloat(gridTessLevel))

	// fill vertices
	for(GLuint x = 0; x < sVertexCnt; ++x)
		for(GLuint y = 0; y < sVertexCnt; ++y)
			vertices.push_back(GRID_VERTEX(x,y));

#undef GRID_VERTEX

	// fill indexes
	for(GLuint x = 0; x < gridTessLevel; ++x)
		for(GLuint y = 0; y < gridTessLevel; ++y) {
			// upper triangle
			indexes.push_back((x+1u) * sVertexCnt + (y+1u));
			indexes.push_back((x+1u) * sVertexCnt + y);
			indexes.push_back(x * sVertexCnt + y);

			// lower triangle
			indexes.push_back((x+1u) * sVertexCnt + (y+1u));
			indexes.push_back(x * sVertexCnt + y);
			indexes.push_back(x * sVertexCnt + y + 1u );
		}

	// fill buffers
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_PLANE_VERTICES]);
		glBufferSubData(GL_ARRAY_BUFFER,
		                0,
		                sizeof(Vector2)*vertices.size(),
		                &vertices[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_PLANE_INDEXES]);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
		                0,
		                sizeof(GLushort)*indexes.size(),
		                &indexes[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// save element cnt
	gridIndexCount = indexes.size();
}

void set_fBm_h() {
	glProgramUniform1f(programs[PROGRAM_TERRAIN],
	                   glGetUniformLocation(programs[PROGRAM_TERRAIN],
	                                        "uH"),
	                   fBmH);
}

void set_fBm_l() {
	glProgramUniform1f(programs[PROGRAM_TERRAIN],
	                   glGetUniformLocation(programs[PROGRAM_TERRAIN],
	                                        "uLacunarity"),
	                   fBmL);
}

void set_fBm_o() {
	glProgramUniform1i(programs[PROGRAM_TERRAIN],
	                   glGetUniformLocation(programs[PROGRAM_TERRAIN],
	                                        "uOctaves"),
	                   fBmO);
}

void set_grid_scale() {
	glProgramUniform1f(programs[PROGRAM_TERRAIN],
	                   glGetUniformLocation(programs[PROGRAM_TERRAIN],
	                                        "uGridScale"),
	                   gridScale);
}

#ifdef _ANT_ENABLE

static void TW_CALL get_grid_tess_level_cb(void *value, void *clientData){
	*(GLint *)value = gridTessLevel;
}

static void TW_CALL set_grid_tess_level_cb(const void *value, void *clientData){
	gridTessLevel = *(const GLint *)value;
	create_plane_mesh();
}

static void TW_CALL get_grid_scale_cb(void *value, void *clientData){
	*(GLfloat *)value = gridScale;
}

static void TW_CALL set_grid_scale_cb(const void *value, void *clientData){
	gridScale = *(const GLfloat *)value;
	set_grid_scale();
}

static void TW_CALL get_fBm_h_cb(void *value, void *clientData){
	*(GLfloat *)value = fBmH;
}

static void TW_CALL set_fBm_h_cb(const void *value, void *clientData){
	fBmH = *(const GLfloat *)value;
	set_fBm_h();
}

static void TW_CALL get_fBm_l_cb(void *value, void *clientData){
	*(GLfloat *)value = fBmL;
}

static void TW_CALL set_fBm_l_cb(const void *value, void *clientData){
	fBmL = *(const GLfloat *)value;
	set_fBm_l();
}

static void TW_CALL get_fBm_o_cb(void *value, void *clientData){
	*(GLint *)value = fBmO;
}

static void TW_CALL set_fBm_o_cb(const void *value, void *clientData){
	fBmO = *(const GLint *)value;
	set_fBm_o();
}

#endif


////////////////////////////////////////////////////////////////////////////////
// on init cb
void on_init() {
	// alloc names
	buffers       = new GLuint[BUFFER_COUNT];
	vertexArrays  = new GLuint[VERTEX_ARRAY_COUNT];
	programs      = new GLuint[PROGRAM_COUNT];

	// gen names
	glGenBuffers(BUFFER_COUNT, buffers);
	glGenVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		programs[i] = glCreateProgram();

	// buffers
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_PLANE_VERTICES]);
		glBufferData(GL_ARRAY_BUFFER,
		             sizeof(Vector2)*(1<<16),
		             NULL,
		             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_PLANE_INDEXES]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		             sizeof(GLushort)*((1<<8)-1)*((1<<8)-1)*6,
		             NULL,
		             GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// fill data
	create_plane_mesh();

	// vertex arrays
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_PLANE]);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_PLANE_VERTICES]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, FW_BUFFER_OFFSET(0));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_PLANE_INDEXES]);
	glBindVertexArray(0);


	// programs
	fw::build_glsl_program(programs[PROGRAM_TERRAIN],
	                       "terrain.glsl",
	                       "",
	                       GL_TRUE);

	// set uniforms
	set_grid_scale();
	set_fBm_h();
	set_fBm_o();
	set_fBm_l();

	glClearColor(0.25, 0.25, 0.25, 1.0);
	glEnable(GL_DEPTH_TEST);
//	glEnable(GL_CULL_FACE);
	fw::check_gl_error();

#ifdef _ANT_ENABLE
	// start ant
	TwInit(TW_OPENGL, NULL);
	// send the ''glutGetModifers'' function pointer to AntTweakBar
	TwGLUTModifiersFunc(glutGetModifiers);

	TwBar *bar = TwNewBar("menu");
	TwDefine("menu label='menu' size='175 260'");
	TwAddVarRW(bar,
	           "wireframe",
	           TW_TYPE_BOOLCPP,
	           &wireframe,
	           "true='ON' false='OFF'");
	TwAddVarRW(bar,
	           "normals",
	           TW_TYPE_BOOLCPP,
	           &normals,
	           "true='ON' false='OFF'");
	TwAddVarCB(bar,
	           "grid tess",
	           TW_TYPE_UINT32,
	           &set_grid_tess_level_cb,
	           &get_grid_tess_level_cb,
	           NULL,
	           "min=1 max=255 step=1");
	TwAddVarCB(bar,
	           "grid scale",
	           TW_TYPE_FLOAT,
	           &set_grid_scale_cb,
	           &get_grid_scale_cb,
	           NULL,
	           "min=1 max=255 step=1");
	TwAddVarRW(bar,
	           "theta",
	           TW_TYPE_FLOAT,
	           &lightTheta,
	           "group='Light' min=0.1 max=180 step=1 group='light'");
	TwAddVarRW(bar,
	           "radius",
	           TW_TYPE_FLOAT,
	           &lightRadius,
	           "group='Light' min=0.0 max=100 step=0.1 group='light'");
	TwAddVarCB(bar,
	           "H",
	           TW_TYPE_FLOAT,
	           &set_fBm_h_cb,
	           &get_fBm_h_cb,
	           NULL,
	           "min=0.0 max=1.0 step=0.01 group='fBm'");
	TwAddVarCB(bar,
	           "Lacunarity",
	           TW_TYPE_FLOAT,
	           &set_fBm_l_cb,
	           &get_fBm_l_cb,
	           NULL,
	           "min=0.0 max=5.0 step=0.01 group='fBm'");
	TwAddVarCB(bar,
	           "Octaves",
	           TW_TYPE_INT32,
	           &set_fBm_o_cb,
	           &get_fBm_o_cb,
	           NULL,
	           "min=1 max=20 step=1 group='fBm'");

#endif // _ANT_ENABLE
}


////////////////////////////////////////////////////////////////////////////////
// on clean cb
void on_clean() {
	// delete objects
	glDeleteBuffers(BUFFER_COUNT, buffers);
	glDeleteVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		glDeleteProgram(programs[i]);

	// release memory
	delete[] buffers;
	delete[] vertexArrays;
	delete[] programs;

#ifdef _ANT_ENABLE
	TwTerminate();
#endif // _ANT_ENABLE
	fw::check_gl_error();
}


////////////////////////////////////////////////////////////////////////////////
// on update cb
void on_update() {
	GLint windowWidth  = glutGet(GLUT_WINDOW_WIDTH);
	GLint windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
	float aspect = float(windowWidth)/float(windowHeight);

	// update transformations
	cameraProjection.FitHeightToAspect(aspect);
	Matrix4x4 mvp = cameraProjection.ExtractTransformMatrix()
	              * invCameraWorld.ExtractTransformMatrix();

	// camera position
	Matrix4x4 tmp  = invCameraWorld.ExtractInverseTransformMatrix();
	Vector3 camPos = Vector3(tmp[3][0], tmp[3][1], tmp[3][2]);

	// update light position
	Vector3 lightPos(0,0,0);
	lightPos[0] = lightRadius * cos(PI*lightTheta/180.0f);
	lightPos[1] = lightRadius * sin(PI*lightTheta/180.0f);

	// update uniforms
	glProgramUniformMatrix4fv(programs[PROGRAM_TERRAIN],
	                          glGetUniformLocation(programs[PROGRAM_TERRAIN],
	                                               "uModelViewProjection"),
	                          1,
	                          0,
	                          reinterpret_cast<const float * >(&mvp));
	glProgramUniform3fv(programs[PROGRAM_TERRAIN],
	                    glGetUniformLocation(programs[PROGRAM_TERRAIN],
	                                         "uLightPos"),
	                    1,
	                    reinterpret_cast<const float * >(&lightPos));
	glProgramUniform3fv(programs[PROGRAM_TERRAIN],
	                    glGetUniformLocation(programs[PROGRAM_TERRAIN],
	                                         "uEyePos"),
	                    1,
	                    reinterpret_cast<const float * >(&camPos));

	// draw
	glViewport(0,0,windowWidth, windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// draw boundaries of the gaussian
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_PLANE]);
	glUseProgram(programs[PROGRAM_TERRAIN]);
	glDrawElements(GL_TRIANGLES,
	               gridIndexCount,
	               GL_UNSIGNED_SHORT,
	               FW_BUFFER_OFFSET(0));

	if(wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#ifdef _ANT_ENABLE
	glUseProgram(0);
	glBindVertexArray(0);
	TwDraw();
#endif // _ANT_ENABLE
	fw::check_gl_error();

	glutSwapBuffers();
	glutPostRedisplay();
}


////////////////////////////////////////////////////////////////////////////////
// on resize cb
void on_resize(GLint w, GLint h) {
#ifdef _ANT_ENABLE
	TwWindowSize(w, h);
#endif
}


////////////////////////////////////////////////////////////////////////////////
// on key down cb
void on_key_down(GLubyte key, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1==TwEventKeyboardGLUT(key, x, y))
		return;
#endif

	if(key=='p')
		fw::save_gl_front_buffer(0,
		                         0,
		                         glutGet(GLUT_WINDOW_WIDTH),
		                         glutGet(GLUT_WINDOW_HEIGHT));
	if(key=='w')
		wireframe = !wireframe;
	if(key=='f')
		glutFullScreenToggle();
	if(key==27) // escape
		glutLeaveMainLoop();
}


////////////////////////////////////////////////////////////////////////////////
// on key up cb
void on_key_up(GLubyte key, GLint x, GLint y) {
#ifdef _ANT_ENABLE
//	if(1==TwEventKeyboardGLUT(key, x, y))
//		return;
#endif
}


////////////////////////////////////////////////////////////////////////////////
// on mouse button cb
void on_mouse_button(GLint button, GLint state, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwEventMouseButtonGLUT(button, state, x, y))
		return;
#endif // _ANT_ENABLE
	if(state==GLUT_DOWN) {
		mouseLeft  |= button == GLUT_LEFT_BUTTON;
		mouseRight |= button == GLUT_RIGHT_BUTTON;
	}
	else {
		mouseLeft  &= button == GLUT_LEFT_BUTTON ? false : mouseLeft;
		mouseRight  &= button == GLUT_RIGHT_BUTTON ? false : mouseRight;
	}
	if(button == 3)
		invCameraWorld.TranslateWorld(Vector3(0,0,0.15f));
	if(button == 4)
		invCameraWorld.TranslateWorld(Vector3(0,0,-0.15f));
}


////////////////////////////////////////////////////////////////////////////////
// on mouse motion cb
void on_mouse_motion(GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwEventMouseMotionGLUT(x,y))
		return;
#endif // _ANT_ENABLE

	static GLint sMousePreviousX = 0;
	static GLint sMousePreviousY = 0;
	const GLint MOUSE_XREL = x-sMousePreviousX;
	const GLint MOUSE_YREL = y-sMousePreviousY;
	sMousePreviousX = x;
	sMousePreviousY = y;

	if(mouseLeft) {
		invCameraWorld.RotateAboutWorldX(-0.01f*MOUSE_YREL);
		invCameraWorld.RotateAboutLocalY( 0.01f*MOUSE_XREL);
	}
	if(mouseRight) {
		invCameraWorld.TranslateWorld(Vector3( 0.01f*MOUSE_XREL,
		                                      -0.01f*MOUSE_YREL,
		                                       0));
	}
}


////////////////////////////////////////////////////////////////////////////////
// on mouse wheel cb
void on_mouse_wheel(GLint wheel, GLint direction, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwMouseWheel(wheel))
		return;
#endif // _ANT_ENABLE
	invCameraWorld.TranslateWorld(GLfloat(direction)*Vector3(0,0,0.15f));
}


////////////////////////////////////////////////////////////////////////////////
// Main
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	const GLuint CONTEXT_MAJOR = 4;
	const GLuint CONTEXT_MINOR = 1;

	// init glut
	glutInit(&argc, argv);
	glutInitContextVersion(CONTEXT_MAJOR ,CONTEXT_MINOR);
#ifdef _ANT_ENABLE
	glutInitContextFlags(GLUT_DEBUG);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
#else
	glutInitContextFlags(GLUT_DEBUG | GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
#endif

	// build window
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(1024, 768);
	glutInitWindowPosition(80, 40);
	glutCreateWindow("OpenGL");

	// init glew
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(GLEW_OK != err) {
		std::stringstream ss;
		ss << err;
		std::cerr << "glewInit() gave error " << ss.str() << std::endl;
		return 0;
	}

	// glewInit generates an INVALID_ENUM error for some reason...
	glGetError();

	fw::check_gl_error();

	// set callbacks
	glutCloseFunc(&on_clean);
	glutReshapeFunc(&on_resize);
	glutDisplayFunc(&on_update);
	glutKeyboardFunc(&on_key_down);
	glutKeyboardUpFunc(&on_key_up);
	glutMouseFunc(&on_mouse_button);
	glutPassiveMotionFunc(&on_mouse_motion);
	glutMotionFunc(&on_mouse_motion);
	glutMouseWheelFunc(&on_mouse_wheel);

	// run
	try {
		// run demo
		on_init();
		glutMainLoop();
	}
	catch(std::exception& e) {
		std::cerr << "Fatal exception: " << e.what() << std::endl;
		return 0;
	}

	return 1;
}



