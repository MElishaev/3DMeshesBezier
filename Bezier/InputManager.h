#pragma once   //maybe should be static class
#include "display.h"
#include "renderer.h"
#include "game.h"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>

#define PI 3.141592
#define EPSILON 0.01

void mouse_callback(GLFWwindow* window,int button, int action, int mods)
{	
	if(action == GLFW_PRESS )
	{
		Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
		Game* scn = (Game*)rndr->GetScene();
		double x2,y2;
		glfwGetCursorPos(window,&x2,&y2);
			
		if (rndr->Picking((int)x2, (int)y2))
		{
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				if (scn->getPickedShape() == 1)
				{
					scn->SaveScissorsStart(x2, y2);
					if (!scn->IsScissoring())
					{
						scn->ToggleScissoring();
						rndr->SetDrawFlag(3, rndr->scissorTest);
						rndr->SetDrawFlag(3, rndr->blend);
					}
				}
				else
					scn->pOnLine();
			}
		}
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{	
			scn->splitBezier((float)x2, (float)y2);
		}
	}
	else if (action == GLFW_RELEASE)
	{
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
		{
			Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
			Game* scn = (Game*)rndr->GetScene();
			double x2,y2;
			glfwGetCursorPos(window,&x2,&y2);

			if (scn->IsScissoring())
			{
				rndr->ClearDrawFlag(3, rndr->scissorTest);
				rndr->ClearDrawFlag(3, rndr->blend);
				rndr->Picking(x2, y2);
				scn->ToggleScissoring();
				scn->ResetScissoringWindow();
			}
		}
	}
	
}
	
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	Game* scn = (Game*)rndr->GetScene();
	double x, y;
	glfwGetCursorPos(window,&x,&y);

	int picked = scn->getPickedShape();

	std::vector<int> shapes = scn->getPickedShapes();
	
	// if picked not in picked shapes, insert it
	std::vector<int>::iterator elem;
	elem=std::find(shapes.begin(), shapes.end(), picked);
	if (elem == shapes.end())
		shapes.push_back(picked);

	for (int s : shapes)
	{
		if (s > scn->GetLastOctaIndex()) // if we picked a bezier surface, translate it
		{
			scn->setPickedShape(s);
			// when scrolling, move selected shape towards/backwards the camera
			glm::vec3 cameraPos = glm::vec3(rndr->GetCameraTransformation(0)[3]);
			glm::vec3 cameraDir = glm::normalize(cameraPos - scn->GetShapePosition(s));
			if (glm::distance(cameraPos, scn->GetShapePosition(s)) < 2 && yoffset > 0)
				continue;
			scn->ShapeTransformation(scn->zTranslate, cameraDir.z * (float) yoffset);
			scn->ShapeTransformation(scn->yTranslate, cameraDir.y * (float) yoffset);
			scn->ShapeTransformation(scn->xTranslate, cameraDir.x * (float)yoffset);
		}
		else if(rndr->checkViewport((int)x, (int)y, 1))
			rndr->scaleCamera(1,(float)yoffset);
	}

	scn->setPickedShape(picked);
}
	
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	Game* scn = (Game*)rndr->GetScene();
	rndr->UpdatePosition((float)xpos,(float)ypos);
	scn->UpdatePosition((float)xpos, (float)ypos);

	int picked = scn->getPickedShape();
	std::vector<int> shapes = scn->getPickedShapes();
	
	// if picked not in picked shapes, insert it
	std::vector<int>::iterator elem;
	elem=std::find(shapes.begin(), shapes.end(), picked);
	if (elem == shapes.end())
		shapes.push_back(picked);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		if (picked <= scn->GetLastOctaIndex())
			scn->translateShape((float)xpos, (float)ypos);
		else // in viewport 0
		{
			for (int s : shapes)
			{
				if (s != 1)
					scn->TranslateShapeInCameraPlane(s, rndr->GetCameraTransformation(0));		
			}
		}
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		for (int s : shapes)
		{
			if (s > scn->GetLastOctaIndex())
				scn->RotateShapeInplace(s);
			else if (picked == 1)
				scn->CreateScissorsPlane(xpos, ypos);
		}

		scn->setPickedShape(picked);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		scn->resetRelativePos();
	}

}

void window_size_callback(GLFWwindow* window, int width, int height)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
		
	rndr->Resize(width,height);
}
	
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	Game* scn = (Game*)rndr->GetScene();

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_SPACE:
			scn->create2DBezier();
			break;
		case GLFW_KEY_2:
		case GLFW_KEY_3:
		case GLFW_KEY_4:
		case GLFW_KEY_5:
		case GLFW_KEY_6:
			key -= 48;
			scn->deleteBezier();
			scn->initOcta(key);
			break;
		case GLFW_KEY_C:
			scn->toggleC_state();
			break;
		case GLFW_KEY_DOWN:
		case GLFW_KEY_UP:
		{
			glm::vec3 cameraPos = glm::vec3(rndr->GetCameraTransformation(0)[3]);
			glm::mat3 cameraRotation = glm::mat3(rndr->GetCameraTransformation(0));
			glm::vec3 cameraDir = glm::normalize(cameraRotation * glm::vec3(0, 0, 1));
			glm::vec3 cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraDir));

			float cameraXrotation = atan2(cameraRotation[2][1], cameraRotation[2][2]); // in radians
			if ((abs(-cameraXrotation + 0.04 - PI / 2) < EPSILON && key == GLFW_KEY_UP) || 
				(abs(-cameraXrotation - 0.04 + PI / 2) < EPSILON && key == GLFW_KEY_DOWN))
				break;

			rndr->TranslateCamera(0, -cameraPos);
			scn->TranslateWindow(-cameraPos);
			if (key == GLFW_KEY_DOWN)
			{
				rndr->RotateCamera(0, cameraRight, -0.02f);
				scn->RotateWindow(-0.02f, cameraRight);
			}
			else
			{
				rndr->RotateCamera(0, cameraRight, 0.02f);
				scn->RotateWindow(0.02f, cameraRight);
			}
			rndr->TranslateCamera(0, cameraPos);
			scn->TranslateWindow(cameraPos);
			break;
		}
		case GLFW_KEY_LEFT:
		{
			glm::vec3 cameraPos = glm::vec3(rndr->GetCameraTransformation(0)[3]);
			glm::mat3 cameraRotation = glm::mat3(rndr->GetCameraTransformation(0));
			glm::vec3 cameraDir = glm::normalize(cameraRotation * glm::vec3(0, 0, 1));
			rndr->TranslateCamera(0, -cameraPos);
			scn->TranslateWindow(-cameraPos);

			scn->RotateWindow(0.02f, glm::vec3(0,1,0));
			rndr->MoveCamera(0, Renderer::yRotate, 0.02f);

			rndr->TranslateCamera(0, cameraPos);
			scn->TranslateWindow(cameraPos);


			break;
		}
		case GLFW_KEY_RIGHT:
		{
			glm::vec3 cameraPos = glm::vec3(rndr->GetCameraTransformation(0)[3]);
			glm::mat3 cameraRotation = glm::mat3(rndr->GetCameraTransformation(0));
			glm::vec3 cameraDir = glm::normalize(cameraRotation * glm::vec3(0, 0, 1));

			rndr->TranslateCamera(0, -cameraPos);
			scn->TranslateWindow(-cameraPos);

			scn->RotateWindow(-0.02f, glm::vec3(0,1,0));
			rndr->MoveCamera(0, Renderer::yRotate, -0.02f);

			rndr->TranslateCamera(0, cameraPos);
			scn->TranslateWindow(cameraPos);
			break;
		}
		case GLFW_KEY_R:
		case GLFW_KEY_L:
		case GLFW_KEY_U:
		case GLFW_KEY_D:
		case GLFW_KEY_F:
		case GLFW_KEY_B:
		{
			glm::vec3 cameraPos = glm::vec3(rndr->GetCameraTransformation(0)[3]);
			glm::mat3 cameraRotation = glm::mat3(rndr->GetCameraTransformation(0));
			glm::vec3 cameraDir = glm::normalize(cameraRotation * glm::vec3(0, 0, 1));
			glm::vec3 cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraDir));
			glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDir, cameraRight));
			float delta = 0.1f;
			if (key == GLFW_KEY_R)
			{
				rndr->TranslateCamera(0, cameraRight*delta);
				scn->TranslateWindow(cameraRight*delta);
			}
			else if (key == GLFW_KEY_L)
			{
				rndr->TranslateCamera(0, -cameraRight*delta);
				scn->TranslateWindow(-cameraRight*delta);
			}
			else if (key == GLFW_KEY_U)
			{
				rndr->TranslateCamera(0, cameraUp*delta);
				scn->TranslateWindow(cameraUp*delta);
			}
			else if (key == GLFW_KEY_D)
			{
				rndr->TranslateCamera(0, -cameraUp*delta);
				scn->TranslateWindow(-cameraUp*delta);
			}
			else if (key == GLFW_KEY_F)
			{
				rndr->TranslateCamera(0, -cameraDir*delta);
				scn->TranslateWindow(-cameraDir*delta);
			}
			else
			{
				rndr->TranslateCamera(0, cameraDir*delta);
				scn->TranslateWindow(cameraDir*delta);
			}
			break;
		}
		default:
			break;
		}
	}
}

void Init(Display &display)
{
	display.AddKeyCallBack(key_callback);
	display.AddMouseCallBacks(mouse_callback,scroll_callback,cursor_position_callback);
	display.AddResizeCallBack(window_size_callback);
}
