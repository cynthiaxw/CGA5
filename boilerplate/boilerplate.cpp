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

#define DIS_E_S 149.2f	// Million km
#define SCALER_E 0.75f
#define SUN_ROTATION 17.3f
#define EARTH_ROTATION 365

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

void RenderScene(MyTexture* tex, Geometry *geometry, GLuint program, Camera* camera, mat4 perspectiveMatrix, mat4 wMp, GLenum rendermode)
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
	vec3 vertices[] = {
		vec3( -.6f, -.4f, -0.5f ),
		vec3( .0f,  .6f, -0.5f ),
		vec3( .6f, -.4f,-0.5f )
	};

	vec3 frustumVertices[] = {
		vec3(-1, -1, -1),
		vec3(-1, -1, 1),
		vec3(-1, 1, 1),
		vec3(1, 1, 1),
		vec3(1, 1, -1),
		vec3(-1, 1, -1),
		vec3(-1, -1, -1),
		vec3(1, -1, -1),
		vec3(1, -1, 1),
		vec3(-1, -1, 1),
		vec3(-1, 1, 1),
		vec3(-1, 1, -1),
		vec3(1, 1, -1),
		vec3(1, -1, -1),
		vec3(1, -1, 1),
		vec3(1, 1, 1)
	};

	vec2 texcoord[] = {
		vec2(0,0),
		vec2(0,1),
		vec2(1,0),
	};
	//Fill in with Perspective Matrix
	//mat4(1.f) identity matrix
	mat4 perspectiveMatrix = glm::perspective(PI_F*0.4f, float(width)/float(height), 0.1f, 5.f);	//last 2 arg, nearst and farest

	for(int i=0; i<16; i++){
		vec4 newPoint = inverse(perspectiveMatrix)*vec4(frustumVertices[i], 1);
		frustumVertices[i] = vec3(newPoint)/newPoint.w;
	}


//----------------------- Generate Planets ---------------------------//
	vector<vec3> Sun;		//vertices
	vector<vec2> sunTex;	//texture
	planetMaker(&Sun, &sunTex, 128);
	mat4 wMs = mat4(vec4(1,0,0,0), vec4(0,1,0,0), vec4(0,0,1,0), vec4(0,0,0,1));
	
	vector<vec3> Earth;
	vector<vec2> earthTex;
	planetMaker(&Earth, &earthTex, 72);
	mat4 wMe = mat4(SCALER_E * vec4(1,0,0,0), SCALER_E * vec4(0,1,0,0), SCALER_E * vec4(0,0,1,0), vec4(0,0.5f,0,1));



//----------------------- Generate Planets ---------------------------//

	Geometry geometry;
	Geometry geometry_sun;
	Geometry geometry_earth;


	// call function to create and fill buffers with geometry data
	if (!InitializeVAO(&geometry_sun))
		cout << "Program failed to intialize geometry!" << endl;

	if(!LoadGeometry(&geometry_sun, Sun.data(), sunTex.data(), Sun.size()))
		cout << "Failed to load geometry" << endl;

	if (!InitializeVAO(&geometry_earth))
	cout << "Program failed to intialize geometry!" << endl;

	if(!LoadGeometry(&geometry_earth, Earth.data(), earthTex.data(), Earth.size()))
		cout << "Failed to load geometry" << endl;	



	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	Camera cam;

	vec2 lastCursorPos;

	float cursorSensitivity = PI_F/200.f;	//PI/hundred pixels
	float movementSpeed = 0.01f;

	//------------------------- Bind texture ------------------------//

	MyTexture texture_sun, texture_earth;
	InitializeTexture(&texture_sun, "2k_sun.jpg", GL_TEXTURE_2D);
	InitializeTexture(&texture_earth, "earthmap1k.jpg", GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_sun.textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_earth.textureID);

	//------------------------- Bind texture ------------------------//


	float timer = 0.f;
	mat3 Rotation;
	vec3 Transition;

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		debug3("camera", cam.pos);
		////////////////////////
		//Camera interaction
		////////////////////////
		//Translation

		// Time
		if(timer >= 2*PI_F){
			timer = 0;
		}
		else timer += 0.001;


		vec3 movement(0.f);

		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
			movement.z += 1.f;
		}
		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
			movement.z -= 1.f;
		}
		if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
			movement.x += 1.f;
		}
		if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
			movement.x -= 1.f;
		}

		cam. move(movement*movementSpeed);


		//Rotation
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		vec2 cursorPos(xpos, ypos);
		vec2 cursorChange = cursorPos - lastCursorPos;

		if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
			cam.rotateHorizontal(-cursorChange.x*cursorSensitivity);
			cam.rotateVertical(-cursorChange.y*cursorSensitivity);
		}
		lastCursorPos = cursorPos;

	
		///////////
		//Drawing
		//////////

		// clear screen to a dark grey colour
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render sun
		// ROtation matrxi by y-axis
		Rotation = mat3(vec3(cos(timer), 0, -sin(timer)), vec3(0, 1, 0), vec3(sin(timer), 0, cos(timer)));
		Transition = vec3(0,0,0);
		wMs = mat4(vec4(Rotation[0],0), vec4(Rotation[1], 0), vec4(Rotation[2], 0), vec4(Transition, 1));
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 0); 
		RenderScene(&texture_sun, &geometry_sun, program, &cam, perspectiveMatrix, wMs, GL_TRIANGLES);

		// Render earth
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "image"), 1);
		RenderScene(&texture_earth, &geometry_earth, program, &cam, perspectiveMatrix, wMe, GL_TRIANGLES);


		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
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
