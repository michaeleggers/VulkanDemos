#ifndef MODEL_H_
#define MODEL_H_
#include <vector>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include "Triangle.h"
namespace obj
{
	/**
	* Wavefront Object model format.
	* An obj file has a simple structure:
	* Vertex:
	*     v x y z
	* Face:
	*     f v1 v2 v3                  #triangle
	*     f v1 v2 v3 v4               #quad
	*     f v/vt/vn v/vt/vn v/vt/vn   #vertex, texture, normal
	* Material: (not used here)
	*     mtllib conferenceRoom.mtl   #material file name
	*     usemtl surface37            #reference to the material
	* @author Andreas Klein
	* @date 19.04.09
	*/
	class Model  
	{
	public:
		//some typedefs for the collections
		typedef std::vector<Triangle*> PrimitiveCollection;
		typedef std::vector<Vertex*> VertexCollection;

		Model()
		{
		}

		/**
		* Loads an obj model from file. Throws an std::runtime_error if the file cannot be opened.
		* \param fileName the model file name.
		*/
		Model(const std::string& fileName)
		{
			Load(fileName);
		}

		~Model();


		/**
		* Loads an obj model from file. Throws an std::runtime_error if the file cannot be opened.
		* \param fileName the model filename
		*/
		void Load(const std::string& fileName);
		
		/**
		* Gets the number of triangles in the model.
		* \return the number of triangles
		*/
		unsigned int GetTriangleCount() const
		{
			return primitives.size();
		}

		/**
		* Gets a pointer to a triangle based on a given id.
		* \param id the id of the triangle.
		* \return a pointer to the triangle.
		*/
		Triangle* GetTriangle(int id) const
		{
			return primitives[id];
		}

		/**
		* Gets the number of vertices in the model.
		* \return the number of vertices.
		*/
		unsigned int GetVertexCount() const
		{
			return vertices.size();
		}

		/**
		* Gets a pointer to a vertex based on a given id.
		* \param id the vertex id.
		* \return a pointer to the vertex.
		*/
		Vertex* GetVertex(int id) const
		{
			return vertices[id];
		}

		/**
		* Gets all triangles that contains the given vertex. Example:

		  Vertex * v = model.GetVertex(id);
		  Model::PrimitiveCollection adjacentTriangles;
		  model.GetAdjacentTriangles(adjacentTriangles,v);

		* \param adjacent reference to a primitive collection. The adjacent triangles will be stored in it.
		* \param vertex a pointer to a vertex.
		*/
		void GetAdjacentTriangles(PrimitiveCollection& adjacent, Vertex* vertex);
	private:
	
		typedef std::map<unsigned int, PrimitiveCollection> AdjacentIndex;
		PrimitiveCollection primitives;
		VertexCollection vertices;
		AdjacentIndex adjacentIndex;

		/**
		* Parses the object file.
		* \param stream the file stream
		*/
		void readContents(std::ifstream& stream);
		/**
		* Parses a vertex from a stream.
		* \param stream the file stream
		*/
		void parseVertex(std::ifstream& stream);

		/**
		* Parses a vector3 from a stream.
		* \param stream the file stream
		*/
		Vertex* parseVector3(std::ifstream& stream);
		/**
		* Parses a face from a stream.
 		* \param stream the file stream.
		*/
		void parseFace(std::ifstream& stream);

		/**
		* Deletes the allocated memory
		*/
		void deleteContent();
	};
}
#endif
