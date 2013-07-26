#include "stdafx.h"
#include "OgreMeshLoader.h"
#include "Renderer.h"

namespace Ext
{
	bool OgreMeshLoader::LoadImpl( const STRING& filename )
	{
		TiXmlDocument doc;
		if(!doc.LoadFile(filename.c_str()))
		{
			throw std::logic_error("Error, Can't load .mesh file! Please make sure it's xml format!");
			return false;
		}

		m_objs.clear();
		m_objs.push_back(SR::RenderObject());
		SR::RenderObject& obj = m_objs.back();

		TiXmlElement* submeshNode = doc.FirstChildElement("mesh")->FirstChildElement("submeshes")->FirstChildElement("submesh");

		//读取面信息
		{
			TiXmlElement* facesNode = submeshNode->FirstChildElement("faces");
			int nFace = 0;
			facesNode->Attribute("count", &nFace);

			obj.m_faces.resize(nFace);

			int idx = 0;
			TiXmlElement* faceNode = facesNode->FirstChildElement("face");
			while (faceNode)
			{
				int v1, v2, v3;
				faceNode->Attribute("v1", &v1);
				faceNode->Attribute("v2", &v2);
				faceNode->Attribute("v3", &v3);

				SR::SFace face(v1, v2, v3);
				obj.m_faces[idx++] = std::move(face);

				faceNode = faceNode->NextSiblingElement("face");
			}
		}

		//读取顶点数据
		{
			TiXmlElement* geometryNode = submeshNode->FirstChildElement("geometry");
			int nVert = 0;
			geometryNode->Attribute("vertexcount", &nVert);

			obj.m_verts.resize(nVert);

			TiXmlElement* vbNode = geometryNode->FirstChildElement("vertexbuffer");
			//check what we have..
			if(vbNode->Attribute("positions") != STRING("true"))
			{
				throw std::logic_error("Error, the .mesh file doesn't even have vertex position info!");
				return false;
			}
			if(vbNode->Attribute("normals") != STRING("true"))
			{
				throw std::logic_error("Error, the .mesh file doesn't even have vertex normal info!");
				return false;
			}

			int idx = 0;
			TiXmlElement* vertNode = vbNode->FirstChildElement("vertex");
			while (vertNode)
			{
				//position
				TiXmlElement* posNode = vertNode->FirstChildElement("position");
				double x, y, z;
				posNode->Attribute("x", &x);
				posNode->Attribute("y", &y);
				posNode->Attribute("z", &z);

				//normal
				TiXmlElement* normalNode = vertNode->FirstChildElement("normal");
				double nx, ny, nz;
				normalNode->Attribute("x", &nx);
				normalNode->Attribute("y", &ny);
				normalNode->Attribute("z", &nz);

				//uv
				TiXmlElement* uvNode = vertNode->FirstChildElement("texcoord");
				double texu, texv;
				if(uvNode)
				{
					uvNode->Attribute("u", &texu);
					uvNode->Attribute("v", &texv);

					//NB: 纹理目前只支持.bmp格式
					texv = 1 - texv;
				}

				SR::SVertex vert;
				vert.pos = VEC4(x, y, z, 1);
				vert.normal = VEC3(nx, ny, nz);
				vert.normal.Normalize();
				if(uvNode)
					vert.uv = VEC2(texu, texv);
				obj.m_verts[idx++] = std::move(vert);

				vertNode = vertNode->NextSiblingElement("vertex");
			}
		}

		return true;
	}
}

