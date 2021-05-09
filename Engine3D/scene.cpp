#define GLEW_STATIC
#include "GL/glew.h"
#include "scene.h"
#include <iostream>

static void printMat(const glm::mat4 mat)
{
	printf(" matrix: \n");
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			printf("%f ", mat[j][i]);
		printf("\n");
	}
}

Scene::Scene()
{
	glLineWidth(5);

	pickedShape = -1;
	depth = 0;

	isActive = true;
}

void Scene::AddShapeFromFile(const std::string& fileName, int parent, unsigned int mode)
{
	chainParents.push_back(parent);
	shapes.push_back(new Shape(fileName, mode));
}

void Scene::AddShape(int type, int parent, unsigned int mode)
{
	chainParents.push_back(parent);
	shapes.push_back(new Shape(type, mode));
}

void Scene::AddShapeCopy(int indx, int parent, unsigned int mode)
{
	chainParents.push_back(parent);
	shapes.push_back(new Shape(*shapes[indx], mode));
}

int Scene::AddShader(const std::string& fileName)
{
	shaders.push_back(new Shader(fileName));
	return (shaders.size() - 1);
}

int  Scene::AddTexture(const std::vector<std::string>& faces)
{
	textures.push_back(new Texture(faces));
	return(textures.size() - 1);
}

int  Scene::AddTexture(const std::string& textureFileName, int dim)
{
	textures.push_back(new Texture(textureFileName, dim));
	return(textures.size() - 1);
}

int Scene::AddTexture(int width, int height, unsigned char* data, int mode)
{
	textures.push_back(new Texture(width, height));

	switch (mode)
	{
	case COLOR:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //note GL_RED internal format, to save memory.
		break;
	case DEPTH:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, data);
		break;
	case STENCIL:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, data);
		break;
	default:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //note GL_RED internal format, to save memory.
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	return(textures.size() - 1);
}

int Scene::AddMaterial(unsigned int texIndices[], unsigned int slots[], unsigned int size)
{
	materials.push_back(new Material(texIndices, slots, size));
	return (materials.size() - 1);
}

void Scene::ToggleScissoring()
{
	if (!isScissor)
		pickedShapes.erase(pickedShapes.begin(), pickedShapes.end());
	isScissor = !isScissor;
}

void Scene::Draw(int shaderIndx, const glm::mat4& VP, const glm::mat4& view, const glm::mat4& proj ,int viewportIndx, unsigned int flags) 
{
	glm::mat4 Normal = MakeTrans();

	int p = pickedShape;


	for (pickedShape = 0; pickedShape < shapes.size(); pickedShape++)
	{
		if (shapes[pickedShape]->Is2Render(viewportIndx))
		{
			glm::mat4 Model = Normal * shapes[pickedShape]->MakeTrans();

			if (shaderIndx > 0)
			{
				Update(view, proj, VP, Model, shapes[pickedShape]->GetShader());
				
				if (viewportIndx == 0)
				{
					if (flags & 8) // scissors flag
					{
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						glScissor(scissorsWindow.x, scissorsWindow.y, scissorsWindow.z, scissorsWindow.w);
						Model = Normal * SelectionWindow->MakeTrans();
						Update(view, proj, VP, Model, 3);
						SelectionWindow->Draw(shaders[3], false);
					}
					else if(flags & 32)
					{
						glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); 
						if ((pickedShape > 1 && p == pickedShape) || std::find(pickedShapes.begin(), pickedShapes.end(), pickedShape) != pickedShapes.end())
						{
							glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 
							// draw colored shapes
							glStencilFunc(GL_ALWAYS, 1, 0xFF);
							shapes[pickedShape]->Draw(shaders[shapes[pickedShape]->GetShader()], false);

							// draw outline
							glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
							glDisable(GL_DEPTH_TEST);
							shapes[pickedShape]->MyScale(glm::vec3(1.1, 1.1, 1.1));
							Model = Normal * shapes[pickedShape]->MakeTrans();
							Update(view, proj, VP, Model, 3);
							shapes[pickedShape]->Draw(shaders[3], false);
							shapes[pickedShape]->MyScale(glm::vec3(1/1.1, 1/1.1, 1/1.1));
							glEnable(GL_DEPTH_TEST); 
							glStencilFunc(GL_ALWAYS, 1, 0xFF);
						}
						else
							shapes[pickedShape]->Draw(shaders[shapes[pickedShape]->GetShader()], false);
					}
				}
				else // viewport != 0
					shapes[pickedShape]->Draw(shaders[shapes[pickedShape]->GetShader()], false);
			}
			else
			{ //picking
				Update(view, proj, VP, Model, 0);
				shaders[0]->Bind();
				shaders[0]->SetUniform1i("id", pickedShape+1);
				shaders[0]->Unbind();
				shapes[pickedShape]->Draw(shaders[0], true);
			}
		}
	}
	pickedShape = p;
}

void Scene::ShapeTransformation(int type, float amt)
{
	if (glm::abs(amt) > 1e-5)
	{
		switch (type)
		{
		case xTranslate:
			shapes[pickedShape]->MyTranslate(glm::vec3(amt, 0, 0), 0);
			break;
		case yTranslate:
			shapes[pickedShape]->MyTranslate(glm::vec3(0, amt, 0), 0);
			break;
		case zTranslate:
			shapes[pickedShape]->MyTranslate(glm::vec3(0, 0, amt), 0);
			break;
		case xRotate:
			shapes[pickedShape]->MyRotate(amt, glm::vec3(1, 0, 0), 0);
			break;
		case yRotate:
			shapes[pickedShape]->MyRotate(amt, glm::vec3(0, 1, 0), 0);
			break;
		case zRotate:
			shapes[pickedShape]->MyRotate(amt, glm::vec3(0, 0, 1), 0);
			break;
		default:
			break;
		}
	}

}

bool Scene::Picking(unsigned char data[4])
{
	pickedShape = (int)data[0] - 1;
	if (int(data[0]) > 1)
	{
		std::cout << "picked shape: " << pickedShape << "\n";
		return true;
	}
	return false;
	//WhenPicked();	
}

bool Scene::WindowPicking(unsigned char * pixelsData)
{
	int pixelOffset = 4;
	int width = scissorsWindow.z;
	int height = scissorsWindow.w;
	int cubeMapIndx = 1;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	std::vector<int> notToCheck;
	unsigned char R_pixel;

	for(int r = 1; r < height-1; r++)
		for (int c = 1; c < width-1; c++)
		{
			int indx = (r * width + c) * pixelOffset;
			int shapeIndx = int(pixelsData[indx]) - 1;
			if (std::find(notToCheck.begin(), notToCheck.end(), shapeIndx) == notToCheck.end()) // if not in notToCheck
			{
				if (shapeIndx != cubeMapIndx && shapeIndx != 204)
				{
					// if need to check shape and shape not yet in pickedShapes, insert it
					if(std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx) == pickedShapes.end())
						pickedShapes.push_back(shapeIndx);

					if (r == 1)
					{
						if (scissorsWindow.y - 1 < 0) // if shape on the edge of the scissor window and edge of screen, not picked
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
						R_pixel = pixelsData[((r - 1)*width + c)*pixelOffset];
						if (int(R_pixel) - 1 == shapeIndx)
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
					}
					else if (r == height - 2)
					{
						if (scissorsWindow.y + 1 >= viewport[3]) // if shape on the edge of the scissor window and edge of screen, not picked
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
						R_pixel = pixelsData[((r + 1)*width + c)*pixelOffset];
						if (int(R_pixel) - 1 == shapeIndx)
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
					}
				
					if (c == 1)
					{
						if (scissorsWindow.x - 1 < 0) // if shape on the edge of the scissor window and edge of screen, not picked
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
						R_pixel = pixelsData[(r*width + c-1)*pixelOffset];
						if (int(R_pixel) - 1 == shapeIndx)
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
					}
					else if (c == width - 2)
					{
						if (scissorsWindow.x + 1 >= viewport[2]) // if shape on the edge of the scissor window and edge of screen, not picked
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
						R_pixel = pixelsData[(r*width + c+1)*pixelOffset];
						if (int(R_pixel) - 1 == shapeIndx)
						{
							std::vector<int>::iterator toErase;
							toErase=std::find(pickedShapes.begin(), pickedShapes.end(), shapeIndx);
							if (toErase != pickedShapes.end())
								pickedShapes.erase(toErase);

							notToCheck.push_back(shapeIndx);
							continue;
						}
					}
				}
			}
		}
	delete(pixelsData);
	if (pickedShapes.empty())
		return false;
	return true;
}

//return coordinates in global system for a tip of arm position is local system 
void Scene::MouseProccessing(int button, int xrel, int yrel)
{
	//if (pickedShape == -1)
	//{
	if (button == 1)
	{
		pickedShape = 0;
		ShapeTransformation(xTranslate, xrel / 80.0f);
		pickedShape = -1;
		//MyTranslate(glm::vec3(-xrel / 80.0f, 0, 0), 0);
		//MyTranslate(glm::vec3(0, yrel / 80.0f, 0), 0);
		WhenTranslate();
	}
	else
	{
		MyRotate(-xrel / 2.0f, glm::vec3(0, 1, 0), 0);
		MyRotate(-yrel / 2.0f, glm::vec3(1, 0, 0), 0);
		WhenRotate();
	}
	//}

}

void Scene::CreateScissorsPlane(double endPointX, double endPointY)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glm::vec2 lowerLeft = glm::vec2(fmin(scissorsX, endPointX), fmin(viewport[3] - scissorsY, viewport[3] - endPointY));
	glm::vec2 upperRight = glm::vec2(fmin(fmax(scissorsX, endPointX), viewport[2]), fmax(viewport[3] - scissorsY, viewport[3] - endPointY));
	glm::vec2 planeSize = upperRight - lowerLeft;
	scissorsWindow = glm::vec4(lowerLeft.x, lowerLeft.y, planeSize.x, planeSize.y);
}

void Scene::ZeroShapesTrans()
{
	for (unsigned int i = 0; i < shapes.size(); i++)
	{
		shapes[i]->ZeroTrans();
	}
}

void Scene::ReadPixel()
{
	glReadPixels(1, 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
}

void Scene::AddShapeViewport(int shpIndx,int viewPortIndx)
{
	shapes[shpIndx]->AddViewport(viewPortIndx);
}

void Scene::RemoveShapeViewport(int shpIndx, int viewPortIndx)
{
	shapes[shpIndx]->RemoveViewport(viewPortIndx);
}

void Scene::BindMaterial(Shader* s, unsigned int materialIndx)
{

	for (size_t i = 0; i < materials[materialIndx]->GetNumOfTexs(); i++)
	{
		materials[materialIndx]->Bind(textures, i);
		s->SetUniform1i("sampler" + std::to_string(i + 1), materials[materialIndx]->GetSlot(i));
	}
}

void Scene::AddShapeInPlace(int type, unsigned int mode, int place) {
	shapes.insert(shapes.begin() + place, new Shape(type, mode));
}

Scene::~Scene(void)
{
	for (Shape* shp : shapes)
	{
		delete shp;
	}

	for (Shader* sdr : shaders)
	{
		delete sdr;
	}
	for (Texture* tex : textures)
	{
		delete tex;
	}
	for (Material* mat : materials)
	{
		delete mat;
	}

}



