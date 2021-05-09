#pragma once
//#include "Mesh.h"
#include "shape.h"

class Bezier1D : public Shape
{
	std::vector<glm::mat4> segments;
	unsigned int resT;
	glm::mat4 M;
	void MoveControlPoint(int segment, int indx, float dx, float dy, bool preserveC1);  //change the position of one control point. when preserveC1 is true it may affect other  control points 
public:
	Bezier1D(int segNum, int res,int mode, int viewport = 0);
	IndexedModel GetLine();	//generates a model for MeshConstructor Constructor
	glm::vec4 GetControlPoint(int segment, int indx) const; //returns a control point in the requested segment. indx will be 0,1,2,3, for p0,p1,p2,p3
	
	glm::vec4 GetPointOnCurve(int segment, int t); //returns point on curve in the requested segment for the value of t
	glm::vec3 GetVelosity(int segment, int t); //returns the derivative of the curve in the requested segment for the value of t
	void SplitSegment(int segment, int t);  // split a segment into two parts
	void AddSegment(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3);  // add a segment at the end of the curve
	glm::mat4 GetSegment(int segment);
	void AddSegmentInPlace(glm::mat4 segment1, glm::mat4 segment2, int place);
	void removeSegment(int pick);
	void CurveUpdate(int pointIndx, float dx, float dy, bool preserveC1 = false);  //change the line in by using ChangeLine function of MeshConstructor and MoveControlPoint 
	
	inline int GetSegmentsNum() const { return segments.size(); }
	~Bezier1D(void);
};

