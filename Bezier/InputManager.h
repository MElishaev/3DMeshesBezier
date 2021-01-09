#pragma once   //maybe should be static class
#include "display.h"
#include "renderer.h"
#include "game.h"
#include <iostream>

void mouse_callback(GLFWwindow* window,int button, int action, int mods)
{	
		
	if(action == GLFW_PRESS )
	{
		Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
		Game* scn = (Game*)rndr->GetScene();
		double x2,y2;
		glfwGetCursorPos(window,&x2,&y2);
		std::cout << "pressed on: " << x2 << " " << y2 << std::endl;
			
		if (rndr->Picking((int)x2, (int)y2))
		{
			std::cout << "in picking condition\n";
			/*Position pos = scn->getShapePosition(scn->getPickedShape());*/
		}
	}
}
	
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	Game* scn = (Game*)rndr->GetScene();

	rndr->MoveCamera(0, scn->zTranslate, yoffset);
}
	
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	Game* scn = (Game*)rndr->GetScene();
	rndr->UpdatePosition((float)xpos,(float)ypos);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		//rndr->MouseProccessing(GLFW_MOUSE_BUTTON_RIGHT);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		//rndr->MouseProccessing(GLFW_MOUSE_BUTTON_LEFT);
		//std::cout << "in cursor pos callback: " << xpos << ", " << ypos << std::endl;
		scn->UpdatePosition((float)xpos, (float)ypos);
		unsigned int p = scn->getPickedShape();
		scn->translateShape(p);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
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
		{
			
		}
		case GLFW_KEY_UP:
			//rndr->MoveCamera(0, scn->zTranslate, 0.4f);
			break;
		case GLFW_KEY_DOWN:
			//rndr->MoveCamera(0, scn->zTranslate, -0.4f);
			break;
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
