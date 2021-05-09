#pragma once
#include "Bezier1D.h"

class Bezier2D : public Shape
{
	int circularSubdivision; //usualy 4 how many subdivision in circular direction ????
	std::vector <glm::vec4> controlPoints;
	int resT; 
	int resS;
	int subNum; // ???
	int bezier1DsegNum;
	
	int Ssegments = 4; // each bezier on s-axis consist of 2 segments (for -z and +z)

	//void AddToModel(IndexedModel& model, float s, float t, const std::vector<glm::vec4> subSurf ,int subIndx);  // add a new Nurb to the model
	glm::vec4 CalcNurbs(float s, float t, const std::vector<glm::vec4> subSurf); // ????
	glm::vec3 CalcNormal(float s, float t, const std::vector<glm::vec4> subSurf);
	void AddToModel(IndexedModel & model, int S, int T, int s, int t, int quadId);
	void CalcControlPoints(const Bezier1D* c);  // calculates control points cubic Bezier manifold.
	glm::vec3 getPointPosition(int segmentS, int segmentT, int s, int t);

public:
	Bezier2D(void);
	Bezier2D(const Bezier1D* c, int circularSubdivision,int _resS,int _resT,int mode, int viewport = 0);
	IndexedModel GetSurface();	//generates a model for MeshConstructor Constructor
	Vertex* GetPointOnSurface(int segmentS, int segmentT, int s, int t);  //returns a point on the surface in the requested segment for value of t and s
	glm::vec3 GetNormal(int segmentS, int segmentT, int s, int t); //returns a normal of a point on the surface in the requested segment for value of t and s

	~Bezier2D(void);
};