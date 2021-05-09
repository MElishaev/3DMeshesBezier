#include "Bezier1D.h"
#include <iostream>

void Bezier1D::MoveControlPoint(int segment, int indx, float dx, float dy, bool preserveC1)
{
	if (segment >= GetSegmentsNum())
	{
		indx = 3;
		segment--;
	}

	glm::mat4 seg = glm::transpose(segments[segment]);
	glm::vec4 vec2change = seg[indx];
	vec2change.x += dx;
	vec2change.y += dy;
	seg[indx] = vec2change;
	segments[segment] = glm::transpose(seg);

	if (indx == 0 && segment > 0)
	{
		segment--;
		indx = 3;
	}
	else if (indx == 3 && segment < GetSegmentsNum() - 1)
	{
		segment++;
		indx = 0;
	}
	else return;

	seg = glm::transpose(segments[segment]);
	seg[indx] = vec2change;
	segments[segment] = glm::transpose(seg);
}

Bezier1D::Bezier1D(int segNum, int res, int mode, int viewport) : Shape(mode)
{
	// 4 used here because we assume x-axis [-2,2]
	float initialXrange = 0.5;
	float initialYrange = 0.5;
	float segs_dx = (2*initialXrange) / (3*segNum);
	glm::mat4 seg0 = glm::mat4(-initialXrange, 0, 0, 0,
							   -initialXrange, initialYrange/2, 0, 0,
							   -initialXrange + 1.5 * segs_dx, initialYrange, 0, 0,
							   -initialXrange + 3 * segs_dx, initialYrange, 0, 0);
	glm::mat4 segLast = glm::mat4(initialXrange - 3 * segs_dx, initialYrange, 0, 0,
								  initialXrange - 1.5 * segs_dx, initialYrange, 0, 0,
								  initialXrange, initialYrange/2, 0, 0,
								  initialXrange, 0, 0, 0);

	segments.push_back(glm::transpose(seg0));
	for (int i = 1; i < segNum-1; i++)
	{
		glm::vec4 prevSegPoint = glm::transpose(segments.back())[3];
		glm::mat4 newSeg;
		newSeg[0] = prevSegPoint;
		newSeg[1] = glm::vec4(prevSegPoint[0] + segs_dx, initialYrange, 0, 0);
		newSeg[2] = glm::vec4(prevSegPoint[0] + 2*segs_dx, initialYrange, 0, 0);
		newSeg[3] = glm::vec4(prevSegPoint[0] + 3*segs_dx, initialYrange, 0, 0);
		segments.push_back(glm::transpose(newSeg));
	}
	segments.push_back(glm::transpose(segLast));

	resT = res;
	AddViewport(viewport);
	mesh = new MeshConstructor(GetLine(), false);
}

IndexedModel Bezier1D::GetLine()
{
	IndexedModel model;
	std::vector<LineVertex> segEdges;
	std::vector<unsigned int> segIndices;
	float t;
	for (unsigned int i = 0; i < GetSegmentsNum(); i++)
		for (unsigned int j = 0; j < resT; j++)
		{
			t = float(j) / resT;
			glm::vec4 TM = glm::vec4(pow(1 - t, 3), 3 * t*pow(1 - t, 2), 3 * pow(t, 2)*(1 - t), pow(t, 3));
			glm::vec4 TMG = TM*segments[i];
			segEdges.push_back(LineVertex(glm::vec3(TMG.x, TMG.y, TMG.z), glm::vec3(1,0,0)));
			segIndices.push_back(i*resT+j);
		}
	glm::vec4 lastPoint = glm::transpose(segments[GetSegmentsNum() - 1])[3];
	segEdges.push_back(LineVertex(glm::vec3(lastPoint.x, lastPoint.y, lastPoint.z), glm::vec3(1,0,0)));
	segIndices.push_back(GetSegmentsNum()*resT);

	for(unsigned int i = 0; i <= resT*GetSegmentsNum(); i++)
	{
		model.positions.push_back(*segEdges[i].GetPos());
		model.colors.push_back(*segEdges[i].GetColor());
		model.indices.push_back(segIndices[i]);
	}

	return model;
}

glm::vec4 Bezier1D::GetControlPoint(int segment, int indx) const
{
	if (segment == GetSegmentsNum() && indx == 0)
	{
		segment--;
		indx = 3;
	}
	if (segment < GetSegmentsNum())
		return glm::transpose(segments[segment])[indx];
	else
		std::cout << "ERROR:: Segment given is out of range.\n";
	return glm::vec4(-1, -1, -1, -1);
}

glm::vec4 Bezier1D::GetPointOnCurve(int segment, int t)
{

	return glm::vec4();
}

void Bezier1D::AddSegment(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3)
{
	if (segments.empty())
	{
		segments.push_back(glm::mat4(0, p1.x, p2.x, p3.x,
									 0, p1.y, p2.y, p3.y,
									 0, p1.z, p2.z, p3.z,
									 0,    0,    0,    0));
	}
	else 
	{
		segments.push_back(glm::mat4(segments[GetSegmentsNum()-1][3].x, p1.x, p2.x, p3.x,
									 segments[GetSegmentsNum()-1][3].y, p1.y, p2.y, p3.y,
									 segments[GetSegmentsNum()-1][3].z, p1.z, p2.z, p3.z,
									 0,    0,    0,    0));
	}
}

glm::mat4 Bezier1D::GetSegment(int segment) {
	if (segment < GetSegmentsNum())
		return glm::transpose(segments[segment]);
	else
		std::cout << "ERROR:: Segment given is out of range.\n";
	return glm::mat4();
}

void Bezier1D::AddSegmentInPlace(glm::mat4 segment1, glm::mat4 segment2, int place) {
	segments.insert(segments.begin() + place, glm::transpose(segment1));
	segments.insert(segments.begin() + (place+1), glm::transpose(segment2));
	delete(mesh);
	mesh = new MeshConstructor(GetLine(), false);
}

void Bezier1D::removeSegment(int pick)
{
	segments.erase(segments.begin()+pick);
}

void Bezier1D::CurveUpdate(int pointIndx, float dx, float dy, bool preserveC1)
{
	MoveControlPoint((pointIndx-3)/3, (pointIndx-3)%3, dx, dy, preserveC1);
	IndexedModel model = GetLine();
	mesh->ChangeLine(model);
}

Bezier1D::~Bezier1D(void)
{
}
