//Multi-Object, Multi-Texture Example
//Stephen J. Guy, 2021

//This example demonstrates:
// Loading multiple models (a cube and a knot)
// Using multiple textures (wood and brick)
// Instancing (the teapot is drawn in two locations)
// Continuous keyboard input - arrows (moves knot up/down/left/right continuous when being held)
// Keyboard modifiers - shift (up/down arrows move knot in/out of screen when shift is pressed)
// Single key events - pressing 'c' changes color of a random teapot
// Mixing textures and colors for models
// Phong lighting
// Binding multiple textures to one shader

const char* INSTRUCTIONS = 
"***************\n"
"This demo shows multiple objects being draw at once along with user interaction.\n"
"\n"
"Up/down/left/right - Moves the knot.\n"
"c - Changes to teapot to a random color.\n"
"***************\n"
;

//Mac OS build: g++ multiObjectTest.cpp -x c glad/glad.c -g -F/Library/Frameworks -framework SDL2 -framework OpenGL -o MultiObjTest
//Linux build:  g++ multiObjectTest.cpp -x c glad/glad.c -g -lSDL2 -lSDL2main -lGL -ldl -I/usr/include/SDL2/ -o MultiObjTest

#include "glad/glad.h"  //Include order can matter here
#if defined(__APPLE__) || defined(__linux__)
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_opengl.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include "parse.h"
#include <math.h>       
#include "loadmodel.h"


#define PI 3.14159265
using namespace std;

int screenWidth = 800; 
int screenHeight = 600;  
float timePast = 0;

//SJG: Store the object coordinates
//You should have a representation for the state of each object
float objx=0, objy=0, objz=0;
float colR=1, colG=1, colB=1;

float viewx;
float viewy;
float lookx;
float looky;

bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;
void Win2PPM(int width, int height);

//srand(time(NULL));
float rand01(){
	return rand()/(float)RAND_MAX;
}

void drawGeometry(int shaderProgram, int model1_start, int model1_numVerts, int model2_start, int model2_numVerts);
void drawWalls(int shaderProgram, int model1_start, int model1_numVerts);
void drawFloors(int shaderProgram, int model1_start, int model1_numVerts);
void drawGoal(int shaderProgram, int model1_start, int model1_numVerts);
void drawKey_Door(int shaderProgram, int model1_start, int model1_numVerts, float xoffset, float yoffset, float zoffset, float r, float g, float b, bool key);
void move_key(float x, float y, float viewx, float viewy);
bool jump(float time, float& playerz);
// build a mesh based on x and y cor, and their type
// type 1 = wall
// type 2 = floor;
// type 3 = door; 31,32,33,34,35 for "a b c d e" door
// type 4 = key; 41,42,43,44,45 for "a,b,c,d,e" key
// type 5 = goal;

void build_mesh(int x, int y, int type); 
bool collision(float x, float y);
int main(int argc, char* argv[]) {
	parseMapFile("map6.txt"); // read map
	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);

	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}

	//Here we will load two different model files 

	//Load Model 1
	ifstream modelFile;
	modelFile.open("models/cube.txt");
	int wallLines = 0;
	modelFile >> wallLines;
	float* model1 = new float[wallLines];
	for (int i = 0; i < wallLines; i++) {
		modelFile >> model1[i];
	}
	printf("%d\n", wallLines);
	int numVertsWall = wallLines / 8;
	modelFile.close();

	//Load Model 2
	modelFile.open("models/teapot.txt");
	int numLines = 0;
	modelFile >> numLines;
	float* model2 = new float[numLines];
	for (int i = 0; i < numLines; i++) {
		modelFile >> model2[i];
	}
	printf("%d\n", numLines);
	int numVertsKnot = numLines / 8;
	modelFile.close();

	// load model 3
	modelFile.open("models/knot.txt");
	int goalLines = 0;
	modelFile >> goalLines;
	float* model3 = new float[goalLines];
	for (int i = 0; i < goalLines; i++) {
		modelFile >> model3[i];
	}
	printf("%d\n", goalLines);
	int numVertsGoal = goalLines / 8;
	modelFile.close();


	//SJG: I load each model in a different array, then concatenate everything in one big array
	// This structure works, but there is room for improvement here. Eg., you should store the start
	// and end of each model a data structure or array somewhere.
	//Concatenate model arrays
	int numVertsTeapot = numVertsWall;

	float* modelData = new float[(numVertsWall + numVertsKnot + numVertsGoal) * 8];
	copy(model1, model1 + numVertsWall * 8, modelData);
	copy(model2, model2 + numVertsKnot * 8, modelData + numVertsTeapot * 8);
	copy(model3, model3 + goalLines, modelData + numVertsTeapot * 8 + numVertsKnot * 8);

	int totalNumVerts = numVertsTeapot + numVertsKnot + numVertsGoal;
	int startVertTeapot = 0;  //The teapot is the first model in the VBO
	int startVertKnot = numVertsTeapot; //The knot starts right after the taepot
	int startVertGoal = numVertsTeapot + numVertsKnot; // start of goal

	//// Allocate Texture 0 (Wood) ///////
	SDL_Surface* surface = SDL_LoadBMP("wood.bmp");
	if (surface == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex0;
	glGenTextures(1, &tex0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);

	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Load the texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface);
	//// End Allocate Texture ///////


	//// Allocate Texture 1 (Brick) ///////
	SDL_Surface* surface1 = SDL_LoadBMP("brick.bmp");
	if (surface == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex1;
	glGenTextures(1, &tex1);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE1);

	glBindTexture(GL_TEXTURE_2D, tex1);

	// third texture


	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//How to filter
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w, surface1->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface1->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface1);
	//// End Allocate Texture ///////


	//// Allocate Texture 3 (leaf) ///////
	SDL_Surface* surface2 = SDL_LoadBMP("door.bmp");
	if (surface2 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex2;
	glGenTextures(1, &tex2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tex2);

	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Load the texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface2->w, surface2->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface2->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface2);
	//// End Allocate Texture ///////

	//Build a Vertex Array Object (VAO) to store mapping of shader attributse to VBO
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, totalNumVerts * 8 * sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

	int texturedShader = InitShader("textured-Vertex.glsl", "textured-Fragment.glsl");

	//Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(texturedShader, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
	//Attribute, vals/attrib., type, isNormalized, stride, offset
	glEnableVertexAttribArray(posAttrib);

	//GLint colAttrib = glGetAttribLocation(phongShader, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);

	GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");

	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one	


	glEnable(GL_DEPTH_TEST);

	printf("%s\n", INSTRUCTIONS);

	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	bool quit = false;
	float camx = player.Playerx;
	float camy = player.Playery;
	float camz = 0;
	float angel = 0;
	bool jumping = false;
	float time = 0;
	while (!quit) {
		if (jumping)
		{
			jumping = jump(time, camz);
			time += 0.1;
		}
		while (SDL_PollEvent(&windowEvent)) {  //inspect all events in the queue
			if (windowEvent.type == SDL_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) { //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
			}
			if (player.goal)// reach the goal
			{
				player.goal = false;
				camx = player.Playerx;
				camy = player.Playery;
			}

			//SJG: Use key input to change the state of the object
			//     We can use the ".mod" flag to see if modifiers such as shift are pressed
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_LEFT) { //If "left key" is pressed
				angel -= 15;
				viewx = sin(angel * PI / 180);
				viewy = cos(angel * PI / 180);
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_RIGHT) { //If "right key" is pressed
				angel += 15;
				viewx = sin(angel * PI / 180);
				viewy = cos(angel * PI / 180);
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_UP) { //If "up key" is pressed
				camx += 0.1 * viewx;
				camy += 0.1 * viewy;
				//move_key(0.1, 0.1, viewx, viewy);
				if (collision(camx, camy))
				{
					camx -= 0.1 * viewx;
					camy -= 0.1 * viewy;
					move_key(-0.1, -0.1, viewx, viewy);
				}
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_DOWN) { //If "down key" is pressed
				camx -= 0.1 * viewx;
				camy -= 0.1 * viewy;
				//move_key(-0.1, -0.1, viewx, viewy);
				if (collision(camx, camy))
				{
					camx += 0.1 * viewx;
					camy += 0.1 * viewy;
					move_key(0.1, 0.1, viewx, viewy);
				}
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_SPACE) { //If "SPACE key" is pressed(jump)
				jumping = true;
				time = 0;
			}
			
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_a) { //If "A" is pressed
				camz++;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_d) { //If "D" is pressed
				camz--;
			}
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_c) { //If "c" is pressed
				colR = rand01();
				colG = rand01();
				colB = rand01();
			}
		}

		// Clear the screen to default color
		glClearColor(.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(texturedShader);


		timePast = SDL_GetTicks() / 1000.f;
		lookx = camx + viewx;
		looky = camy + viewy;
		glm::mat4 view = glm::lookAt(
			glm::vec3(camx, camy, camz),  //Cam Position
			glm::vec3(lookx, looky, camz/4.0*3),  //Look at point
			glm::vec3(0.0f, 0.0f, 1.0f)); //Up
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 proj = glm::perspective(3.14f / 4, screenWidth / (float)screenHeight, 1.0f, 10.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex2);
		glUniform1i(glGetUniformLocation(texturedShader, "tex2"), 2);

		glBindVertexArray(vao);
		//drawGeometry(texturedShader, startVertTeapot, numVertsTeapot, startVertKnot, numVertsKnot);
		drawWalls(texturedShader, startVertTeapot, numVertsTeapot);
		drawFloors(texturedShader, startVertTeapot, numVertsTeapot);
		for (int i = 0; i < player.doors.size(); i++)
		{
			if (!player.doors[i].have_key)
			{
				drawKey_Door(texturedShader, startVertKnot, numVertsKnot, player.doors[i].keyx, player.doors[i].keyy, player.doors[i].keyz, player.doors[i].r, player.doors[i].g, player.doors[i].b, true);
			}
			if (!player.doors[i].open)
			{
				drawKey_Door(texturedShader, startVertTeapot, numVertsTeapot, (float)player.doors[i].doorx, (float)player.doors[i].doory, 0, player.doors[i].r, player.doors[i].g, player.doors[i].b, false);
			}
		}
		drawGoal(texturedShader, startVertGoal, numVertsGoal);
		SDL_GL_SwapWindow(window); //Double buffering
	}

	//Clean Up
	glDeleteProgram(texturedShader);
	glDeleteBuffers(1, vbo);
	glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}
 
void drawGeometry(int shaderProgram, int model1_start, int model1_numVerts, int model2_start, int model2_numVerts){
	
	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(colR,colG,colB);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));
      
    GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	  
	//************
	//Draw model #1 the first time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num of verticies
	//*************

	
	//model = glm::scale(model,glm::vec3(.2f,.2f,.2f)); //An example of scale 
    glm::mat4 model = glm::mat4(1);
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

	//Set which texture to use (-1 = no texture)
	glUniform1i(uniTexID, 0); 

	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
	
	
	//************
	//Draw model #1 the second time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num. of verticies
	//*************

	//Translate the model (matrix) left and back
	model = glm::mat4(1); //Load intentity
	model = glm::translate(model,glm::vec3(-2,-1,-.4));
	//model = glm::scale(model,2.f*glm::vec3(1.f,1.f,0.5f)); //scale example
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//Set which texture to use (0 = wood texture ... bound to GL_TEXTURE0)
	glUniform1i(uniTexID, 0);

  //Draw an instance of the model (at the position & orientation specified by the model matrix above)
	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
		
	//************
	//Draw model #2 once
	//This model is stored in the VBO starting a offest model2_start and with model2_numVerts num of verticies
	//*************

	//Translate the model (matrix) based on where objx/y/z is
	// ... these variables are set when the user presses the arrow keys
	model = glm::mat4(1);
	model = glm::scale(model,glm::vec3(.8f,.8f,.8f)); //scale this model
	model = glm::translate(model,glm::vec3(objx,objy,objz));

	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
	glUniform1i(uniTexID, 1); 
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
	glDrawArrays(GL_TRIANGLES, model2_start, model2_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
}
void drawWalls(int shaderProgram, int model1_start, int model1_numVerts) {

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(colR, colG, colB);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glm::mat4 model = glm::mat4(1);
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	
	//Draw model #1 the second time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num. of verticies
	//*************
	
	for (int i = 0; i < walls.size(); i++)
	{
		int x = walls[i].x;
		int y = walls[i].y;
		//Translate the model (matrix) left and back
		model = glm::mat4(1); //Load intentity
		model = glm::translate(model, glm::vec3(x, y, 0));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

		//Set which texture to use (0 = wood texture ... bound to GL_TEXTURE0)
		glUniform1i(uniTexID, 1);

		//Draw an instance of the model (at the position & orientation specified by the model matrix above)
		glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
	}
}
bool jump(float time,float &playerz)
{
	float z =  time - 0.1  * time * time;
	if (z < 0)
	{
		z = 0;
		playerz = z;
		return false;
	}
	playerz = z;
	return true;
}
void drawFloors(int shaderProgram, int model1_start, int model1_numVerts) {

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(colR, colG, colB);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glm::mat4 model = glm::mat4(1);
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");

	//Draw model #1 the second time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num. of verticies
	//*************
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			//Translate the model (matrix) left and back
			model = glm::mat4(1); //Load intentity
			model = glm::translate(model, glm::vec3(i, j, -1));
			glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

			//Set which texture to use (0 = wood texture ... bound to GL_TEXTURE0)
			glUniform1i(uniTexID, 0);

			//Draw an instance of the model (at the position & orientation specified by the model matrix above)
			glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
		}
		
	}


}
void move_key(float x, float y,float viewx, float viewy)
{
	float keypos = 0;
	for (int i = 0; i < player.doors.size(); i++)
	{
		if (player.doors[i].have_key)
		{
			player.doors[i].keyx += x * viewx;
			player.doors[i].keyy += y * viewy;
			if (keypos == 1)
			{
				player.doors[i].keyz = 0.3;
			}
			else if (keypos == 2)
			{
				player.doors[i].keyz = -0.3;
			}
			else if (keypos == 3)
			{
				player.doors[i].keyz = 0.6;
			}
			else if (keypos == 4)
			{
				player.doors[i].keyz = -0.6;
			}
			keypos++;
		}
	}
}
void drawGoal(int shaderProgram, int model1_start, int model1_numVerts) {

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(colR, colG, colB);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glm::mat4 model = glm::mat4(1);
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");

	//Draw model #1 the second time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num. of verticies
	//*************
	
	//Translate the model (matrix) left and back
	model = glm::mat4(1); //Load intentity
	model = glm::translate(model, glm::vec3(player.goalx,player.goaly ,0 ));
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//Set which texture to use (0 = wood texture ... bound to GL_TEXTURE0)
	glUniform1i(uniTexID, 0);

	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)



}
void drawKey_Door(int shaderProgram, int model1_start, int model1_numVerts, float xoffset, float yoffset,float zoffset, float r, float g, float b,bool key) {

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(r, g,b);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glm::mat4 model = glm::mat4(1);
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");

	//Draw model #1 the second time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num. of verticies
	//*************
	
	//Translate the model (matrix) left and back
	model = glm::mat4(1); //Load intentity
	model = glm::translate(model, glm::vec3(xoffset, yoffset, zoffset));
	if (key) // if draw the key, shrink it a little bit
	{
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f)); //An example of scale
	}
	
	
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//Set which texture to use (0 = wood texture ... bound to GL_TEXTURE0)
	glUniform1i(uniTexID, -1);

	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)



}
// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile){
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}
// collision for door/key/wall
bool collision(float x, float y)
{ 
	//check for wall
	for (int i = 0; i < walls.size(); i++)
	{
		
		if (fabs(x - walls[i].x) < 0.75 && fabs(y - walls[i].y) < 0.75) // collide with wall
		{
			return true;
		}
	}

	// check for door
	for (int i = 0; i < player.doors.size(); i++)
	{
		if (player.doors[i].open)
		{
			continue;
		}
		if (fabs(x - player.doors[i].doorx) < 1 && fabs(y - player.doors[i].doory) < 1) // collide with door
		{
			if (player.doors[i].have_key)
			{
				player.doors[i].open = true; // if have the key, open the door
				return false;
			}
			else
			{
				return true;
			}
		}
	}
	
	// check for key
	
	for (int i = 0; i < player.doors.size(); i++)
	{
		if (player.doors[i].have_key)
		{
			continue;
		}
		float dis = sqrt(pow(x - player.doors[i].keyx, 2) + pow(y - player.doors[i].keyy, 2));
		if (dis < 0.5) // collide with key
		{
			player.doors[i].have_key = true;
			//player.doors[i].keyx = player.Playerx + viewx;
			//player.doors[i].keyy = player.Playery + viewy;
			continue;
		}
	}
	
	// check for goal
	float dis = sqrt(pow(x - player.goalx, 2) + pow(y - player.goaly, 2));
	if (dis < 0.5) // reach the goal, reload the game
	{
		player.goal = true;
		player.Playerx = player.startx;
		player.Playery = player.starty;
		for (int i = 0; i < player.doors.size(); i++)
		{
			player.doors[i].open = false;
			player.doors[i].have_key = false;
		}
	}
	return false;
}
// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName){
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders
	
	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}
	
	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	
	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}
