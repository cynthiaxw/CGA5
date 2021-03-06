// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Co-Authors:
//			Jeremy Hart, University of Calgary
//			John Hall, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "texture.h"
#include "Camera.h"
#include <vector>

using namespace std;
using namespace glm;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

#define PI_F 3.14159265359f

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

bool lbPushed = false;

float ROTATION_SCALER = 50.f;

#define DIS_E_S 149.2f	// Million km
#define PLANET_SIZE_SCALER 24.f
#define PLANET_REVO_SCALER 1.f
#define PLANET_REVO_RADIUS_SCALER 500.f
#define SCALER_STAR 10.f

float SCALER_SUN = 0.1f;//8/PLANET_SIZE_SCALER;
float SCALER_JUPITER = 2/PLANET_SIZE_SCALER;//6.9/PLANET_SIZE_SCALER;
float SCALER_SATURN = 1.7/PLANET_SIZE_SCALER;//6.0268/PLANET_SIZE_SCALER;
float SCALER_URANUS = 0.731/PLANET_SIZE_SCALER;//2.5559/PLANET_SIZE_SCALER;
float SCALER_NEPTUNE = 0.7076/PLANET_SIZE_SCALER;//2.4764/PLANET_SIZE_SCALER;
float SCALER_EARTH = 0.63781/PLANET_SIZE_SCALER;
float SCALER_VENUS = 0.60518/PLANET_SIZE_SCALER;
float SCALER_MARS = 0.33962/PLANET_SIZE_SCALER;
float SCALER_MERCURY = 0.24397/PLANET_SIZE_SCALER;
float SCALER_MOON = 0.17381/PLANET_SIZE_SCALER;
float SCALER_PLUTO = 0.1195/PLANET_SIZE_SCALER;

float EARTH_REVOLUTION = 365 * PLANET_REVO_SCALER;
float MOON_REVOLUTION = 27.3 * PLANET_REVO_SCALER;
float MARS_REVOLUTION = 687 * PLANET_REVO_SCALER;
float MERCURY_REVOLUTION = 87.96 * PLANET_REVO_SCALER;
float VENUS_REVOLUTION = 224.7 * PLANET_REVO_SCALER;
float JUPITER_REVOLUTION = 11.86 * 365 * PLANET_REVO_SCALER;
float SATURN_REVOLUTION = 29.5 * 365 * PLANET_REVO_SCALER;
float URANUS_REVOLUTION = 84 * 365 * PLANET_REVO_SCALER;
float NEPTUNE_REVOLUTION = 164.8 * 365 * PLANET_REVO_SCALER;

#define SUN_ROTATION 17.3f
#define EARTH_ROTATION 1.f
#define MARS_ROTATION 1.f
#define MERCURY_ROTATION 58.65f
#define VENUS_ROTATION 243.02f
#define JUPITER_ROTATION 0.41f
#define SATURN_ROTATION 0.42f
#define URANUS_ROTATION 0.6458f
#define NEPTUNE_ROTATION 0.9167f

float MERCURY_REVO_RADIUS = 57.9/PLANET_REVO_RADIUS_SCALER;
float VENUS_REVO_RADIUS = 108.2/PLANET_REVO_RADIUS_SCALER;
float EARTH_REVO_RADIUS = 149.6/PLANET_REVO_RADIUS_SCALER;
float MARS_REVO_RADIUS = 227.9/PLANET_REVO_RADIUS_SCALER;
float JUPITER_REVO_RADIUS = 300.3/PLANET_REVO_RADIUS_SCALER;//778.3/PLANET_REVO_RADIUS_SCALER;
float SATURN_REVO_RADIUS = 500/PLANET_REVO_RADIUS_SCALER;//1427/PLANET_REVO_RADIUS_SCALER;
float URANUS_REVO_RADIUS = 600/PLANET_REVO_RADIUS_SCALER;//2882.3/PLANET_REVO_RADIUS_SCALER;
float NEPTUNE_REVO_RADIUS = 700/PLANET_REVO_RADIUS_SCALER;//4523.9/PLANET_REVO_RADIUS_SCALER;

#define MOON_ROTATION 27.f
#define MOON_REVO_RADIUS 0.04f

#define SCALER_CAM_RADIUS 0.03f
float cam_max_r, cam_min_r;
float cam_phi, cam_theta;
Camera cam;


int planet_mode = 1;
// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

// load, compile, and link shaders, returning true if successful
GLuint InitializeShaders()
{
	// load shader source from files
	string vertexSource = LoadSource("shaders/vertex.glsl");
	string fragmentSource = LoadSource("shaders/fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	GLuint program = LinkProgram(vertex, fragment);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	// check for OpenGL errors and return false if error occurred
	return program;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct Geometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	Geometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

bool InitializeVAO(Geometry *geometry){

	const GLuint VERTEX_INDEX = 0;
	const GLuint TEXCOORD_INDEX = 1;

	//Generate Vertex Buffer Objects
	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	// create an array buffer object for storing our texture coordicates
	glGenBuffers(1, &geometry->textureBuffer);

	//Set up Vertex Array Object
	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(
		VERTEX_INDEX,		//Attribute index 
		3, 					//# of components
		GL_FLOAT, 			//Type of component
		GL_FALSE, 			//Should be normalized?
		sizeof(vec3),		//Stride - can use 0 if tightly packed
		0);					//Offset to first element
	glEnableVertexAttribArray(VERTEX_INDEX);

	// associate the texture array with the texture coordinates array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glVertexAttribPointer(
		TEXCOORD_INDEX,		//Attribute index
		2, 					//# of components
		GL_FLOAT, 			//Type of component
		GL_FALSE, 			//Should be normalized?
		sizeof(vec2), 		//Stride - can use 0 if tightly packed
		0);					//Offset to first element
	glEnableVertexAttribArray(TEXCOORD_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return !CheckGLErrors();
}

// create buffers and fill with geometry data, returning true if successful
bool LoadGeometry(Geometry *geometry, vec3 *vertices, vec2 *textures, int elementCount)
{
	geometry->elementCount = elementCount;

	// create an array buffer object for storing our vertices
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*geometry->elementCount, vertices, GL_STATIC_DRAW);

	// create another one for storing our texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2)*geometry->elementCount, textures, GL_STATIC_DRAW);

	//Unbind buffer to reset to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(Geometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyTexture* tex, Geometry *geometry, GLuint program, Camera* camera, mat4 perspectiveMatrix, mat4 wMp, GLenum rendermode, int shadeflg, int nightflg, MyTexture* nighttex)
{

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(program);

	int vp [4];
	glGetIntegerv(GL_VIEWPORT, vp);
	int width = vp[2];
	int height = vp[3];


	//Bind uniforms
	GLint uniformLocation;

	mat4 modelViewProjection = perspectiveMatrix*camera->viewMatrix();
	uniformLocation = glGetUniformLocation(program, "modelViewProjection");
	glUniformMatrix4fv(uniformLocation, 1, false, glm::value_ptr(modelViewProjection));

	uniformLocation = glGetUniformLocation(program, "modelMatrix");
	glUniformMatrix4fv(uniformLocation, 1, false, glm::value_ptr(wMp));

	uniformLocation = glGetUniformLocation(program, "shade_flg");
	glUniform1i(uniformLocation, shadeflg);

	uniformLocation = glGetUniformLocation(program, "night_flg");
	glUniform1i(uniformLocation, nightflg);

	glBindVertexArray(geometry->vertexArray);
	glBindTexture(tex->target, tex->textureID);
	glDrawArrays(rendermode, 0, geometry->elementCount);

	glBindVertexArray(geometry->vertexArray);
	glBindTexture(nighttex->target, nighttex->textureID);
	glDrawArrays(rendermode, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

void RenderEarth(MyTexture* tex, Geometry *geometry, GLuint program, Camera* camera, mat4 perspectiveMatrix, mat4 wMp, GLenum rendermode, int shadeflg, int nightflg, MyTexture* nighttex, MyTexture* spectex)
{

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(program);

	int vp [4];
	glGetIntegerv(GL_VIEWPORT, vp);
	int width = vp[2];
	int height = vp[3];


	//Bind uniforms
	GLint uniformLocation;

	mat4 modelViewProjection = perspectiveMatrix*camera->viewMatrix();
	uniformLocation = glGetUniformLocation(program, "modelViewProjection");
	glUniformMatrix4fv(uniformLocation, 1, false, glm::value_ptr(modelViewProjection));

	uniformLocation = glGetUniformLocation(program, "modelMatrix");
	glUniformMatrix4fv(uniformLocation, 1, false, glm::value_ptr(wMp));

	uniformLocation = glGetUniformLocation(program, "shade_flg");
	glUniform1i(uniformLocation, shadeflg);

	uniformLocation = glGetUniformLocation(program, "night_flg");
	glUniform1i(uniformLocation, nightflg);

	glBindVertexArray(geometry->vertexArray);
	glBindTexture(tex->target, tex->textureID);
	glDrawArrays(rendermode, 0, geometry->elementCount);

	glBindVertexArray(geometry->vertexArray);
	glBindTexture(nighttex->target, nighttex->textureID);
	glDrawArrays(rendermode, 0, geometry->elementCount);

	glBindVertexArray(geometry->vertexArray);
	glBindTexture(spectex->target, spectex->textureID);
	glDrawArrays(rendermode, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions
int pause_flg = 0;

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && pause_flg == 0)
		pause_flg = 1;		
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && pause_flg == 1)
		pause_flg = 0;
	else if(key == GLFW_KEY_1 && action == GLFW_PRESS){
		planet_mode = 1;
	}
	else if(key == GLFW_KEY_2 && action == GLFW_PRESS){
		planet_mode = 2;
	}
	else if(key == GLFW_KEY_3 && action == GLFW_PRESS){
		planet_mode = 3;
	}
	else if(key == GLFW_KEY_4 && action == GLFW_PRESS){
		planet_mode = 4;
	}
	else if(key == GLFW_KEY_5 && action == GLFW_PRESS){
		planet_mode = 5;
	}
	else if(key == GLFW_KEY_6 && action == GLFW_PRESS){
		planet_mode = 6;
	}
	else if(key == GLFW_KEY_7 && action == GLFW_PRESS){
		planet_mode = 7;
	}
	else if(key == GLFW_KEY_8 && action == GLFW_PRESS){
		planet_mode = 8;
	}
	else if(key == GLFW_KEY_9 && action == GLFW_PRESS){
		planet_mode = 9;
	}
	else if(key == GLFW_KEY_0 && action == GLFW_PRESS){
		planet_mode = 0;
	}


	else if(key == GLFW_KEY_W && action == GLFW_PRESS){
		ROTATION_SCALER *= 0.9f;
	}

	else if(key == GLFW_KEY_S && action == GLFW_PRESS){
		ROTATION_SCALER *= 1.4f;
	}

	else if(key == GLFW_KEY_R && action == GLFW_PRESS){
		ROTATION_SCALER = 50.f;
	}
}

void  scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	cam.radius -= yoffset * SCALER_CAM_RADIUS;
	if(cam.radius > cam_max_r) cam.radius = cam_max_r;
	else if(cam.radius < cam_min_r) cam.radius = cam_min_r;
	cam.pos.y = cam.radius * cos(cam_phi);
	cam.pos.x = cam.radius * sin(cam_phi) * cos(cam_theta);
	cam.pos.z = cam.radius * sin(cam_phi) * sin(cam_theta);
}


void planetMaker(vector<vec3>* sphere, vector<vec2>* texCoord, int n){
	float step = PI_F/n;
	float p = 0;
	float t = step;
	float ty = 1.f - 1.f/n;
	float tx = 0;
	for(int i = 0; i<n-2; i++){
		p = 0;
		tx = 0;
		for(int j = 0; j<2*n; j++){
			float t1 = t + step;
			float p1 = p + step;
			float tx1 = tx + 0.5f/n;
			float ty1 = ty - 1.f/n;
			sphere->push_back(vec3(sin(t)*cos(p), cos(t), sin(t)*sin(p)));
			sphere->push_back(vec3(sin(t1)*cos(p), cos(t1), sin(t1)*sin(p)));
			sphere->push_back(vec3(sin(t1)*cos(p1), cos(t1), sin(t1)*sin(p1)));
			
			sphere->push_back(vec3(sin(t)*cos(p), cos(t), sin(t)*sin(p)));
			sphere->push_back(vec3(sin(t1)*cos(p1), cos(t1), sin(t1)*sin(p1)));
			sphere->push_back(vec3(sin(t)*cos(p1), cos(t), sin(t)*sin(p1)));

			texCoord->push_back(vec2(tx, ty));
			texCoord->push_back(vec2(tx, ty1));
			texCoord->push_back(vec2(tx1, ty1));

			texCoord->push_back(vec2(tx, ty));
			texCoord->push_back(vec2(tx1, ty1));
			texCoord->push_back(vec2(tx1, ty));

			p+=step;
			tx+=0.5f/n;
		}
		t+=step;
		ty-=1.f/n;
	}
	// North pole and south pole
	p = 0;
	tx = 0;
	for(int j = 0; j<2*n; j++){
		 	t = step; ty = 1.f - 1.f/n;
			sphere->push_back(vec3(sin(t)*cos(p), cos(t), sin(t)*sin(p)));
			sphere->push_back(vec3(sin(t)*cos(p+step), cos(t), sin(t)*sin(p+step)));
			sphere->push_back(vec3(0.f, 1.f, 0.f));

			texCoord->push_back(vec2(tx, ty));
			texCoord->push_back(vec2(tx+0.5f/n, ty));
			texCoord->push_back(vec2(tx, 1.f));

			t = PI_F - step; ty = 1.f/n;
			sphere->push_back(vec3(sin(t)*cos(p), cos(t), sin(t)*sin(p)));
			sphere->push_back(vec3(sin(t)*cos(p+step), cos(t), sin(t)*sin(p+step)));
			sphere->push_back(vec3(0.f, -1.f, 0.f));

			texCoord->push_back(vec2(tx, ty));
			texCoord->push_back(vec2(tx+0.5f/n, ty));
			texCoord->push_back(vec2(tx, 0.f));

			p+=step;
			tx+=0.5f/n;
	}
}

void generateRing(vector<vec3>* ring, vector<vec2>* texCoord){
	float step = 2*PI_F/128.f;
	float in_r = 67300.f/60300.f;
	float out_r = 140300.f/60300.f;
	for(float i = 0; i< 2*PI_F; i+=step){
		vec3 p1 = vec3(cos(i),0,sin(i)) * in_r;
		vec3 p2 = vec3(cos(i),0,sin(i)) * out_r;
		vec3 p3 = vec3(cos(i+step),0,sin(i+step)) * in_r;
		vec3 p4 = vec3(cos(i+step),0,sin(i+step)) * out_r;
		ring->push_back(p1);
		ring->push_back(p2);
		ring->push_back(p3);
		ring->push_back(p3);
		ring->push_back(p2);
		ring->push_back(p4);

		texCoord->push_back(vec2(0.1,0));
		texCoord->push_back(vec2(1,0));
		texCoord->push_back(vec2(0.1,1));
		texCoord->push_back(vec2(0.1,1));
		texCoord->push_back(vec2(1,0));
		texCoord->push_back(vec2(1,1));
	}
}

void debug3(char* s, vec3 v){
	cout << s << endl;
	cout << v.x << "," << v.y << "," <<v.z << endl;
}


// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	int width = 1024, height = 1024;
	window = glfwCreateWindow(width, height, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwMakeContextCurrent(window);

	//Intialize GLAD
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	GLuint program = InitializeShaders();
	if (program == 0) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}
	GLuint program1 = InitializeShaders();


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// three vertex positions and assocated colours of a triangle
	//Fill in with Perspective Matrix
	//mat4(1.f) identity matrix
	mat4 perspectiveMatrix = glm::perspective(PI_F*0.4f, float(width)/float(height), 0.0001f, 20.f);	//last 2 arg, nearst and farest

//----------------------- Generate Planets ---------------------------//
	vector<vec3> Planet;		//vertices
	vector<vec2> planetTex;	//texture
	planetMaker(&Planet, &planetTex, 128);

	Geometry geometry_sun;
	Geometry geometry_earth;
	Geometry geometry_star;
	Geometry geometry_moon;
	Geometry geometry_mars;
	Geometry geometry_mercury;
	Geometry geometry_venus;
	Geometry geometry_jupiter;
	Geometry geometry_saturn;
	Geometry geometry_uranus;
	Geometry geometry_neptune;
	Geometry geometry_saturn_ring;


	// call function to create and fill buffers with geometry data
	if (!InitializeVAO(&geometry_sun))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_sun, Planet.data(), planetTex.data(),Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_earth))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_earth, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;	

	if (!InitializeVAO(&geometry_star))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_star, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_moon))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_moon, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_mars))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_mars, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_mercury))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_mercury, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_venus))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_venus, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_jupiter))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_jupiter, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_saturn))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_saturn, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_uranus))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_uranus, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_neptune))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_neptune, Planet.data(), planetTex.data(), Planet.size()))
		cout << "Failed to load geometry" << endl;


	vector<vec3> ring;
	vector<vec2> ringtex;
	generateRing(&ring, &ringtex);
	if (!InitializeVAO(&geometry_saturn_ring))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_saturn_ring, ring.data(), ringtex.data(), ringtex.size()))
		cout << "Failed to load geometry" << endl;



	mat4 wMs, wMe, wMmoon, wMmars, wMmercury, wMjupiter, wMsaturn, wMuranus, wMvenus, wMneptune;
	mat4 wMstar = mat4(SCALER_STAR * vec4(1,0,0,0), SCALER_STAR * vec4(0,1,0,0), SCALER_STAR * vec4(0,0,1,0), vec4(0,0,0,1));

//----------------------- Generate Planets ---------------------------//





	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);


	vec2 lastCursorPos;

	float cursorSensitivity = PI_F/500.f;	//PI/hundred pixels

	//------------------------- Bind texture ------------------------//

	MyTexture texture_sun, texture_earth, texture_star, texture_moon, texture_earthnight;
	MyTexture texture_mars, texture_venus, texture_mercury, texture_saturn, texture_jupiter, texture_uranus, texture_neptune, texture_saturn_ring, texture_earth_spec_map;
	InitializeTexture(&texture_sun, "2k_sun.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_earth, "2k_earth_daymap.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_star, "8k_stars_milky_way.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_moon, "2k_moon.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_earthnight, "2k_earth_nightmap.jpg", GL_TEXTURE_2D);
	//InitializeTexture(&texture_earthnight, "spec.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_mars, "2k_mars.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_mercury, "2k_mercury.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_neptune, "2k_neptune.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_jupiter, "2k_jupiter.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_saturn, "2k_saturn.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_uranus, "2k_uranus.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_venus, "2k_venus_atmosphere.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_saturn_ring, "2k_saturn_ring_alpha.png", GL_TEXTURE_2D);
	InitializeTexture(&texture_earth_spec_map, "spec.jpg", GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_sun.textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_earth.textureID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture_star.textureID);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texture_moon.textureID);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, texture_earthnight.textureID);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, texture_mars.textureID);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, texture_venus.textureID);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, texture_mercury.textureID);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, texture_jupiter.textureID);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, texture_saturn.textureID);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, texture_uranus.textureID);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, texture_neptune.textureID);
	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_2D, texture_saturn_ring.textureID);
	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, texture_earth_spec_map.textureID);


	//------------------------- Bind texture ------------------------//


	float timer = 0.f;
	float sunTimer = 0.f;
	float earthTimer = 0.f;
	float moonTimer = 0.f;
	float marsTimer = 0.f;
	float mercuryTimer = 0.f;
	float venusTimer = 0.f;
	float jupiterTimer = 0.f;
	float saturnTimer = 0.f;
	float uranusTimer = 0.f;
	float neptuneTimer = 0.f;

	float earthRevoTimer = 0.f;
	float moonRevoTimer = 0.f;
	float marsRevoTimer = 0.f;
	float venusRevoTimer = 0.f;
	float mercuryRevoTimer = 0.f;
	float jupiterRevoTimer = 0.f;
	float saturnRevoTimer = 0.f;
	float uranusRevoTimer = 0.f;
	float neptuneRevoTimer = 0.f;

	float tilt = -23.5f/180.f * PI_F;

	cam.radius = SCALER_SUN + 0.7f;

	cam_max_r = SCALER_SUN + 3.f;
	cam_min_r = SCALER_SUN + 0.1f;

	cam_phi = PI_F/2.f;
	cam_theta = 0;
	cam.pos.y = cam.radius * cos(cam_phi);
	cam.pos.x = cam.radius * abs(sin(cam_phi)) * cos(cam_theta);
	cam.pos.z = cam.radius * abs(sin(cam_phi)) * sin(cam_theta);
	
	float cam_scaler = 1;

	mat3 Rotation;
	vec3 Transition;
	vec3 cam_transition;

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		if(pause_flg == 0){
			// Time
			if(sunTimer >= 2*PI_F){
				sunTimer = 0;
			}else sunTimer += 2*PI_F/SUN_ROTATION/ROTATION_SCALER;

			if(earthTimer >= 2*PI_F){
				earthTimer = 0;
			}else earthTimer += 2*PI_F/EARTH_ROTATION/ROTATION_SCALER;
			
			if(earthRevoTimer >= 2*PI_F){
				earthRevoTimer = 0;
			}else earthRevoTimer += 2*PI_F/EARTH_REVOLUTION/ROTATION_SCALER;

			if(moonTimer >= 2*PI_F){
				moonTimer = 0;
			}else moonTimer += 2*PI_F/MOON_ROTATION/ROTATION_SCALER;

			if(moonRevoTimer >= 2*PI_F){
				moonRevoTimer = 0;
			}else moonRevoTimer += 2*PI_F/MOON_REVOLUTION/ROTATION_SCALER;

			if(marsTimer >= 2*PI_F){
				marsTimer = 0;
			}else marsTimer += 2*PI_F/MARS_ROTATION/ROTATION_SCALER;
			
			if(marsRevoTimer >= 2*PI_F){
				marsRevoTimer = 0;
			}else marsRevoTimer += 2*PI_F/MARS_REVOLUTION/ROTATION_SCALER;

			if(venusTimer >= 2*PI_F){
				venusTimer = 0;
			}else venusTimer += 2*PI_F/VENUS_ROTATION/ROTATION_SCALER;
			
			if(venusRevoTimer >= 2*PI_F){
				venusRevoTimer = 0;
			}else venusRevoTimer += 2*PI_F/VENUS_REVOLUTION/ROTATION_SCALER;

			if(mercuryTimer >= 2*PI_F){
				mercuryTimer = 0;
			}else mercuryTimer += 2*PI_F/MERCURY_ROTATION/ROTATION_SCALER;
			
			if(mercuryRevoTimer >= 2*PI_F){
				mercuryRevoTimer = 0;
			}else mercuryRevoTimer += 2*PI_F/MERCURY_REVOLUTION/ROTATION_SCALER;

			if(jupiterTimer >= 2*PI_F){
				jupiterTimer = 0;
			}else jupiterTimer += 2*PI_F/JUPITER_ROTATION/ROTATION_SCALER;
			
			if(jupiterRevoTimer >= 2*PI_F){
				jupiterRevoTimer = 0;
			}else jupiterRevoTimer += 2*PI_F/JUPITER_REVOLUTION/ROTATION_SCALER;

			if(saturnTimer >= 2*PI_F){
				saturnTimer = 0;
			}else saturnTimer += 2*PI_F/SATURN_ROTATION/ROTATION_SCALER;
			
			if(saturnRevoTimer >= 2*PI_F){
				saturnRevoTimer = 0;
			}else saturnRevoTimer += 2*PI_F/SATURN_REVOLUTION/ROTATION_SCALER;

			if(neptuneTimer >= 2*PI_F){
				neptuneTimer = 0;
			}else neptuneTimer += 2*PI_F/NEPTUNE_ROTATION/ROTATION_SCALER;
			
			if(neptuneRevoTimer >= 2*PI_F){
				neptuneRevoTimer = 0;
			}else neptuneRevoTimer += 2*PI_F/NEPTUNE_REVOLUTION/ROTATION_SCALER;

		}

		// Planet movement

		// Sun
		timer = sunTimer;
		Rotation = SCALER_SUN * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));
		Transition = vec3(0,0,0);
		wMs = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Earth
		timer = earthTimer; 
		Rotation = SCALER_EARTH * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));
		tilt = -23.5f/180.f * PI_F;
		mat3 earthTilt = mat3(vec3(cos(tilt), sin(tilt), 0), vec3(-sin(tilt), cos(tilt), 0), vec3(0,0,1));
		Rotation =  earthTilt * Rotation;

		timer = earthRevoTimer;
		Transition = EARTH_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMe = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Moon
		timer = moonTimer;
		Rotation = SCALER_MOON * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));
		tilt = 6.8f/180.f * PI_F;
		mat3 moonTilt = mat3(vec3(cos(tilt), sin(tilt), 0), vec3(-sin(tilt), cos(tilt), 0), vec3(0,0,1));
		Rotation = moonTilt * Rotation;
		
		timer = moonRevoTimer;
		tilt = 5.f/180.f * PI_F;
		mat3 moon_orbital = mat3(vec3(cos(tilt), sin(tilt), 0), vec3(-sin(tilt), cos(tilt), 0), vec3(0,0,1));
		Transition = moon_orbital * MOON_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		mat4 eMmoon = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));;	// Moon to earth
		wMmoon = mat4(vec4(1,0,0,0),vec4(0,1,0,0),vec4(0,0,1,0),wMe[3]) * eMmoon;

		// Mars
		timer = marsTimer; 
		Rotation = SCALER_MARS * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));

		timer = marsRevoTimer;
		Transition = MARS_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMmars = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Mercury
		timer = mercuryTimer; 
		Rotation = SCALER_MERCURY * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));

		timer = mercuryRevoTimer;
		Transition = MERCURY_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMmercury = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Venus
		timer = venusTimer; 
		Rotation = SCALER_VENUS * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));

		timer = venusRevoTimer;
		Transition = VENUS_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMvenus = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Jupiter
		timer = jupiterTimer; 
		Rotation = SCALER_JUPITER * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));

		timer = jupiterRevoTimer;
		Transition = JUPITER_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMjupiter = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Saturn
		timer = saturnTimer; 
		Rotation = SCALER_SATURN * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));

		timer = saturnRevoTimer;
		Transition = SATURN_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMsaturn = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));


		// Uranus
		timer = uranusTimer; 
		Rotation = SCALER_URANUS * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));

		timer = uranusRevoTimer;
		Transition = URANUS_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMuranus = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Neptune
		timer = neptuneTimer; 
		Rotation = SCALER_NEPTUNE* mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));

		timer = neptuneRevoTimer;
		Transition = NEPTUNE_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMneptune = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));


		

		////////////////////////
		//Camera interaction
		////////////////////////

		// Select mode
		if(planet_mode == 1){	// sun
			cam_scaler = 1.f;
			cam_transition = vec3(0,0,0);
		}
		else if(planet_mode == 4){	// earth
			cam_scaler = SCALER_EARTH/SCALER_SUN;
			cam_transition = wMe[3];
		}
		else if (planet_mode == 5){	//moon
			cam_scaler = SCALER_MOON/SCALER_SUN;
			cam_transition = wMmoon[3];
		}
		else if(planet_mode == 6){	// mars
			cam_scaler = SCALER_MARS/SCALER_SUN;
			cam_transition = wMmars[3];
		}
		else if(planet_mode == 2){	// mercury
			cam_scaler = SCALER_MERCURY/SCALER_SUN;
			cam_transition = wMmercury[3];
		}
		else if(planet_mode == 3){	// venus
			cam_scaler = SCALER_VENUS/SCALER_SUN;
			cam_transition = wMvenus[3];
		}
		else if(planet_mode == 7){	// jupiter
			cam_scaler = SCALER_JUPITER/SCALER_SUN;
			cam_transition = wMjupiter[3];
		}
		else if(planet_mode == 8){	// saturn
			cam_scaler = SCALER_SATURN/SCALER_SUN;
			cam_transition = wMsaturn[3];
		}
		else if(planet_mode == 9){	// uranus
			cam_scaler = SCALER_URANUS/SCALER_SUN;
			cam_transition = wMuranus[3];
		}
		else if(planet_mode == 0){	// neptune
			cam_scaler = SCALER_NEPTUNE/SCALER_SUN;
			cam_transition = wMneptune[3];
		}
		wMstar = mat4(SCALER_STAR * vec4(1,0,0,0), SCALER_STAR * vec4(0,1,0,0), SCALER_STAR * vec4(0,0,1,0), vec4(cam_transition,1));

		//Rotation
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		vec2 cursorPos(xpos, ypos);
		vec2 cursorChange = cursorPos - lastCursorPos;

		if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
			cam_phi += cursorChange.y * cursorSensitivity;	// along longitude
			cam_theta += cursorChange.x * cursorSensitivity;	// along latitude
			if(cam_theta > 2*PI_F) cam_theta -= 2*PI_F;
			else if(cam_theta < -2*PI_F) cam_theta += 2*PI_F;
			if(cam_phi > PI_F-0.001)cam_phi = PI_F-0.001;
			if(cam_phi < 0.001)cam_phi = 0.001;
		}
		lastCursorPos = cursorPos;

		cam.pos.y = cam.radius * cos(cam_phi);
		cam.pos.x = cam.radius * sin(cam_phi) * cos(cam_theta);
		cam.pos.z = cam.radius * sin(cam_phi) * sin(cam_theta);

		cam.centre = vec3(0,0,0) + cam_transition;
		cam.pos = cam_scaler*cam.pos + cam_transition;


		///////////
		//Drawing
		//////////

		// clear screen to a dark grey colour
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render sun
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 0); 
		RenderScene(&texture_sun, &geometry_sun, program, &cam, perspectiveMatrix, wMs, GL_TRIANGLES ,0,0, &texture_earthnight);

		// Render earth
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "pecularmap"), 13);
		glUniform1i(glGetUniformLocation(program, "image"), 1);
		glUniform1i(glGetUniformLocation(program, "nightmap"), 4);
		glUniform3f(glGetUniformLocation(program, "camPosition"), cam.pos.x, cam.pos.y, cam.pos.z);
		RenderEarth(&texture_earth, &geometry_earth, program, &cam, perspectiveMatrix, wMe, GL_TRIANGLES,1,1, &texture_earthnight, &texture_earth_spec_map);

		// Render star background
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 2);
		RenderScene(&texture_star, &geometry_star, program, &cam, perspectiveMatrix, wMstar, GL_TRIANGLES,0,0, &texture_earthnight);

		// Render moon
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 3);
		RenderScene(&texture_moon, &geometry_moon, program, &cam, perspectiveMatrix, wMmoon, GL_TRIANGLES,1,0, &texture_earthnight);

		// Render Mars
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 5);
		RenderScene(&texture_mars, &geometry_mars, program, &cam, perspectiveMatrix, wMmars, GL_TRIANGLES,1,0, &texture_mars);

		// Render Mercury
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 7);
		RenderScene(&texture_mercury, &geometry_mercury, program, &cam, perspectiveMatrix, wMmercury, GL_TRIANGLES,1,0, &texture_mercury);

		// Render Venus
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 6);
		RenderScene(&texture_venus, &geometry_venus, program, &cam, perspectiveMatrix, wMvenus, GL_TRIANGLES,1,0, &texture_venus);

		// Render Jupiter
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 8);
		RenderScene(&texture_jupiter, &geometry_jupiter, program, &cam, perspectiveMatrix, wMjupiter, GL_TRIANGLES,1,0, &texture_jupiter);

		// Render Saturn
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 9);
		RenderScene(&texture_saturn, &geometry_saturn, program, &cam, perspectiveMatrix, wMsaturn, GL_TRIANGLES,1,0, &texture_saturn);

		// Render Saturn Rings
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 12);
		RenderScene(&texture_saturn_ring, &geometry_saturn_ring, program, &cam, perspectiveMatrix, wMsaturn, GL_TRIANGLES,0,0, &texture_saturn_ring); 


		// Render Uranus
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 10);
		RenderScene(&texture_uranus, &geometry_uranus, program, &cam, perspectiveMatrix, wMuranus, GL_TRIANGLES,1,0, &texture_uranus);

		// Render Neptune
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 11);
		RenderScene(&texture_neptune, &geometry_neptune, program, &cam, perspectiveMatrix, wMneptune, GL_TRIANGLES,1,0, &texture_neptune);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry_sun);
	DestroyGeometry(&geometry_earth);
	DestroyGeometry(&geometry_star);
	DestroyGeometry(&geometry_moon);
	glUseProgram(0);
	glDeleteProgram(program);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
