#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <math.h>
#include <queue>
#include <functional>
#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include "cNode.h"
#include "cShaderManager.h"
#include "cMesh.h"
#include "cVAOMeshManager.h"
#include "cGameObject.h"

float angle = 0.0f;
float lookx = 5.0f, looky = 0.0f, lookz = 5.0f;
float camPosx = 5.0f, camPosy = 15.0f, camPosz = 12.0f;
glm::vec3 g_cameraXYZ = glm::vec3(camPosx, camPosy, camPosz);
glm::vec3 g_cameraTarget_XYZ = glm::vec3(lookx, looky, lookz);

//Important Global Variables
cShaderManager* g_pShaderManager = new cShaderManager();
cVAOMeshManager* g_pVAOManager = new cVAOMeshManager();
std::vector <cGameObject*> g_vecGameObject;

//Global variables for pathfinding
int positionOnPath = 0;				// This is how far along the path our player is
std::vector<cNode*> pathToVictory;	// Create a vector of just the nodes in the path from start to finish

int Heuristics(int nodeID, int goalNode);
bool LoadPlyFileIntoMesh(std::string filename, cMesh &theMesh);
void ReadFileToToken(std::ifstream &file, std::string token);

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	const float CAMERASPEED = 0.3f;
	switch (key)
	{
		//MOVEMENT USING TANK CONTROLS WASD
	case GLFW_KEY_A:		// Look Left
		//angle -= CAMERASPEED * 0.1f;
		//lookx = sin(angle);
		//lookz = -cos(angle);
		camPosz -= CAMERASPEED;
		break;
	case GLFW_KEY_D:		// Look Right
		//angle += CAMERASPEED * 0.1f;
		//lookx = sin(angle);
		//lookz = -cos(angle);
		camPosz += CAMERASPEED;
		break;
	case GLFW_KEY_W:		// Move Forward (relative to which way you're facing)
		camPosx += CAMERASPEED;
		break;
	case GLFW_KEY_S:		// Move Backward
		camPosx -= CAMERASPEED;
		break;
	case GLFW_KEY_DOWN:		// Look Down (along y axis)
		camPosy -= CAMERASPEED;	//if (g_cameraTarget_XYZ.y > 0.5)	//Up and down looking range is limited
		break;
	case GLFW_KEY_UP:		// Look Up (along y axis)
		camPosy += CAMERASPEED;	//if (g_cameraTarget_XYZ.y < 1.5)
		break;
	}
}

int main()
{
	//A whole bunch of code for setting up openGL. Head to line 181 for AI coding
	GLFWwindow* window;
	GLuint vertex_buffer, vertex_shader, fragment_shader, program;
	GLint mvp_location, vpos_location, vcol_location;
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(1080, 768,
		"welcome",
		NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);

	cShaderManager::cShader vertShader;
	cShaderManager::cShader fragShader;

	vertShader.fileName = "simpleVert.glsl";
	fragShader.fileName = "simpleFrag.glsl";

	::g_pShaderManager->setBasePath("assets//shaders//");

	if (!::g_pShaderManager->createProgramFromFile(
		"simpleShader", vertShader, fragShader))
	{
		std::cout << "Failed to create shader program. Shutting down." << std::endl;
		std::cout << ::g_pShaderManager->getLastError() << std::endl;
		return -1;
	}
	std::cout << "The shaders comipled and linked OK" << std::endl;
	GLint shaderID = ::g_pShaderManager->getIDFromFriendlyName("simpleShader");

	{
		cMesh newMesh;
		newMesh.name = "Floor";
		if (!LoadPlyFileIntoMesh("100SquarePlaneNormals.ply", newMesh))
		{
			std::cout << "Didn't load model" << std::endl;
		}
		if (!::g_pVAOManager->loadMeshIntoVAO(newMesh, shaderID))
		{
			std::cout << "Could not load mesh into VAO" << std::endl;
		}
	}

	{
		cMesh newMesh;
		newMesh.name = "Wall";
		if (!LoadPlyFileIntoMesh("1x1SquareNormals.ply", newMesh))
		{
			std::cout << "Didn't load model" << std::endl;
		}
		if (!::g_pVAOManager->loadMeshIntoVAO(newMesh, shaderID))
		{
			std::cout << "Could not load mesh into VAO" << std::endl;
		}
	}

	{
		cMesh newMesh;
		newMesh.name = "Player";
		if (!LoadPlyFileIntoMesh("Sphereply_xyz_n.ply", newMesh))
		{
			std::cout << "Didn't load model" << std::endl;
		}
		if (!::g_pVAOManager->loadMeshIntoVAO(newMesh, shaderID))
		{
			std::cout << "Could not load mesh into VAO" << std::endl;
		}
	}

	GLint currentProgID = ::g_pShaderManager->getIDFromFriendlyName("simpleShader");

	GLint uniLoc_mModel = glGetUniformLocation(currentProgID, "mModel");
	GLint uniLoc_mView = glGetUniformLocation(currentProgID, "mView");
	GLint uniLoc_mProjection = glGetUniformLocation(currentProgID, "mProjection");

	GLint uniLoc_materialDiffuse = glGetUniformLocation(currentProgID, "materialDiffuse");

	glEnable(GL_DEPTH);

	//NOW IT'S TIME FOR A* CALCULATING

	std::ifstream mazeFile("MAZE.txt");
	if (!mazeFile.is_open())
		return 0;

	char curNode;
	int goal = -1;
	int start = -1;

	char nodeText[100];	// The contents of the maze text file
	std::vector<cNode*> nodes;	// A vector that will be holding the actual node objects
	// Priority queue that will be ordering nodes based on the A* heuristic
	std::priority_queue<std::pair<int, cNode*>, std::vector<std::pair<int, cNode*>>, std::greater<std::pair<int, cNode*>>> frontier;
	std::map<cNode*, int> costToReach;	// A map indicating how many steps it took to reach this node
	std::map<cNode*, cNode*> nodeBeforeMe;	// A map showing the node that comes before this node on the fastest recorded path

	for (int index = 0; index < 100; index++)
	{
		mazeFile >> curNode;
		nodeText[index] = curNode;
		if (curNode == 'G')
		{	// In the event we find multiple goal or start nodes, the maze is considered broken, and we return
			if (goal == -1)
				goal = index;
			else
				return -1;
		}
		if (curNode == 'S')
		{
			if (start == -1)
				start = index;
			else
				return -1;
		}
			
		nodes.push_back(new cNode(index));
	}

	if (goal == -1 || start == -1)	// If there was no goal or start, the maze is incomplete, and we return
		return -1;

	{
		//The player ball character is object 0
		cGameObject* newObj = new cGameObject();
		newObj->meshName = "Player";
		newObj->position.x = (float)(start % 10);
		newObj->position.y = 0.0f;
		newObj->position.z = (float)(start / 10);
		newObj->scale = 1.0f;
		newObj->diffuseColour.x = 0.0f;
		newObj->diffuseColour.y = 0.0f;
		newObj->diffuseColour.z = 1.0f;

		::g_vecGameObject.push_back(newObj);
	}

	{
		//The goal will also be a ball
		cGameObject* newObj = new cGameObject();
		newObj->meshName = "Player";
		newObj->position.x = (float)(goal % 10);
		newObj->position.y = 0.0f;
		newObj->position.z = (float)(goal / 10);
		newObj->scale = 1.0f;
		newObj->diffuseColour.x = 1.0f;
		newObj->diffuseColour.y = 0.0f;
		newObj->diffuseColour.z = 0.0f;

		::g_vecGameObject.push_back(newObj);
	}

	{
		cGameObject* newObj = new cGameObject();
		newObj->meshName = "Floor";
		newObj->position.x = 0.0f;
		newObj->position.y = 0.0f;
		newObj->position.z = 0.0f;
		newObj->scale = 0.1f;
		newObj->diffuseColour.x = 0.0f;
		newObj->diffuseColour.y = 1.0f;
		newObj->diffuseColour.z = 0.0f;

		::g_vecGameObject.push_back(newObj);
	}

	for (int index = 0; index < 100; index++)
	{	// Next we add the nodes adjacent to each node as its connections
		if (nodeText[index] == '1')
		{	//If the node is an "obstacle" node, we make a wall object to render
			cGameObject* newObj = new cGameObject();
			newObj->meshName = "Wall";
			newObj->position.x = (float)(index % 10);
			newObj->position.y = 0.0f;
			newObj->position.z = (float)(index / 10);
			newObj->scale = 1.0f;
			newObj->diffuseColour.x = 0.5f;
			newObj->diffuseColour.y = 0.5f;
			newObj->diffuseColour.z = 0.5f;

			::g_vecGameObject.push_back(newObj);
			continue;
		}	

		cNode* thisNode = nodes[index];
		//If it's a free space, or the goal, we connect it to its adjacent nodes

		//Add the node above this node
		if (index >= 10)
			if (nodeText[index - 10] == '0' || nodeText[index - 10] == 'G' || nodeText[index - 10] == 'S')
				thisNode->addConnection(nodes[index - 10]);
		//Add the node below this node
		if (index < 90)
			if (nodeText[index + 10] == '0' || nodeText[index + 10] == 'G' || nodeText[index + 10] == 'S')
				thisNode->addConnection(nodes[index + 10]);
		//Add the node to the left of this node
		if (index % 10 != 0)
			if (nodeText[index - 1] == '0' || nodeText[index - 1] == 'G' || nodeText[index - 1] == 'S')
				thisNode->addConnection(nodes[index - 1]);
		//Add the node to the right of this node
		if (index % 10 != 9)
			if (nodeText[index + 1] == '0' || nodeText[index + 1] == 'G' || nodeText[index + 1] == 'S')
				thisNode->addConnection(nodes[index + 1]);
	}

	std::pair<int, cNode*> startNode(0, nodes[start]);	// Adding the beginning node to the queue
	frontier.push(startNode);
	costToReach[nodes[start]] = 0;
	nodeBeforeMe[nodes[start]] = NULL;
	bool exitFound = false;

	while (!frontier.empty())
	{
		cNode* currentNode = frontier.top().second;
		frontier.pop();	//Remove the top node from the list

		if (currentNode->ID == goal)
		{
			exitFound = true;
			break;//We're done here
		}	

		for (int index = 0; index < currentNode->connectedNodes.size(); index++)
		{
			int newCost = costToReach[currentNode] + 1;
			if (costToReach.find(currentNode->connectedNodes[index]) == costToReach.end() || costToReach[currentNode->connectedNodes[index]] > newCost)
			{	// Check if a) this node has been mapped or not, and b) if the path we're on now is quicker than the one we had before
				costToReach[currentNode->connectedNodes[index]] = newCost;
				nodeBeforeMe[currentNode->connectedNodes[index]] = currentNode;	// Insert this node into all the maps and queue
				int newPriority = newCost + Heuristics(currentNode->connectedNodes[index]->ID, goal);
				std::pair<int, cNode*> nextNode(newPriority, currentNode->connectedNodes[index]);
				frontier.push(nextNode);
			}
		}
	}

	if (exitFound)
	{	// If we never found an exit, we can't really conclude the pathfinding exercise
		cNode* step = nodes[goal];

		while (step->ID != start)
		{	// We start at the end, and follow the path back to the starting node
			pathToVictory.push_back(step);
			step = nodeBeforeMe[step];
		}
		pathToVictory.push_back(step);

		std::reverse(pathToVictory.begin(), pathToVictory.end());	// Finally, we reverse the path

		for (int index = 0; index < 100; index++)
		{	// A text representation of the maze. 0s are walkable paths, G is the goal, S is the start node
			if (nodeText[index] == '0' || nodeText[index] == 'G' || nodeText[index] == 'S')
				std::cout << nodeText[index];
			else
				std::cout << ' ';
			if (index % 10 == 9)
				std::cout << '\n';
			else
				std::cout << ' ';
		}

		std::cout << "\n\n\nPATH TO VICTORY\n\n";

		for (int index = 0; index < pathToVictory.size(); index++)
		{	// Also writing out the path we found using the node IDs. Just for fun
			std::cout << pathToVictory[index]->ID;
			if (index != pathToVictory.size() - 1)
				std::cout << " -> ";
		}
	}
	
	else
	{
		std::cout << "No path was found. We have failed utterly." << std::endl;
	}

	// We'll keep track of which node the ball is on
	int posOnPath = 1;

	while (!glfwWindowShouldClose(window))
	{
		float ratio;
		int width, height;
		glm::mat4x4 m, p;

		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;
		glViewport(0, 0, width, height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		if (exitFound)
		{	// If we never found an exit, don't bother doing any of this
			if (posOnPath < pathToVictory.size())
			{	// Grab the coordinates of the node our ball player should be heading towards
				cNode* nextNode = pathToVictory[posOnPath];
				float nextXCoord = (float)(nextNode->ID % 10);
				float nextZCoord = (float)(nextNode->ID / 10);

				// Adjust x and z values appropriately
				if (::g_vecGameObject[0]->position.x < nextXCoord)
				{
					::g_vecGameObject[0]->position.x += 0.05f;
					if (::g_vecGameObject[0]->position.x > nextXCoord)
						::g_vecGameObject[0]->position.x = nextXCoord;
				}
				else if (::g_vecGameObject[0]->position.x > nextXCoord)
				{
					::g_vecGameObject[0]->position.x -= 0.05f;
					if (::g_vecGameObject[0]->position.x < nextXCoord)
						::g_vecGameObject[0]->position.x = nextXCoord;
				}
				else if (::g_vecGameObject[0]->position.z < nextZCoord)
				{
					::g_vecGameObject[0]->position.z += 0.05f;
					if (::g_vecGameObject[0]->position.z > nextZCoord)
						::g_vecGameObject[0]->position.z = nextZCoord;
				}
				else if (::g_vecGameObject[0]->position.z > nextZCoord)
				{
					::g_vecGameObject[0]->position.z -= 0.05f;
					if (::g_vecGameObject[0]->position.z < nextZCoord)
						::g_vecGameObject[0]->position.z = nextZCoord;
				}
				else
				{	// If the ball has reached the next node, start heading for the one after that
					posOnPath++;
				}
			}

			else
			{		// Once the ball has reached the goal, we teleport to the next level
				if (::g_vecGameObject[0]->scale > 0)
				{
					::g_vecGameObject[0]->scale -= 0.01;
					::g_vecGameObject[1]->scale -= 0.01;
				}
			}
		}

		unsigned int sizeOfVector = ::g_vecGameObject.size();
		for (int index = 0; index != sizeOfVector; index++)
		{
			// Is there a game object? 
			if (::g_vecGameObject[index] == 0)	//if ( ::g_GameObjects[index] == 0 )
			{	// Nothing to draw
				continue;		// Skip all for loop code and go to next
			}

			// Was near the draw call, but we need the mesh name
			std::string meshToDraw = ::g_vecGameObject[index]->meshName;		//::g_GameObjects[index]->meshName;

			sVAOInfo VAODrawInfo;
			if (::g_pVAOManager->lookupVAOFromName(meshToDraw, VAODrawInfo) == false)
			{	// Didn't find mesh
				continue;
			}

			m = glm::mat4x4(1.0f);

			glm::mat4 matPreRotX = glm::mat4x4(1.0f);
			matPreRotX = glm::rotate(matPreRotX, ::g_vecGameObject[index]->orientation.x,
				glm::vec3(0.0f, 0.0f, 1.0f));
			m = m * matPreRotX;

			glm::mat4 matPreRotY = glm::mat4x4(1.0f);
			matPreRotY = glm::rotate(matPreRotY, ::g_vecGameObject[index]->orientation.y,
				glm::vec3(0.0f, 0.0f, 1.0f));
			m = m * matPreRotY;

			glm::mat4 matPreRotZ = glm::mat4x4(1.0f);
			matPreRotZ = glm::rotate(matPreRotZ, ::g_vecGameObject[index]->orientation.z,
				glm::vec3(0.0f, 0.0f, 1.0f));
			m = m * matPreRotZ;

			glm::mat4 trans = glm::mat4x4(1.0f);
			trans = glm::translate(trans,
				::g_vecGameObject[index]->position);
			m = m * trans;

			glm::mat4 matPostRotZ = glm::mat4x4(1.0f);
			matPostRotZ = glm::rotate(matPostRotZ, ::g_vecGameObject[index]->orientation2.z,
				glm::vec3(0.0f, 0.0f, 1.0f));
			m = m * matPostRotZ;

			glm::mat4 matPostRotY = glm::mat4x4(1.0f);
			matPostRotY = glm::rotate(matPostRotY, ::g_vecGameObject[index]->orientation2.y,
				glm::vec3(0.0f, 1.0f, 0.0f));
			m = m * matPostRotY;


			glm::mat4 matPostRotX = glm::mat4x4(1.0f);
			matPostRotX = glm::rotate(matPostRotX, ::g_vecGameObject[index]->orientation2.x,
				glm::vec3(1.0f, 0.0f, 0.0f));
			m = m * matPostRotX;

			float finalScale = ::g_vecGameObject[index]->scale;
			glm::mat4 matScale = glm::mat4x4(1.0f);
			matScale = glm::scale(matScale,
				glm::vec3(finalScale,
					finalScale,
					finalScale));
			m = m * matScale;

			p = glm::perspective(0.6f,			// FOV
				ratio,		// Aspect ratio
				0.1f,			// Near (as big as possible)
				1000.0f);	// Far (as small as possible)

							// View or "camera" matrix
			glm::mat4 v = glm::mat4(1.0f);	// identity

			g_cameraXYZ.x = camPosx;
			g_cameraXYZ.y = camPosy;
			g_cameraXYZ.z = camPosz;

			g_cameraTarget_XYZ.x = lookx;
			g_cameraTarget_XYZ.y = looky;
			g_cameraTarget_XYZ.z = lookz;

			v = glm::lookAt(g_cameraXYZ,						// "eye" or "camera" position
				g_cameraTarget_XYZ,		// "At" or "target" 
				glm::vec3(0.0f, 1.0f, 0.0f));	// "up" vector

			v = glm::lookAt(g_cameraXYZ,						// "eye" or "camera" position
				g_cameraTarget_XYZ,		// "At" or "target" 
				glm::vec3(0.0f, 1.0f, 0.0f));	// "up" vector


			glUniform4f(uniLoc_materialDiffuse,
				::g_vecGameObject[index]->diffuseColour.r,
				::g_vecGameObject[index]->diffuseColour.g,
				::g_vecGameObject[index]->diffuseColour.b,
				::g_vecGameObject[index]->diffuseColour.a);


			//        glUseProgram(program);
			::g_pShaderManager->useShaderProgram("simpleShader");

			glUniformMatrix4fv(uniLoc_mModel, 1, GL_FALSE,
				(const GLfloat*)glm::value_ptr(m));

			glUniformMatrix4fv(uniLoc_mView, 1, GL_FALSE,
				(const GLfloat*)glm::value_ptr(v));

			glUniformMatrix4fv(uniLoc_mProjection, 1, GL_FALSE,
				(const GLfloat*)glm::value_ptr(p));

			if (::g_vecGameObject[index]->wireFrame)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			glBindVertexArray(VAODrawInfo.VAO_ID);

			glDrawElements(GL_TRIANGLES,
				VAODrawInfo.numberOfIndices,
				GL_UNSIGNED_INT,
				0);

			glBindVertexArray(0);

		}//for ( int index = 0...

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

int Heuristics(int nodeID, int goalNode)
{
	int xCoord = nodeID / 10;
	int yCoord = nodeID % 10;

	int xGoal = goalNode / 10;
	int yGoal = goalNode % 10;

	int dist = abs(xCoord - xGoal) + abs(yCoord - yGoal);
	return dist;
}

bool LoadPlyFileIntoMesh(std::string filename, cMesh &theMesh)
{
	std::ifstream plyFile(filename.c_str());

	if (!plyFile.is_open())
	{	// Didn't open file, so return
		return false;
	}

	ReadFileToToken(plyFile, "vertex");
	plyFile >> theMesh.numberOfVertices;

	ReadFileToToken(plyFile, "face");
	plyFile >> theMesh.numberOfTriangles;

	ReadFileToToken(plyFile, "end_header");

	// Allocate the appropriate sized array (+a little bit)
	theMesh.pVertices = new cVertex_xyz_rgb_n[theMesh.numberOfVertices];
	theMesh.pTriangles = new cTriangle[theMesh.numberOfTriangles];

	// Read vertices
	for (int index = 0; index < theMesh.numberOfVertices; index++)
	{
		float x, y, z, nx, ny, nz;

		plyFile >> x;
		plyFile >> y;
		plyFile >> z;
		plyFile >> nx;
		plyFile >> ny;
		plyFile >> nz;


		theMesh.pVertices[index].x = x;
		theMesh.pVertices[index].y = y;
		theMesh.pVertices[index].z = z;
		theMesh.pVertices[index].r = 1.0f;
		theMesh.pVertices[index].g = 1.0f;
		theMesh.pVertices[index].b = 1.0f;
		theMesh.pVertices[index].nx = nx;
		theMesh.pVertices[index].ny = ny;
		theMesh.pVertices[index].nz = nz;
	}

	// Load the triangle (or face) information, too
	for (int count = 0; count < theMesh.numberOfTriangles; count++)
	{
		int discard = 0;
		plyFile >> discard;
		plyFile >> theMesh.pTriangles[count].vertex_ID_0;
		plyFile >> theMesh.pTriangles[count].vertex_ID_1;
		plyFile >> theMesh.pTriangles[count].vertex_ID_2;
	}

	return true;
}

void ReadFileToToken(std::ifstream &file, std::string token)
{
	bool bKeepReading = true;
	std::string garbage;
	do
	{
		file >> garbage;		// Title_End??
		if (garbage == token)
		{
			return;
		}
	} while (bKeepReading);
	return;
}