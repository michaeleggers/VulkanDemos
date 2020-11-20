#ifndef TRIANGLE_H_
#define TRIANGLE_H_
#include "Vertex.h"
namespace obj
{
	/**
	* Represents a triangle. It has three vertices and a normal.
	* \author Andreas Klein
	* \date 19.04.09
	*/
	struct Triangle 
	{
		//the vertices
		Vertex* vertex[3];
		//normal of the triangle
		Vector3 normal;
	};
}
#endif

