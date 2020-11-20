#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "model.h"
#include "platform.h"

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

Model create_model_from_file(char const * file)
{
	Model model = {};
	aiScene const * scene = aiImportFile(file, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder | aiProcess_CalcTangentSpace);
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		model.vertex_count += scene->mMeshes[i]->mNumVertices;
	}
	model.vertices = (Vertex*)malloc(model.vertex_count * sizeof(Vertex));
	//p.push_array(arena, model.vertices, model.vertex_count * sizeof(Vertex));
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		for (int j = 0; j < scene->mMeshes[i]->mNumVertices; j += 3) {
			aiVector3D vertex0 = scene->mMeshes[i]->mVertices[j];
			aiVector3D vertex1 = scene->mMeshes[i]->mVertices[j + 1];
			aiVector3D vertex2 = scene->mMeshes[i]->mVertices[j + 2];
			model.vertices[j].pos = { vertex0.x, vertex0.y, vertex0.z };
			model.vertices[j + 1].pos = { vertex1.x, vertex1.y, vertex1.z };
			model.vertices[j + 2].pos = { vertex2.x, vertex2.y, vertex2.z };

			/* Tangent */
			if (scene->mMeshes[i]->HasTangentsAndBitangents()) {	
				aiVector3D tangent0 = scene->mMeshes[i]->mTangents[j];
				aiVector3D tangent1 = scene->mMeshes[i]->mTangents[j + 1];
				aiVector3D tangent2 = scene->mMeshes[i]->mTangents[j + 2];
				glm::vec3 t0 = glm::vec3(tangent0.x, tangent0.y, tangent0.z);
				glm::vec3 t1 = glm::vec3(tangent1.x, tangent1.y, tangent1.z);
				glm::vec3 t2 = glm::vec3(tangent2.x, tangent2.y, tangent2.z);
				model.vertices[j].tangent = t0;
				model.vertices[j + 1].tangent = t1;
				model.vertices[j + 2].tangent = t2;
			}
			if (scene->mMeshes[i]->HasNormals()) {
				aiVector3D normal0 = scene->mMeshes[i]->mNormals[j];
				aiVector3D normal1 = scene->mMeshes[i]->mNormals[j + 1];
				aiVector3D normal2 = scene->mMeshes[i]->mNormals[j + 2];
				model.vertices[j].normal = glm::normalize(glm::vec3(normal0.x, normal0.y, normal0.z));
				model.vertices[j + 1].normal = glm::normalize(glm::vec3(normal1.x, normal1.y, normal1.z));
				model.vertices[j + 2].normal = glm::normalize(glm::vec3(normal2.x, normal2.y, normal2.z));
			}
			else {
				aiVector3D a = vertex1 - vertex0;
				aiVector3D b = vertex2 - vertex0;
				glm::vec3 normal = glm::cross(normalize(glm::vec3(a.x, a.y, a.z)), normalize(glm::vec3(b.x, b.y, b.z)));
				model.vertices[j].normal = normal;
				model.vertices[j + 1].normal = normal;
				model.vertices[j + 2].normal = normal;
			}
			if (scene->mMeshes[i]->HasTextureCoords(i)) {
				model.vertices[j].uv.x = scene->mMeshes[i]->mTextureCoords[i][j].x;
				model.vertices[j].uv.y = scene->mMeshes[i]->mTextureCoords[i][j].y;
				model.vertices[j + 1].uv.x = scene->mMeshes[i]->mTextureCoords[i][j + 1].x;
				model.vertices[j + 1].uv.y = scene->mMeshes[i]->mTextureCoords[i][j + 1].y;
				model.vertices[j + 2].uv.x = scene->mMeshes[i]->mTextureCoords[i][j + 2].x;
				model.vertices[j + 2].uv.y = scene->mMeshes[i]->mTextureCoords[i][j + 2].y;
			}
			else {
				// No UVs!
			}
		}
	}

	return model;
}

Model create_model_from_file2(char const * file, Platform p, MemoryArena * arena)
{
	Model model = {};
	/* Find out how many vertices and indices this model has in total.
	Assimp makes sure to use multiple vertices per face by the flag 'aiProcess_JoinIdenticalVertices'.
	So we allocate exactly as much memory as is required for the vertex-buffer.
	*/
	aiScene const * scene = aiImportFile(file, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder | aiProcess_GenBoundingBoxes);
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		model.vertex_count += scene->mMeshes[i]->mNumVertices;
		for (int f = 0; f < scene->mMeshes[i]->mNumFaces; ++f) {
			model.index_count += scene->mMeshes[i]->mFaces[f].mNumIndices;
			model.face_count++;
		}
	}
	model.vertices = (Vertex*)malloc(model.vertex_count * sizeof(Vertex));
	model.indices = (uint32_t*)malloc(model.index_count * sizeof(uint32_t));

	/* Go through all faces of meshes and copy. */
	uint32_t indices_copied = 0;
	uint32_t vertices_copied = 0;
	for (int m = 0; m < scene->mNumMeshes; ++m) {
		for (int f = 0; f < scene->mMeshes[m]->mNumFaces; ++f) {
			for (int i = 0; i < scene->mMeshes[m]->mFaces[f].mNumIndices; ++i) {
				/* Get current index/vertex/normal from model file */
				uint32_t * index = (uint32_t*)&(scene->mMeshes[m]->mFaces[f].mIndices[i]);
				aiVector3D vertex = scene->mMeshes[m]->mVertices[*index];

				/* copy into Model data */
				model.indices[indices_copied] = *index;
				Vertex * current_vertex = &model.vertices[*index];
				current_vertex->pos.x = vertex.x;
				current_vertex->pos.y = vertex.y;
				current_vertex->pos.z = vertex.z;

				if (scene->mMeshes[m]->HasTextureCoords(0)) {
					aiVector3D uv = scene->mMeshes[m]->mTextureCoords[0][*index];
					current_vertex->uv.x = uv.x;
					current_vertex->uv.y = uv.y;
				}

				if (scene->mMeshes[m]->HasNormals()) {
					aiVector3D normal = scene->mMeshes[m]->mNormals[*index];
					current_vertex->normal.x = normal.x;
					current_vertex->normal.y = normal.y;
					current_vertex->normal.z = normal.z;
				}

				/* add debug color */
				{
					current_vertex->color = glm::vec4(1.f, 0.84f, 0.f, 0.f);
				}

				indices_copied++;
			}
		}

		aiAABB aabb = scene->mMeshes[m]->mAABB;
		model.bounding_box.min_xyz = glm::vec4(aabb.mMin.x, aabb.mMin.y, aabb.mMin.z, 1.0f);
		model.bounding_box.max_xyz = glm::vec4(aabb.mMax.x, aabb.mMax.y, aabb.mMax.z, 1.0f);
	}

	return model;
}

void delete_model(Model model)
{
	free(model.indices);
	free(model.vertices);
}

Image load_image_file(char const * file)
{
	Image image = {};
	int tw, th, tn;
	image.data = stbi_load(file, &tw, &th, &tn, 4);
	assert(image.data != NULL);
	image.width = tw;
	image.height = th;
	image.channels = tn;

	return image;
}

void assign_texture_to_model(Model * model, Texture texture, uint32_t id, TextureType texture_type)
{
	assert( (texture_type < MAX_TEXTURE_TYPES) && "Unknown TextureType!" );
	if (texture_type == TEXTURE_TYPE_DIFFUSE) {
		model->material.texture_id = id;
		model->texture = texture;
		model->material.is_textured = 1;
	}
	else if (texture_type == TEXTURE_TYPE_NORMAL) {
		model->material.normal_id = id;
		model->normal_map = texture;
		model->material.has_normal_map = 1;
	}
}
