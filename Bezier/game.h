#pragma once
#include "scene.h"
#include <queue>
#include <stack>


class Game : public Scene
{
public:
	Game();
	~Game(void);
	void Init();
	void deleteBezier();
	void Update(const glm::mat4& view, const glm::mat4& proj, const glm::mat4 &MVP,const glm::mat4 &Model,const int  shaderIndx);
	
	void WhenRotate();
	void WhenTranslate();
	void Motion();
	
	unsigned int TextureDesine(size_t width, size_t height);
	void UpdatePosition(float x, float y);
	int getPickedShape() const { return pickedShape; }
	std::vector<int> getPickedShapes() const{ return pickedShapes; }
	void setPickedShape(int p) { pickedShape = p; }


	void TranslateShapeInCameraPlane(int shapeId, glm::mat4 cameraTransform);
	void translateShape(float xpos, float ypos);
	std::stack<int> sortByAngle(glm::mat4 segment);
	void translateSegment(int i, glm::mat4 segment);
	void movePickedConvexHull(int pickedshape, float newXrel, float newYrel);
	std::pair<int, glm::vec4> getConvexHullP0(glm::mat4 segmentCopy);
	bool pointInSegment(std::stack<int> St, glm::mat4 segment, float x, float y);
	int compareFloats(float a, float b);
	float getAngle(glm::vec4 first, glm::vec4 second);
	void resetRelativePos() { xold = yold = -1; }
	void pOnLine(); // Ex5b - places 3 adjacents points on the same line
	void initOcta(unsigned int const segNum);
	void toggleC_state() { C_state = !C_state; }
	void create2DBezier();
	void ShapeTransformation(int type, float amount);
	void splitBezier(float xpos, float ypos);
	glm::vec3 GetShapePosition(int shapeIndx) { return glm::vec3(shapes[shapeIndx]->MakeTrans()[3]); }
	int GetLastOctaIndex() { return lastOctaIndx; }
	void RotateShapeInplace(int shapeIndx);
private: 
	int getPickedConvexHull(float x, float y);

	float xold=-1, yold=-1, xrel, yrel;
	bool C_state = false;
	int bezierIndx;
	int lastOctaIndx;
};

