#include "game.h"
#include <iostream>
#include "GL/glew.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "renderer.h"
#include "display.h"
#include "Bezier1D.h"


#define EPSILON 0.001

static void printMat(const glm::mat4 mat)
{
	std::cout<<" matrix:"<<std::endl;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			std::cout<< mat[j][i]<<" ";
		std::cout<<std::endl;
	}
}

Game::Game() : Scene()
{
}

void Game::Init()
{		
	unsigned int texIDs[3] = { 0 , 1, 0 };
	unsigned int slots[3] = { 0 , 1, 0 };

	AddShader("../res/shaders/pickingShader2");
	//AddShader("../res/shaders/basicShader2");
	AddShader("../res/shaders/basicShader");
	AddTexture("../res/textures/plane.png", 2);
	AddMaterial(texIDs, slots, 1);

	AddShape(Axis, -1, LINES);
	AddShapeViewport(0, 1);
	RemoveShapeViewport(0, 0);
	SetShapeShader(0, 1);

	// init bezier curve
	Shape* obj = new Bezier1D(3, 20, LINE_STRIP, 1);
	chainParents.push_back(-1);
	shapes.push_back(obj);
	RemoveShapeViewport(1, 0);

	// add octahedrons to scene
	for (int seg=0; seg < ((Bezier1D*)obj)->GetSegmentsNum(); seg++)
		for(int cp=0; cp < 4; cp++)
		{
			if (seg > 0 && seg < ((Bezier1D*)obj)->GetSegmentsNum() && cp == 0) // don't draw duplicate octahendrons on joint control points
				continue;
			AddShape(Octahedron, -1, TRIANGLES);
			unsigned int shape_id = shapes.size() - 1;
			AddShapeViewport(shape_id, 1);
			RemoveShapeViewport(shape_id, 0);
			SetShapeShader(shape_id, 1);
			shapes[shape_id]->MyScale(glm::vec3(0.02, 0.02, 0.02));
			glm::vec4 controlP = ((Bezier1D*)obj)->GetControlPoint(seg, cp);
			shapes[shape_id]->MyTranslate(glm::vec3(controlP.x,controlP.y,controlP.z), 0);
		}

	//ReadPixel(); //uncomment when you are reading from the z-buffer
}

void Game::UpdatePosition(float x, float y)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	x = x / viewport[2];
	y = 1 - y / viewport[3];

	if (xold == -1)
	{
		xold = x;
		yold = y;
	}
	// x position of octahedrons out of viewport 1 bounds
	if (x <= 1 || x >= 2) xrel = 0;
	else xrel = x - xold;

	// edge control points to descend beyond y axis
	if ((y < 0.5 && (pickedShape - 2) % 3 == 0) || (y >= 1 || y <= 0)) yrel = 0;
	else yrel = y - yold;

	if (!(x <= 1 || x >= 2)) xold = x;
	if (!(y < 0.5 && (pickedShape - 2) % 3 == 0) || (y >= 1 || y <= 0)) 
		yold = y;
}

void Game::translateShape(const unsigned int pickedShape)
{
	//((Bezier1D*)shapes[1])->
	if(pickedShape >= 2 && pickedShape < shapes.size())
		// multiplied by 2 because of relative movement of the mouse on window that divided to 2 viewports
		shapes[pickedShape]->MyTranslate(glm::vec3(2*xrel, 2*yrel, 0), 0);
}

void Game::Update(const glm::mat4 &MVP,const glm::mat4 &Model,const int  shaderIndx)
{
	Shader *s = shaders[shaderIndx];
	int r = ((pickedShape+1) & 0x000000FF) >>  0;
	int g = ((pickedShape+1) & 0x0000FF00) >>  8;
	int b = ((pickedShape+1) & 0x00FF0000) >> 16;
	if (shapes[pickedShape]->GetMaterial() >= 0 && !materials.empty())
		BindMaterial(s, shapes[pickedShape]->GetMaterial());
	//textures[0]->Bind(0);
	s->Bind();
	if (shaderIndx != 2)
	{
		s->SetUniformMat4f("MVP", MVP);
		s->SetUniformMat4f("Normal", Model);
	}
	else
	{
		s->SetUniformMat4f("MVP", glm::mat4(1));
		s->SetUniformMat4f("Normal", glm::mat4(1));
	}
	s->SetUniform1i("sampler1", materials[shapes[pickedShape]->GetMaterial()]->GetSlot(0));
	if(shaderIndx!=2)
		s->SetUniform1i("sampler2", materials[shapes[pickedShape]->GetMaterial()]->GetSlot(1));

	s->Unbind();
}

void Game::WhenRotate()
{
}

void Game::WhenTranslate()
{
}

void Game::Motion()
{
	
}

unsigned int Game::TextureDesine(size_t width, size_t height)
{
	unsigned char* data = new unsigned char[width * height * 4];
	for (size_t i = 0; i < width; i++)
	{
		for (size_t j = 0; j < height; j++)
		{
			data[(i * height + j) * 4] = (i + j) % 256;
			data[(i * height + j) * 4 + 1] = (i + j * 2) % 256;
			data[(i * height + j) * 4 + 2] = (i * 2 + j) % 256;
			data[(i * height + j) * 4 + 3] = (i * 3 + j) % 256;
		}
	}
	textures.push_back(new Texture(width, height));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //note GL_RED internal format, to save 
	glBindTexture(GL_TEXTURE_2D, 0);
	delete[] data;
	return(textures.size() - 1);
}

Game::~Game(void)
{

}
