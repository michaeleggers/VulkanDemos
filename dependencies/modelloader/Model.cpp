#include "Model.h"

#include <stdexcept>
#include <cstring>
using namespace std;
namespace obj
{
	Vertex* Model::parseVector3(ifstream& stream)
	{
		//allocate the vertex on heap
		Vertex* vertex = new Vertex();
		stream >> vertex->pos.x;
		stream >> vertex->pos.y;
		stream >> vertex->pos.z;
		return vertex;
	}

	void Model::Load( const string& fileName )
	{
		//open the stream
		std::ifstream stream;
		stream.open(fileName.c_str());
		if (!stream.is_open())
		{
			throw runtime_error("Can't load model file!");
		}
		deleteContent();
		//parse the file
		readContents(stream);
	}

	void Model::readContents( ifstream& stream )
	{
		char cmd[256] = {0};
		while(true)
		{
			stream >> cmd;
			if (!stream){
				break;
			}
			if (strcmp(cmd,"v")==0)
			{
				parseVertex(stream);
			}
			else if (strcmp(cmd,"f")==0)
			{
				parseFace(stream);
			}
			//if needed, add more commands here.
			stream.ignore(1000,'\n');
			
		}
	}

	void Model::parseFace( ifstream& stream )
	{
		unsigned int val;
		unsigned int verts[3];
		//assume that only triangle faces exists
		for (int i = 0; i < 3;++i)
		{
			// get the vertex id
			stream >> val;
			verts[i] = val-1;
			//ignore the texture and normal indices
			if (stream.peek() == '/')
			{
				stream.ignore();
				if (stream.peek() == '/')
				{
					stream.ignore();
				}
			}
		}
		//allocate the triangle on heap
		Triangle* triangle = new Triangle();
		//add the triangle to the adjacent list for each vertex
		for (int i = 0; i < 3;++i){
			triangle->vertex[i]=vertices[verts[i]];
			adjacentIndex[triangle->vertex[i]->id].push_back(triangle);
		}
		primitives.push_back(triangle);
	}

	void Model::parseVertex(ifstream& stream )
	{
		Vertex* vertex = parseVector3(stream);
		//assign the vertex id, its just its position in the array
		vertex->id = vertices.size();
		vertices.push_back(vertex);
	}

	void Model::GetAdjacentTriangles(std::vector<Triangle*>& adjacent, Vertex* t )
	{
		PrimitiveCollection collection = adjacentIndex[t->id];
		adjacent.resize(collection.size());
		//copy it to the output
		std::copy(collection.begin(),collection.end(),adjacent.begin());
	}

	Model::~Model()
	{
		deleteContent();
	}

	void Model::deleteContent()
	{
		//delete the triangles an the vertices
		for (unsigned int i =0; i < primitives.size();++i)
			delete primitives[i];

		primitives.clear();

		for (unsigned int i =0; i < vertices.size();++i)
			delete vertices[i];

		vertices.clear();

		adjacentIndex.clear();
	}
}
