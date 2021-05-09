#include "Bezier2D.h"

int nChoosek(int n, int k)
{
	if (k > n) return 0;
	if (k * 2 > n) k = n-k;
	if (k == 0) return 1;

	int result = n;
	for( int i = 2; i <= k; ++i ) {
		result *= (n-i+1);
		result /= i;
	}
	return result;
}

int eq(glm::vec3 a, glm::vec3 b)
{
	return (abs(a.x - b.x) < 0.001 && abs(a.y - b.y) < 0.001 && abs(a.z - b.z) < 0.001);
}

glm::vec3 evalBezier(const std::vector<glm::vec4> points, const float s)
{
	int n = 3;
	glm::vec4 pos = glm::vec4(0, 0, 0, 1);
	for(int j=0; j<n; j++)
	{
		float Bi_s = float(nChoosek(n, j)) * pow(1 - s, n - j) * pow(s, j);
		pos += Bi_s * points[j];
	}
	return glm::vec3(pos);
}

void Bezier2D::AddToModel(IndexedModel & model, int S, int T, int s, int t, int quadId)
{
	Vertex* v = GetPointOnSurface(S, T, s, t);
	model.positions.push_back(*v->GetPos());
	model.colors.push_back(*v->GetColor());
	model.normals.push_back(*v->GetNormal());
	model.texCoords.push_back(*v->GetTexCoord());
	model.indices.push_back(quadId++);

	delete(v);
	v = nullptr;

	v = GetPointOnSurface(S, T, s+1, t);
	model.positions.push_back(*v->GetPos());
	model.colors.push_back(*v->GetColor());
	model.normals.push_back(*v->GetNormal());
	model.texCoords.push_back(*v->GetTexCoord());
	model.indices.push_back(quadId++);

	delete(v);
	v = nullptr;

	v = GetPointOnSurface(S, T, s+1, t+1);
	model.positions.push_back(*v->GetPos());
	model.colors.push_back(*v->GetColor());
	model.normals.push_back(*v->GetNormal());
	model.texCoords.push_back(*v->GetTexCoord());
	model.indices.push_back(quadId++);

	delete(v);
	v = nullptr;

	v = GetPointOnSurface(S, T, s, t+1);
	model.positions.push_back(*v->GetPos());
	model.colors.push_back(*v->GetColor());
	model.normals.push_back(*v->GetNormal());
	model.texCoords.push_back(*v->GetTexCoord());
	model.indices.push_back(quadId++);

	delete(v);
	v = nullptr;
}

void Bezier2D::CalcControlPoints(const Bezier1D * c)
{
	for (int seg = 0; seg < bezier1DsegNum; seg++)
	{
		for (int cp = 0; cp < 4; cp++)
		{
			if (seg != bezier1DsegNum-1 && cp == 3) continue; // skip last cp in each seg except the last
			glm::vec4 cPoint = c->GetControlPoint(seg, cp); //point #3 in S-segment

			// circular bezier
			// starting from the 1D control point and adding 11 control points
			// in a circular manner clockwise in ZY plane
			controlPoints.push_back(cPoint);
			controlPoints.push_back(glm::vec4(cPoint.x, cPoint.y,0.5*cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, 0.5*cPoint.y,cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, 0,cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, -0.5*cPoint.y,cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, -cPoint.y,0.5*cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, -cPoint.y,0,1));
			controlPoints.push_back(glm::vec4(cPoint.x, -cPoint.y,-0.5*cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, -0.5*cPoint.y,-cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, 0,-cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, 0.5*cPoint.y,-cPoint.y,1));
			controlPoints.push_back(glm::vec4(cPoint.x, cPoint.y,-0.5*cPoint.y,1));
		}
	}
}

Bezier2D::Bezier2D(const Bezier1D * c, int circularSubdivision, int _resS, int _resT, int mode, int viewport) : Shape(mode)
{
	this->circularSubdivision = circularSubdivision;
	resS = _resS;
	resT = _resT;
	bezier1DsegNum = c->GetSegmentsNum();
	CalcControlPoints(c);
	AddViewport(viewport);

	mesh = new MeshConstructor(GetSurface(), true);
}


IndexedModel Bezier2D::GetSurface()
{
	IndexedModel model;

	int i = 0;
	for(int S = 0; S < bezier1DsegNum; S++)
		for (int T = 0; T < circularSubdivision; T++)
			for(int s = 0; s < resS; s++)
				for (int t = 0; t < resT; t++)
					AddToModel(model, S, T, s, t, 4*i++); // adds quad. given uniquely by (S,T,s,t)

	return model;
}

glm::vec3 Bezier2D::getPointPosition(int segmentS, int segmentT, int s, int t)
{
	float _s = float(s) / resS;
	float _t = float(t) / resT;

	int segS = segmentS * 12 * 3; // the first control point in S
	int segT = segmentT * 3;
	std::vector<glm::vec4> Pij;

	// get the 16 control points based on given S and T
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			Pij.push_back(controlPoints[segS + i * 12 + (segT + j)%12]);

	// calculate the point on the surface
	glm::vec4 pos = glm::vec4(0, 0, 0, 1);
	int n = 3;
	for (int i = 0; i <= n; i++)
	{
		float Bi_s = float(nChoosek(n, i)) * pow(1 - _s, n - i) * pow(_s, i);
		for (int j = 0; j <= n; j++)
		{
			float Bj_t = float(nChoosek(n, j)) * pow(1 - _t, n - j) * pow(_t, j);
			pos += Bi_s * Bj_t * Pij[i*4+j];
		}
	}

	return glm::vec3(pos);
}

Vertex * Bezier2D::GetPointOnSurface(int segmentS, int segmentT, int s, int t)
{
	// 0 <= s,t <= resS,resT
	glm::vec3 pos = getPointPosition(segmentS, segmentT, s, t);
	glm::vec3 normal = GetNormal(segmentS, segmentT, s, t);
	glm::vec2 texCoords = glm::vec2(float(s) / resS, 
									float(t) / resT); // maps on each (S,T) subsurf the texture
	Vertex* v = new Vertex(glm::vec3(pos),texCoords, normal, glm::vec3(1,0,0));

	return v;
}

glm::vec3 Bezier2D::GetNormal(int segmentS, int segmentT, int s, int t)
{
	float _s = float(s) / resS;
	float _t = float(t) / resT;

	int segS = segmentS * 12 * 3; // the first control point in S
	int segT = segmentT * 3;
	std::vector<glm::vec4> Pij;

	// get the 16 control points based on given S and T
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			Pij.push_back(controlPoints[segS + i * 12 + (segT + j)%12]);

	// calculate d/ds
	glm::vec3 d_ds = 
		-3 * pow(1 - _s, 2)*evalBezier({ Pij[0],Pij[1],Pij[2],Pij[3] }, _t) +
		(3 * pow(1 - _s, 2) - 6 * _s*(1 - _s))*evalBezier({ Pij[4],Pij[5],Pij[6],Pij[7] }, _t) +
		(6 * _s*(1 - _s) - 3 * pow(_s,2))*evalBezier({ Pij[8],Pij[9],Pij[10],Pij[11] }, _t) +
		3 * pow(_s,2)*evalBezier({ Pij[12],Pij[13],Pij[14],Pij[15] }, _t);

	// calculate d/dt
	glm::vec3 d_dt = 
		-3 * pow(1 - _t, 2)*evalBezier({ Pij[0],Pij[4],Pij[8],Pij[12] }, _s) +
		(3 * pow(1 - _t, 2) - 6 * _t*(1 - _t))*evalBezier({ Pij[1],Pij[5],Pij[9],Pij[13] }, _s) +
		(6 * _t*(1 - _t) - 3 * pow(_t,2))*evalBezier({ Pij[2],Pij[6],Pij[10],Pij[14] }, _s) +
		3 * pow(_t,2)*evalBezier({ Pij[3],Pij[7],Pij[11],Pij[15] }, _s);

	if (eq(d_ds, glm::vec3(0, 0, 0)) || eq(d_dt, glm::vec3(0, 0, 0)))
		return glm::vec3(0, 0, 0);

	return glm::normalize(glm::cross(d_ds, d_dt));
}

Bezier2D::~Bezier2D(void)
{
}
