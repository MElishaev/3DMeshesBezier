#pragma once
#include "scene.h"
#include <queue>


class Game : public Scene
{
public:
	Game();
	~Game(void);
	void Init();
	void Update(const glm::mat4 &MVP,const glm::mat4 &Model,const int  shaderIndx);
	
	void WhenRotate();
	void WhenTranslate();
	void Motion();
	
	unsigned int TextureDesine(size_t width, size_t height);
	void UpdatePosition(float x, float y);
	int getPickedShape() const { return pickedShape; }

	void translateShape(const unsigned int pickedShape);
	void resetRelativePos() { xold = yold = -1; }
private:
	float xold=-1, yold=-1, xrel, yrel;
};

