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

				//uv
				TiXmlElement* uvNode = vertNode->FirstChildElement("texcoord");
				double texu, texv;
				if(uvNode)
				{
					uvNode->Attribute("u", &texu);
					uvNode->Attribute("v", &texv);
				}

				SR::SVertex vert;
				vert.pos = VEC4(x, y, z, 1);
				vert.normal = VEC3(nx, ny, nz);
				vert.normal.Normalize();
				if(uvNode)
					vert.uv = VEC2(texu, texv);
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

			const VEC4& p0 = m_obj.VB[idx1].pos;
			const VEC4& p1 = m_obj.VB[idx2].pos;
			const VEC4& p2 = m_obj.VB[idx3].pos;

			//NB: 顶点必须是逆时针顺序
			const VEC3 u = Common::Sub_Vec4_By_Vec4(p1, p0).GetVec3();
			const VEC3 v = Common::Sub_Vec4_By_Vec4(p2, p0).GetVec3();
			m_obj.faces[i].faceNormal = Common::CrossProduct_Vec3_By_Vec3(u, v);
			m_obj.faces[i].faceNormal.Normalize();
		}

		m_obj.matWorldIT = m_obj.matWorld.Inverse();
		m_obj.matWorldIT = m_obj.matWorldIT.Transpose();

		return true;
	}
}

