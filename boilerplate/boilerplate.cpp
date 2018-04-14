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

#define ROTATION_SCALER 50.f

#define DIS_E_S 149.2f	// Million km

#define SCALER_EARTH 0.025f
#define SCALER_SUN 0.1f
#define SCALER_STAR 4.f
#define SCALER_MOON 0.01f

#define SUN_ROTATION 17.3f

#define EARTH_ROTATION 1.f
#define EARTH_REVOLUTION 365.f*0.5f
#define EARTH_REVO_RADIUS 0.2f

#define MOON_ROTATION 27.f
#define MOON_REVOLUTION 27.3f
#define MOON_REVO_RADIUS 0.04f

#define SCALER_CAM_RADIUS 0.01f
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

void RenderScene(MyTexture* tex, Geometry *geometry, GLuint program, Camera* camera, mat4 perspectiveMatrix, mat4 wMp, GLenum rendermode, int shadeflg)
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

	glBindVertexArray(geometry->vertexArray);
	glBindTexture(tex->target, tex->textureID);
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
	
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
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


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// three vertex positions and assocated colours of a triangle
	//Fill in with Perspective Matrix
	//mat4(1.f) identity matrix
	mat4 perspectiveMatrix = glm::perspective(PI_F*0.4f, float(width)/float(height), 0.01f, 10.f);	//last 2 arg, nearst and farest

//----------------------- Generate Planets ---------------------------//
	vector<vec3> Sun;		//vertices
	vector<vec2> sunTex;	//texture
	planetMaker(&Sun, &sunTex, 128);
	mat4 wMs;
	
	vector<vec3> Earth;
	vector<vec2> earthTex;
	planetMaker(&Earth, &earthTex, 72);
	mat4 wMe;

	vector<vec3> Star;
	vector<vec2> starTex;
	planetMaker(&Star, &starTex, 256);
	mat4 wMstar = mat4(SCALER_STAR * vec4(1,0,0,0), SCALER_STAR * vec4(0,1,0,0), SCALER_STAR * vec4(0,0,1,0), vec4(0,0,0,1));

	vector<vec3> Moon;
	vector<vec2> moonTex;
	planetMaker(&Moon, &moonTex, 72);
	mat4 wMmoon;

//----------------------- Generate Planets ---------------------------//

	Geometry geometry_sun;
	Geometry geometry_earth;
	Geometry geometry_star;
	Geometry geometry_moon;


	// call function to create and fill buffers with geometry data
	if (!InitializeVAO(&geometry_sun))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_sun, Sun.data(), sunTex.data(), Sun.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_earth))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_earth, Earth.data(), earthTex.data(), Earth.size()))
		cout << "Failed to load geometry" << endl;	

	if (!InitializeVAO(&geometry_star))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_star, Star.data(), starTex.data(), Star.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_moon))
		cout << "Program failed to intialize geometry!" << endl;
	if(!LoadGeometry(&geometry_moon, Moon.data(), moonTex.data(), Moon.size()))
		cout << "Failed to load geometry" << endl;



	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);


	vec2 lastCursorPos;

	float cursorSensitivity = PI_F/400.f;	//PI/hundred pixels

	//------------------------- Bind texture ------------------------//

	MyTexture texture_sun, texture_earth, texture_star, texture_moon;
	InitializeTexture(&texture_sun, "2k_sun.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_earth, "earthmap1k.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_star, "2k_stars_milky_way.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_moon, "2k_moon.jpg", GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_sun.textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_earth.textureID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture_star.textureID);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texture_moon.textureID);

	//------------------------- Bind texture ------------------------//


	float timer = 0.f;
	float sunTimer = 0.f;
	float earthTimer = 0.f;
	float earthRevoTimer = 0.f;
	float moonTimer = 0.f;
	float moonRevoTimer = 0.f;

	float tilt = 23.5f/180.f * PI_F;

	cam.radius = SCALER_SUN + 0.5f;

	cam_max_r = SCALER_SUN + 1.5f;
	cam_min_r = SCALER_SUN + 0.11f;

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
		tilt = 23.5f/180.f * PI_F;
		mat3 earthTilt = mat3(vec3(cos(tilt), sin(tilt), 0), vec3(-sin(tilt), cos(tilt), 0), vec3(0,0,1));
		Rotation =  earthTilt * Rotation;

		timer = earthRevoTimer;
		Transition = EARTH_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		wMe = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));

		// Moon
		timer = moonTimer;
		Rotation = SCALER_MOON * mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));
		tilt = 5.f/180.f * PI_F;
		mat3 moonTilt = mat3(vec3(cos(tilt), sin(tilt), 0), vec3(-sin(tilt), cos(tilt), 0), vec3(0,0,1));
		Rotation = moonTilt * Rotation;
		
		timer = moonRevoTimer;
		Transition = MOON_REVO_RADIUS * vec3(cos(timer) + sin(timer), 0, -sin(timer) + cos(timer));
		mat4 eMmoon = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));;	// Moon to earth
		wMmoon = mat4(vec4(1,0,0,0),vec4(0,1,0,0),vec4(0,0,1,0),wMe[3]) * eMmoon;
		

		////////////////////////
		//Camera interaction
		////////////////////////

		// Select mode
		if(planet_mode == 1){
			cam_scaler = 1.f;
			cam_transition = vec3(0,0,0);
			wMstar = mat4(SCALER_STAR * vec4(1,0,0,0), SCALER_STAR * vec4(0,1,0,0), SCALER_STAR * vec4(0,0,1,0), vec4(0,0,0,1));
		}
		else if(planet_mode == 2){
			cam_scaler = SCALER_EARTH/SCALER_SUN;
			cam_transition = wMe[3];
			wMstar = mat4(SCALER_STAR * vec4(1,0,0,0), SCALER_STAR * vec4(0,1,0,0), SCALER_STAR * vec4(0,0,1,0), vec4(cam_transition,1));
		}
		else if (planet_mode == 3){
			cam_scaler = SCALER_MOON/SCALER_SUN;
			cam_transition = wMmoon[3];
			wMstar = mat4(SCALER_STAR * vec4(1,0,0,0), SCALER_STAR * vec4(0,1,0,0), SCALER_STAR * vec4(0,0,1,0), vec4(cam_transition,1));
		}

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
		RenderScene(&texture_sun, &geometry_sun, program, &cam, perspectiveMatrix, wMs, GL_TRIANGLES ,0);

		// Render earth
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 1);
		RenderScene(&texture_earth, &geometry_earth, program, &cam, perspectiveMatrix, wMe, GL_TRIANGLES,1);

		// Render star background
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 2);
		RenderScene(&texture_star, &geometry_star, program, &cam, perspectiveMatrix, wMstar, GL_TRIANGLES,0);

		// Render moon
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 3);
		RenderScene(&texture_moon, &geometry_moon, program, &cam, perspectiveMatrix, wMmoon, GL_TRIANGLES,1);


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
