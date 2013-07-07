#include "stdafx.h"
#include "OgreMeshLoader.h"

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

		m_VB.clear();
		m_IB.clear();

		TiXmlElement* submeshNode = doc.FirstChildElement("mesh")->FirstChildElement("submeshes")->FirstChildElement("submesh");

		//读取面信息
		{
			TiXmlElement* facesNode = submeshNode->FirstChildElement("faces");
			int nFace = 0;
			facesNode->Attribute("count", &nFace);

			m_IB.resize(nFace * 3);

			int curPos = 0;
			TiXmlElement* faceNode = facesNode->FirstChildElement("face");
			while (faceNode)
			{
				int v1, v2, v3;
				faceNode->Attribute("v1", &v1);
				faceNode->Attribute("v2", &v2);
				faceNode->Attribute("v3", &v3);

				m_IB[curPos++] = v1;
				m_IB[curPos++] = v2;
				m_IB[curPos++] = v3;

				faceNode = faceNode->NextSiblingElement("face");
			}
		}

		//读取顶点数据
		{
			TiXmlElement* geometryNode = submeshNode->FirstChildElement("geometry");
			int nVert = 0;
			geometryNode->Attribute("vertexcount", &nVert);

			m_VB.resize(nVert);

			TiXmlElement* vbNode = geometryNode->FirstChildElement("vertexbuffer");
			if(vbNode->Attribute("positions") != STRING("true"))
			{
				throw std::logic_error("Error, the .mesh file doesn't even have vertex position info!");
				return false;
			}

			int curPos = 0;
			TiXmlElement* vertNode = vbNode->FirstChildElement("vertex");
			while (vertNode)
			{
				TiXmlElement* posNode = vertNode->FirstChildElement("position");

				double x, y, z;
				posNode->Attribute("x", &x);
				posNode->Attribute("y", &y);
				posNode->Attribute("z", &z);

				SR::SVertex vert;
				vert.pos = VEC4(x, y, z, 1);
				m_VB[curPos++] = std::move(vert);

				vertNode = vertNode->NextSiblingElement("vertex");
			}
		}

		return true;
	}
}

