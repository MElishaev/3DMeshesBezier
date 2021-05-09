#include "game.h"
#include <iostream>
#include "GL/glew.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "renderer.h"
#include "display.h"
#include "Bezier1D.h"
#include "Bezier2D.h"

#define FRST_OCTA_INDX 3
#define EPSILON 0.01
#define PI 3.141592
#define GET_BEZIER_INDX(x) x-FRST_OCTA_INDX
#define GET_CP_INDX(x) (GET_BEZIER_INDX(x))%3
#define GET_SEG_INDX(x) (GET_BEZIER_INDX(x))/3

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
	AddShader("../res/shaders/cubemapShader");
	AddShader("../res/shaders/stencilShader");
	std::vector<std::string> faces =
	{
		"../res/textures/cubemap/Daylight Box_Right.bmp",
		"../res/textures/cubemap/Daylight Box_Left.bmp",
		"../res/textures/cubemap/Daylight Box_Top.bmp",
		"../res/textures/cubemap/Daylight Box_Bottom.bmp",
		"../res/textures/cubemap/Daylight Box_Front.bmp",
		"../res/textures/cubemap/Daylight Box_Back.bmp"
	};
	AddTexture(faces);
	AddMaterial(texIDs, slots, 1);

	AddTexture("../res/textures/box0.bmp", 2);
	AddMaterial(texIDs+1, slots, 1);

	// Scissors selection plane
	SelectionWindow = new Shape(Plane, TRIANGLES);
	SelectionWindow->MyScale(glm::vec3(100,100,100));
	SelectionWindow->MyTranslate(glm::vec3(0, 0, 8.8), 0);

	// adding axes
	AddShape(Axis, -1, LINES);
	AddShapeViewport(0, 1);
	RemoveShapeViewport(0, 0);
	SetShapeShader(0, 1);

	// adding cubemap
	AddShape(Cube, -1, TRIANGLES);
	shapes[shapes.size() - 1]->MyScale(glm::vec3(50, 50, 50));
	SetShapeShader(shapes.size() - 1, 2);
	SetShapeMaterial(shapes.size() - 1, 0);
	
	// init bezier curve
	initOcta(3);
	//ReadPixel(); //uncomment when you are reading from the z-buffer
}

void Game::deleteBezier()
{
	shapes.erase(shapes.begin() + bezierIndx, shapes.begin() + bezierIndx + 1 + ((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum() * 3 + 1);
}

void Game::initOcta(unsigned int const segNum)
{
	Shape* obj = new Bezier1D(segNum, 20, LINE_STRIP, 1);
	chainParents.push_back(-1);

	bool bIsSurfacesRendered = shapes.size() > 2;

	if (bIsSurfacesRendered)
	{
		shapes.insert(shapes.begin() + 2, obj);
		bezierIndx = 2;
	}
	else
	{
		shapes.push_back(obj);
		bezierIndx = shapes.size() - 1;
	}

	RemoveShapeViewport(bezierIndx, 0);

	// add octahedrons to scene
	for (int seg=0; seg < ((Bezier1D*)obj)->GetSegmentsNum(); seg++)
		for(int cp=0; cp < 4; cp++)
		{
			// every segment doens't include its last control point except the 1st segment
			if (seg > 0 && cp == 0) // don't draw duplicate octahendrons on joint control points
				continue;
			unsigned int shape_id;
			if (bIsSurfacesRendered)
			{
				AddShapeInPlace(Octahedron, TRIANGLES, bezierIndx + 1 + seg * 3 + cp);
				shape_id = bezierIndx + 1 + seg * 3 + cp;
			}
			else
			{
				AddShape(Octahedron, -1, TRIANGLES);
				shape_id = shapes.size() - 1;
			}
			AddShapeViewport(shape_id, 1);
			RemoveShapeViewport(shape_id, 0);
			SetShapeShader(shape_id, 1);
			shapes[shape_id]->MyScale(glm::vec3(0.02, 0.02, 0.02));
			glm::vec4 controlP = ((Bezier1D*)obj)->GetControlPoint(seg, cp);
			shapes[shape_id]->MyTranslate(glm::vec3(controlP.x,controlP.y,controlP.z), 0);
		}
	pickedShape = -1;
	lastOctaIndx = bezierIndx + 3 * segNum + 1;
}

void Game::create2DBezier()
{
	Bezier2D* surf = new Bezier2D((Bezier1D*)shapes[bezierIndx], 4, 20, 20, QUADS, 0);
	chainParents.push_back(-1);
	shapes.push_back(surf);
	int idx = shapes.size() - 1;
	SetShapeShader(idx, 1);
	SetShapeMaterial(idx, 1);
	AddShapeViewport(idx, 0);
	//RemoveShapeViewport(idx, 1);
}

void Game::ShapeTransformation(int type, float amount)
{
	Scene::ShapeTransformation(type, amount);
}

void Game::splitBezier(float xpos, float ypos)
{
	int pick = getPickedConvexHull(xpos,ypos);
	if (pick != -1 && ((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum() < 6) {

		//0. Get the picked segment and create new segments according to PS instructions
		glm::mat4 segment = ((Bezier1D*)shapes[bezierIndx])->GetSegment(pick);
		glm::vec4 l0 = segment[0];
		glm::vec4 r3 = segment[3];
		glm::vec4 l1 = 0.5f * (segment[0] + segment[1]);
		glm::vec4 r2 = 0.5f * (segment[2] + segment[3]);
		glm::vec4 l2 = 0.5f * (l1 + 0.5f * (segment[1] + segment[2]));
		glm::vec4 r1 = 0.5f * (r2 + 0.5f * (segment[1] + segment[2]));
		glm::vec4 l3 = 0.5f * (l2 + r1);
		glm::vec4 r0 = l3;

		//1. remove the current segment at number pick. 
		((Bezier1D*)shapes[bezierIndx])->removeSegment(pick);

		//2. remove the related octahedron shapes.
		shapes.erase(shapes.begin() + bezierIndx + pick * 3 + 2 , shapes.begin() + bezierIndx + pick * 3 + 4);

		//3.Put the segments in the right place in the bezier
		glm::mat4 segment1 = glm::mat4(l0, l1, l2, l3);
		glm::mat4 segment2 = glm::mat4(r0, r1, r2, r3);
		((Bezier1D*)shapes[bezierIndx])->AddSegmentInPlace(segment1, segment2, pick);

		//4. add the related octahedrons -- The octhahedrons needs to be rotated -- needs to be handled or maybe not? because it's an initialization.
		for(int j= 1; j<6; j++){
			int shape_id = pick * 3 + (j + bezierIndx + 1);
			AddShapeInPlace(Octahedron, TRIANGLES, shape_id);
			AddShapeViewport(shape_id, 1);
			RemoveShapeViewport(shape_id, 0);
			SetShapeShader(shape_id, 1);
			shapes[shape_id]->MyScale(glm::vec3(0.02, 0.02, 0.02));
			glm::vec4 controlP = j < 4 ? segment1[j % 4] : segment2[j % 3];
			shapes[shape_id]->MyTranslate(glm::vec3(controlP.x, controlP.y, controlP.z), 0);
		}
	}
	pickedShape = -1;
}

void Game::RotateShapeInplace(int shapeIndx)
{
	glm::vec3 shapePos = glm::vec3(shapes[shapeIndx]->MakeTrans()[3]);
	shapes[shapeIndx]->MyTranslate(-shapePos, 0);
	shapes[shapeIndx]->MyRotate(xrel*20, glm::vec3(0, 1, 0), 0);
	shapes[shapeIndx]->MyRotate(-yrel*20, glm::vec3(1, 0, 0), 0);
	shapes[shapeIndx]->MyTranslate(shapePos, 0);
}

void Game::UpdatePosition(float x, float y)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int vp = x > viewport[2] ? 1 : 0;
	x = x / viewport[2];
	y = 1 - y / viewport[3];

	if (xold == -1)
	{
		xold = x;
		yold = y;
	}

	if (vp == 1) // viewport 1
	{
		// x position of octahedrons out of viewport 1 bounds
		if (x <= 1 || x >= 2) 
			xrel = 0;
		else 
			xrel = x - xold;

		// edge control points to descend beyond y axis
		if ((y < 0.5 && GET_CP_INDX(pickedShape) == 0) || (y >= 1 || y <= 0)) 
			yrel = 0;
		else 
			yrel = y - yold;

		if (!(x <= 1 || x >= 2)) 
			xold = x;
		if (!(y < 0.5 && GET_CP_INDX(pickedShape) == 0) || (y >= 1 || y <= 0)) 
			yold = y;

	}
	else // viewport 0
	{
		xrel = x - xold;
		yrel = y - yold;
		xold = x;
		yold = y;
	}
}

void Game::TranslateShapeInCameraPlane(int shapeId, glm::mat4 cameraTransform)
{
	glm::mat3 cameraRotation = glm::mat3(cameraTransform);
	glm::vec3 cameraDir = glm::normalize(cameraRotation * glm::vec3(0, 0, 1));
	glm::vec3 cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraDir));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDir, cameraRight));

	shapes[shapeId]->MyTranslate(8.f*(cameraRight*xrel + cameraUp*yrel), 0);
}

void Game::translateShape(float xpos, float ypos)
{
	int pickedShape = getPickedShape();
	int segNum = ((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum();
	if (pickedShape >= FRST_OCTA_INDX && pickedShape < (segNum * 3 + 1 + FRST_OCTA_INDX))
	{
		// save previous pickedshape position 
		glm::vec4 pickedPrevPos = ((Bezier1D*)shapes[bezierIndx])->GetControlPoint(GET_SEG_INDX(pickedShape), GET_CP_INDX(pickedShape));

		// multiplied by 2 because of relative movement of the mouse on window that divided to 2 viewports
		// translates the shape we hold
		shapes[pickedShape]->MyTranslate(glm::vec3(2 * xrel, 2 * yrel, 0), 0);
		((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pickedShape, 2 * xrel, 2 * yrel, false);


		// if moving connection point, move also adjusted control points
		if (GET_CP_INDX(pickedShape) == 0)
		{
			if (GET_SEG_INDX(pickedShape) == 0)
			{
				// seg-0 cp-0
				shapes[pickedShape + 1]->MyTranslate(glm::vec3(2 * xrel, 2 * yrel, 0), 0);
				((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pickedShape + 1, 2 * xrel, 2 * yrel, false);
			}
			else if (GET_SEG_INDX(pickedShape) == ((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum())
			{
				// seg-last cp-3
				shapes[pickedShape - 1]->MyTranslate(glm::vec3(2 * xrel, 2 * yrel, 0), 0);
				((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pickedShape - 1, 2 * xrel, 2 * yrel, false);
			}
			else
			{
				shapes[pickedShape - 1]->MyTranslate(glm::vec3(2 * xrel, 2 * yrel, 0), 0);
				((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pickedShape - 1, 2 * xrel, 2 * yrel, false);
				shapes[pickedShape + 1]->MyTranslate(glm::vec3(2 * xrel, 2 * yrel, 0), 0);
				((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pickedShape + 1, 2 * xrel, 2 * yrel, false);
			}
		}

		// if rotating p1/p2 points
		if (GET_CP_INDX(pickedShape) != 0)
		{
			// determine which connection point to rotate around
			unsigned int midShape;
			if (GET_CP_INDX(pickedShape) == 1)
				midShape = pickedShape - 1;
			else
				midShape = pickedShape + 1;

			glm::vec4 newPos = ((Bezier1D*)shapes[bezierIndx])->GetControlPoint(GET_SEG_INDX(pickedShape), GET_CP_INDX(pickedShape));
			glm::vec4 mid = ((Bezier1D*)shapes[bezierIndx])->GetControlPoint(GET_SEG_INDX(midShape), GET_CP_INDX(midShape));
			// calculate the angle diff based on 2xrel 2yrel
			float angleDiff = glm::acos(glm::dot(glm::normalize(pickedPrevPos - mid), glm::normalize(newPos - mid)));
			glm::vec4 oldOffseted = glm::normalize(pickedPrevPos - mid);
			glm::vec4 newOffseted = glm::normalize(newPos - mid);
			bool isClockwise = (newOffseted.y > 0 && newOffseted.x < oldOffseted.x) || (newOffseted.y < 0 && newOffseted.x > oldOffseted.x);
			if (!isClockwise)
				angleDiff = -angleDiff;

			// rotate picked shape around itself based on angle diff above
			shapes[pickedShape]->MyTranslate(glm::vec3(-newPos.x, -newPos.y, 0), 0);
			ShapeTransformation(zRotate, angleDiff);
			shapes[pickedShape]->MyTranslate(glm::vec3(newPos.x, newPos.y, 0), 0);

			// if continuity state, rotate also the compatible control point on the adjusted segment
			if (C_state)
			{
				// determine which point to move
				unsigned int pointToMove;
				if (GET_SEG_INDX(pickedShape) > 0 && GET_CP_INDX(pickedShape) == 1)
					pointToMove = pickedShape - 2;
				else if (GET_SEG_INDX(pickedShape) < segNum - 1 && GET_CP_INDX(pickedShape) == 2)
					pointToMove = pickedShape + 2;
				else
					return;


				shapes[pointToMove]->MyTranslate(glm::vec3(-mid.x, -mid.y, 0), 0);
				int temp = this->pickedShape;
				this->pickedShape = pointToMove;
				ShapeTransformation(zRotate, angleDiff);
				this->pickedShape = temp;
				shapes[pointToMove]->MyTranslate(glm::vec3(mid.x, mid.y, 0), 0);

				glm::vec4 pToMove = ((Bezier1D*)shapes[bezierIndx])->GetControlPoint(GET_SEG_INDX(pointToMove), GET_CP_INDX(pointToMove));
				glm::vec4 finalPos = shapes[pointToMove]->MakeTrans() * glm::vec4(1, 1, 1, 1);
				((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pointToMove, finalPos.x - pToMove.x, finalPos.y - pToMove.y, false);
			}
		}
	}
	else 
	{
		int pick = getPickedConvexHull(xpos, ypos);
		if (pick != -1) {
			translateSegment(pick, ((Bezier1D*)shapes[bezierIndx])->GetSegment(pick)); // move all points in the relative xrel yrel
		}
	}
}

int Game::getPickedConvexHull(float x, float y)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	if (x >= viewport[2]) {
		x = x - viewport[2];
		x = x / viewport[2];
		y = 1 - y / viewport[3];
		glm::vec2 newXY = glm::mix(glm::vec2(-1, -1), glm::vec2(1, 1), glm::vec2(x, y));

		std::cout << "x: " << x << std::endl;
		std::cout << "y: " << y << std::endl;
		if (pickedShape <= bezierIndx) {
			std::cout << "getPickedConvexHull\n";
			glm::mat4 segmentCopy;

			for (int i = 0; i < ((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum(); i++)
			{
				glm::mat4 segment = ((Bezier1D*)shapes[bezierIndx])->GetSegment(i);
				std::vector<std::pair<int, glm::vec4>> pointsByX = { std::pair<int, glm::vec4>(0,segment[0]),std::pair<int, glm::vec4>(1,segment[1]),
					std::pair<int, glm::vec4>(2,segment[2]),std::pair<int, glm::vec4>(3,segment[3]) };
				std::sort(pointsByX.begin(), pointsByX.end(), [](std::pair<int, glm::vec4> const& first, std::pair<int, glm::vec4>& second) { return first.second.x < second.second.x;});

				std::stack<int> conS = sortByAngle(segment);

				if (conS.size() < 3)
				{
					if (compareFloats(pointsByX[0].second.x, pointsByX[1].second.x)) {
						std::sort(pointsByX.begin(), pointsByX.end(), [](std::pair<int, glm::vec4> const& first, std::pair<int, glm::vec4>& second) {return  first.second.y < second.second.y;});
						if (glm::abs(pointsByX[3].second.x - newXY.x) < 0.2 && newXY.y >= pointsByX[0].second.y && newXY.y <= pointsByX[3].second.y)
							return i;
					}
					else if (compareFloats(pointsByX[0].second.y, pointsByX[1].second.y)) {
						if ((glm::abs(pointsByX[3].second.y - newXY.y) < 0.2) && newXY.x >= pointsByX[0].second.x && newXY.x <= pointsByX[3].second.x)
							return i;
					}
					else
					{
						float m = (pointsByX[2].second.y - pointsByX[1].second.y) / (pointsByX[2].second.x - pointsByX[1].second.x);
						float mOrt = (-1) * (1 / m);
						float tng = atan(mOrt);
						float dx = glm::abs(0.2 * sin(tng));
						float dy = glm::abs(0.2 * cos(tng));
						glm::vec4 imP1 = glm::vec4(pointsByX[0].second.x - dx, pointsByX[0].second.y + dy, 0, 0);
						glm::vec4 imP2 = glm::vec4(pointsByX[0].second.x + dx, pointsByX[0].second.y - dy, 0, 0);
						glm::vec4 imP3 = glm::vec4(pointsByX[3].second.x - dx, pointsByX[3].second.y + dy, 0, 0);
						glm::vec4 imP4 = glm::vec4(pointsByX[3].second.x + dx, pointsByX[3].second.y - dy, 0, 0);
						glm::mat4 imSeg = glm::mat4(imP1, imP2, imP3, imP4);
						std::stack<int> conImS = sortByAngle(imSeg);
						if (pointInSegment(conImS, imSeg, newXY.x, newXY.y))
							return i;
					}
				}
				//now that we have the convex hull need to check if point in it
				else if (pointInSegment(conS, segment, newXY.x, newXY.y)) {
					return i;
				}

			}
		}
	}

	return -1;
}

std::stack<int> Game::sortByAngle(glm::mat4 segment)
{
	std::stack<int> conS;
	std::vector<std::pair<int, float>> pointByAngle;
	std::pair<int, glm::vec4> p0 = getConvexHullP0(segment); // the lowest left-est point of the segment // int is for indx of p0 in the segment so that we won't compare p0 to his own angle
															 //push all counterclockwise angles of other points compared to p0

	for (int j = 0; j < 4; j++) {
		if (p0.first != j) pointByAngle.push_back(std::pair<int, float>(j, getAngle(p0.second, segment[j])));
	}

	//sorting the points's angle's
	std::sort(pointByAngle.begin(), pointByAngle.end(), [](std::pair<int, float> const& first, std::pair<int, float>& second) { return first.second < second.second;});

	//pointByAngle.push_back(std::pair<int, float>(p0.first, 0));
	// 5) Create an empty stack ‘S’ and push points[0], points[1] and points[2] to S.
	conS.push(p0.first);
	conS.push(pointByAngle[0].first);
	int j = 1;
	while (j < 3)
	{
		int secToLowest = conS.top(); //3
		glm::vec4 p1 = segment[secToLowest];
		conS.pop();
		int lowest = conS.top(); //0
		glm::vec4 pp0 = segment[lowest];
		conS.pop();
		glm::vec4 p2 = segment[pointByAngle[j].first];
		glm::vec4 vec1 = (p1 - pp0);
		glm::vec4 vec2 = (p2 - p1);
		float dot = vec1.x * vec2.x + vec1.y * vec2.y;
		float det = vec1.x * vec2.y - vec1.y * vec2.x;
		float angle = atan2(det, dot);
		conS.push(lowest);
		conS.push(secToLowest);
		if (angle >= PI - 0.01 || (angle >= 0 && angle <= 0.01)) {
			conS.pop();
			p1.x > p2.x ? conS.push(secToLowest) : conS.push(pointByAngle[j].first);
			j++;
		}
		else if (angle > 0.01) {
			conS.push(pointByAngle[j].first);
			j++;
		}
		else if (conS.size() > 2)
			conS.pop();
		else
			j++;
	}
	return conS;
}

//int Game::getPickedConvexHull(float x, float y)
//{
//	int viewport[4];
//	glGetIntegerv(GL_VIEWPORT, viewport);
//	if (x >= viewport[2])
//	{
//		x = x - viewport[2];
//		x = x / viewport[2];
//		y = 1 - y / viewport[3];
//		glm::vec2 newXY = glm::mix(glm::vec2(-1, -1), glm::vec2(1, 1), glm::vec2(x, y));
//
//		std::cout << "x: " << x << std::endl;
//		std::cout << "y: " << y << std::endl;
//		if (pickedShape <= bezierIndx) {
//			std::cout << "getPickedConvexHull\n";
//			glm::mat4 segmentCopy;
//			for (int i = 0; i < ((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum(); i++)
//			{
//				std::vector<std::pair<int, float>> pointByAngle;
//				std::stack<int> conS;
//				segmentCopy = ((Bezier1D*)shapes[bezierIndx])->GetSegment(i); // the i segment
//				std::pair<int, glm::vec4> p0 = getConvexHullP0(i); // the lowest left-est point of the segment // int is for indx of p0 in the segment so that we won't compare p0 to his own angle
//																   //push all counterclockwise angles of other points compared to p0
//				for (int j = 0; j < 4; j++) {
//					if (p0.first != j) pointByAngle.push_back(std::pair<int, float>(j, getAngle(p0.second, segmentCopy[j])));
//				}
//				////////////////////////////////////////////////
//
//				// if exists two equal angles then convex hull does'nt exist.
//				//needs to be checked;
//
//				//sorting the points's angle's
//				std::sort(pointByAngle.begin(), pointByAngle.end(), [](std::pair<int, float> const& first, std::pair<int, float>& second) { return first.second < second.second;});
//				//pointByAngle.push_back(std::pair<int, float>(p0.first, 0));
//				// 5) Create an empty stack ‘S’ and push points[0], points[1] and points[2] to S.
//				conS.push(p0.first);
//				conS.push(pointByAngle[0].first);
//				int j = 1;
//				while(j<3)
//				{
//					int secToLowest = conS.top(); //3
//					glm::vec4 p1 = segmentCopy[secToLowest];
//					conS.pop();
//					int lowest = conS.top(); //0
//					glm::vec4 pp0 = segmentCopy[lowest];
//					conS.pop();
//					glm::vec4 p2 = segmentCopy[pointByAngle[j].first];
//					glm::vec4 vec1 = (p1 - pp0);
//					glm::vec4 vec2 = (p2 - p1);
//					float dot = vec1.x * vec2.x + vec1.y * vec2.y;
//					float det = vec1.x * vec2.y - vec1.y * vec2.x;
//					float angle = atan2(det, dot);
//					conS.push(lowest);
//					conS.push(secToLowest);
//					if (angle > 0) {
//						conS.push(pointByAngle[j].first);
//						j++;
//					}
//					else if (conS.size() > 2)
//						conS.pop();
//					else
//						j++;
//				}
//				if (conS.size() < 3)
//				{
//					//needs to be considered
//				}
//				//now that we have the convex hull need to check if point in it
//				if (pointInSegment(conS, segmentCopy, newXY.x, newXY.y)) {
//					return i;
//				}
//			}
//		}
//	}
//	return -1;
//}

void Game::translateSegment(int i, glm::mat4 segment) {
	//we should only move the convex hull if points On Line do not exceed the bounds - but it doesn't work
	float maxX = std::max(segment[0].x, segment[3].x);
	float minX = std::min(segment[0].x, segment[3].x);
	float maxY = std::max(segment[0].y, segment[3].y);
	float minY = std::min(segment[0].y, segment[3].y);
	float newXrel = glm::clamp(xrel, -1.0f - minX, 1.0f - maxX);
	float newYrel = glm::clamp(yrel, - minY, 1.0f - maxY);
	//now that we established qualified xrel and yrel time to move all shapes
	for (int j = 0; j < 4; j++) {
		movePickedConvexHull(i*3+(j+bezierIndx+1), newXrel, newYrel);
	}
	if (C_state) {
		if (i != (((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum() - 1)) {
			movePickedConvexHull((i + 1) * 3 + (1 + bezierIndx + 1), newXrel, newYrel);
		}
		if (i != 0)
			movePickedConvexHull((i - 1) * 3 + (2 + bezierIndx + 1), newXrel, newYrel);
	}
}

void Game::movePickedConvexHull(int pickedshape, float newXrel, float newYrel) {
	shapes[pickedshape]->MyTranslate(glm::vec3(2 * newXrel, 2 * newYrel, 0), 0);
	((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pickedshape, 2 * newXrel, 2 * newYrel, false);
}

std::pair<int,glm::vec4> Game::getConvexHullP0(glm::mat4 segmentCopy) {
	std::vector<float> minXcor;
	std::vector<float> yCoordinates = { segmentCopy[0].y, segmentCopy[1].y,segmentCopy[2].y, segmentCopy[3].y };
	std::sort(yCoordinates.begin(), yCoordinates.end());
	for (int j = 0; j < 4;j++)
	{
		if (compareFloats(segmentCopy[j].y, yCoordinates[0]))
		{
			minXcor.push_back(segmentCopy[j].x);
			break;
		}
	}
	std::sort(minXcor.begin(), minXcor.end());
	for (int i = 0; i < 4; i++) {
		if (compareFloats(segmentCopy[i].y, yCoordinates[0]) && compareFloats(segmentCopy[i].x, minXcor[0]))
			return std::pair<int, glm::vec4>(i,segmentCopy[i]);
	}
}

bool Game::pointInSegment(std::stack<int> St, glm::mat4 segment, float x, float y) {
	// needs to check if the dot is to "the right" of the line clock-wise order;
	glm::vec4 p = segment[St.top()];
	St.pop();
	glm::vec4 temp = p;
	glm::vec4 q;
	glm::vec4 r = glm::vec4(x, y,0,0);
	while (!St.empty()) {
		q = segment[St.top()];
		St.pop();
		glm::vec4 vec1 = (q - p);
		glm::vec4 vec2 = (r - p);	
		float dot = vec1.x * vec2.x + vec1.y * vec2.y;
		float det = vec1.x * vec2.y - vec1.y * vec2.x;
		float angle = atan2(det, dot);
		if (angle >= 0) return false;
		p = q;
	}
	q = temp;
	glm::vec4 vec1 = glm::normalize(q - p);
	glm::vec4 vec2 = glm::normalize(r - p);
	float dot = vec1.x * vec2.x + vec1.y * vec2.y;
	float det = vec1.x * vec2.y - vec1.y * vec2.x;
	float angle = atan2(det, dot);
	if (angle >= 0) return false;
	return true;
}

int Game::compareFloats(float a, float b) {
	if (glm::abs(a - b) < 0.01)
		return 1;
	return 0;
}

float Game::getAngle(glm::vec4 first, glm::vec4 second) {
	//calculate angle between first and second; 
	glm::vec4 temp_second = second - first;
	float secondToX = glm::acos(glm::dot(glm::normalize(temp_second), glm::vec4(1, 0, 0, 0)));
	return secondToX;
}

void Game::pOnLine()
{
	// straights the left point to be on the same line as the mid and right point
	int pickedOcta = pickedShape - FRST_OCTA_INDX;
	if (pickedOcta % 3 == 0 && pickedOcta > 0 && pickedOcta / 3 < ((Bezier1D*)shapes[bezierIndx])->GetSegmentsNum())
	{
		glm::vec4 p1 = ((Bezier1D*)shapes[bezierIndx])->GetControlPoint((pickedOcta+1) / 3, (pickedOcta+1) % 3);
		glm::vec4 p0 = ((Bezier1D*)shapes[bezierIndx])->GetControlPoint(pickedOcta / 3, pickedOcta % 3);
		pickedOcta--;
		glm::vec4 pp2 = ((Bezier1D*)shapes[bezierIndx])->GetControlPoint(pickedOcta / 3, pickedOcta % 3);
		if (abs(abs(glm::dot(p0, pp2)) - 1) < EPSILON) return;
		float d = glm::distance(p0, pp2);
		glm::vec4 dir = glm::normalize(p0 - p1);
		glm::vec4 pTemp = p0 + dir * d;
		shapes[pickedShape-1]->MyTranslate(glm::vec3(pTemp.x-pp2.x, pTemp.y-pp2.y, 0), 0);
		((Bezier1D*)shapes[bezierIndx])->CurveUpdate(pickedShape-1, pTemp.x-pp2.x, pTemp.y-pp2.y, false);
	}
}

void Game::Update(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& MVP, const glm::mat4& Model, const int  shaderIndx)
{
 	Shader *s = shaders[shaderIndx];
	int r = ((pickedShape+1) & 0x000000FF) >>  0;
	int g = ((pickedShape+1) & 0x0000FF00) >>  8;
	int b = ((pickedShape+1) & 0x00FF0000) >> 16;
	if (shapes[pickedShape]->GetMaterial() >= 0 && !materials.empty())
		BindMaterial(s, shapes[pickedShape]->GetMaterial());
	//textures[0]->Bind(0);
	s->Bind(); // glUse for drawing with the current shader
	if (shaderIndx != 2)
	{
		s->SetUniformMat4f("MVP", MVP);
		s->SetUniformMat4f("Normal", Model);
	}
	else if (shaderIndx == 2) // cubemap shader
	{
		s->SetUniform1i("sampler1", materials[shapes[pickedShape]->GetMaterial()]->GetSlot(0));
		s->SetUniformMat4f("Proj", proj);
		s->SetUniformMat4f("View", view);
		s->SetUniformMat4f("Model", Model);
	}
	else
	{
		s->SetUniformMat4f("MVP", glm::mat4(1));
		s->SetUniformMat4f("Normal", glm::mat4(1));
	}
	if(shaderIndx!=2)
		s->SetUniform1i("sampler", materials[shapes[pickedShape]->GetMaterial()]->GetSlot(1));

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
