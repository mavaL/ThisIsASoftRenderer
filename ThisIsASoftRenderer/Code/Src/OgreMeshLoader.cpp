#include "stdafx.h"
#include "OgreMeshLoader.h"
#include "Renderer.h"

namespace Ext
{
	bool OgreMeshLoader::LoadMeshFile( const STRING& filename )
	{
		TiXmlDocument doc;
		if(!doc.LoadFile(filename.c_str()))
		{
			throw std::logic_error("Error, Can't load .mesh file! Please make sure it's in a xml format!");
			return false;
		}

		m_obj.VB.clear();
		m_obj.faces.clear();

		TiXmlElement* submeshNode = doc.FirstChildElement("mesh")->FirstChildElement("submeshes")->FirstChildElement("submesh");

		//读取面信息
		{
			TiXmlElement* facesNode = submeshNode->FirstChildElement("faces");
			int nFace = 0;
			facesNode->Attribute("count", &nFace);

			m_obj.faces.resize(nFace);

			int idx = 0;
			TiXmlElement* faceNode = facesNode->FirstChildElement("face");
			while (faceNode)
			{
				int v1, v2, v3;
				faceNode->Attribute("v1", &v1);
				faceNode->Attribute("v2", &v2);
				faceNode->Attribute("v3", &v3);

				SR::SFace face(v1, v2, v3);
				m_obj.faces[idx++] = std::move(face);

				faceNode = faceNode->NextSiblingElement("face");
			}
		}

		//读取顶点数据
		{
			TiXmlElement* geometryNode = submeshNode->FirstChildElement("geometry");
			int nVert = 0;
			geometryNode->Attribute("vertexcount", &nVert);

			m_obj.VB.resize(nVert);

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

				SR::SVertex vert;
				vert.pos = std::move(VEC4(x, y, z, 1));
				vert.normal = std::move(VEC3(nx, ny, nz));
				vert.normal.Normalize();
				m_obj.VB[idx++] = std::move(vert);

				vertNode = vertNode->NextSiblingElement("vertex");
			}
		}

		//计算包围球
		m_obj.boundingRadius = SR::RenderUtil::ComputeBoundingRadius(m_obj.VB);

		//计算面法线
		for (size_t i=0; i<m_obj.faces.size(); ++i)
		{
			//fetch vertexs
			const SR::Index idx1 = m_obj.faces[i].index1;
			const SR::Index idx2 = m_obj.faces[i].index2;
			const SR::Index idx3 = m_obj.faces[i].index3;

			const VEC3& n1 = m_obj.VB[idx1].normal;
			const VEC3& n2 = m_obj.VB[idx2].normal;
			const VEC3& n3 = m_obj.VB[idx3].normal;

			//average
			VEC3 sum = Common::Add_Vec3_By_Vec3(Common::Add_Vec3_By_Vec3(n1, n2), n3);
			m_obj.faces[i].faceNormal = Common::Multiply_Vec3_By_K(sum, 0.33333f);
			m_obj.faces[i].faceNormal.Normalize();
		}

		return true;
	}
}

