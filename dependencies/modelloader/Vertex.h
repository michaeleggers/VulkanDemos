#ifndef VERTEX_H_
#define VERTEX_H_
#include "Vector3.h"
namespace obj
{
	/**
	* Vertex representation. A vertex has a id, position and a normal.
	* \author Andreas Klein
	* \date 24.04.09
	*/
	struct Vertex
	{
		// id of the vertex, used to find the adjacent triangles
		unsigned int id;

		//position of the vertex
		Vector3 pos;

		//normal of the vertex
		Vector3 normal;

		Vertex()
			:id(0){}
	};
}
#endif