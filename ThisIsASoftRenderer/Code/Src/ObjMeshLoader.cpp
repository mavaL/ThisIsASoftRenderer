#include "stdafx.h"
#include "ObjMeshLoader.h"
#include <fstream>
#include "Renderer.h"

using namespace std;

namespace Ext
{
	bool ObjMeshLoader::LoadImpl( const STRING& filename )
	{
		std::ifstream file(filename.c_str());
		if(file.fail())
			return false;

		m_objs.clear();

		//.obj格式每个物体的f各索引竟然是从前面所有物体来累加的.
		//艹,这些变量用来减去累加部分,太麻烦了
		DWORD nTotalPosCount, nTotalUvCount, nTotalNormalCount;
		nTotalPosCount = nTotalUvCount = nTotalNormalCount = 0;

		//each object
		for (;;)
		{
			if(file.eof())
				break;

			m_objs.push_back(SR::RenderObject());
			SR::RenderObject& obj = m_objs.back();

			//获取数据量,避免反复分配存储空间
			DWORD nPos, nUv, nNormal, nFace;
			nPos = nUv = nNormal = nFace = 0;
			_PreReadObject(file, nPos, nUv, nNormal, nFace);

			vector<VEC4> vecPos(nPos);
			vector<VEC2> vecUv(nUv);
			vector<VEC3> vecNormal(nNormal);

			obj.m_verts.reserve(nFace*3);
			obj.m_faces.resize(nFace);
			m_vecComp.clear();
			m_vecComp.reserve(nFace*3);

			//正式读取数据
			DWORD curVert, curUv, curNormal, curFace;
			curVert = curUv = curNormal = curFace = 0;
			STRING command;
			bool bFlush = false;

			//each command
			for(;;)
			{
				if(file.eof())
					break;

				file >> command;

				if (strcmp(command.c_str(), "v") == 0)
				{
					if(bFlush)
					{
						//解析下一个物体
						file.seekg(-1, ios_base::cur);
						break;
					}

					VEC4& pos = vecPos[curVert++];
					file >> pos.x >> pos.y >> pos.z;
					pos.w = 1;
				}
				else if (strcmp(command.c_str(), "vt") == 0)
				{
					VEC2& uv = vecUv[curUv++];
					file >> uv.x >> uv.y;
					//NB: 纹理目前只支持.bmp格式
					//uv.y = 1 - uv.y;
				}
				else if (strcmp(command.c_str(), "vn") == 0)
				{
					VEC3& normal = vecNormal[curNormal++];
					file >> normal.x >> normal.y >> normal.z;
				}
				else if (strcmp(command.c_str(), "f") == 0)
				{
					DWORD idxPos[3] = {0};
					DWORD idxUv[3] = {0};
					DWORD idxNormal[3] = {0};
					SR::Index idxVert[3] = {0};

					for (int i=0; i<3; ++i)
					{
						file >> idxPos[i];
						file.ignore(10, '/');
						file >> idxUv[i];
						file.ignore(10, '/');
						file >> idxNormal[i];

						//.obj索引是从1开始的
						idxPos[i] -= 1; idxUv[i] -= 1; idxNormal[i] -= 1;
						//正如前面所说,减去累加的部分
						idxPos[i] -= nTotalPosCount;
						idxUv[i] -= nTotalUvCount;
						idxNormal[i] -= nTotalNormalCount;

						SR::SVertex vertex;
						vertex.pos		= vecPos[idxPos[i]];
						vertex.uv		= vecUv[idxUv[i]];
						vertex.normal	= vecNormal[idxNormal[i]];

						SVertCompare comp = { idxPos[i], idxUv[i], idxNormal[i] };

						_DefineVertex(vertex, comp, obj, idxVert[i]);
					}

					SR::SFace& face = obj.m_faces[curFace++];
					face.index1 = idxVert[0]; face.index2 = idxVert[1]; face.index3 = idxVert[2];
				}
				else if (strcmp(command.c_str(), "g") == 0)
				{
					bFlush = true;
				}
				else if (strcmp(command.c_str(), "mtllib") == 0)
				{
					STRING filename;
					file >> filename;
					assert(!filename.empty());

					if(!_ReadMtrl(filename))
						return false;
				}
				else if (strcmp(command.c_str(), "usemtl") == 0)
				{
					STRING matName;
					file >> matName;

					obj.m_pMaterial = g_env.renderer->GetMaterial(matName);
				}

				//读下一行
				file.ignore(1000, '\n');
			}

			nTotalPosCount += nPos;
			nTotalUvCount += nUv;
			nTotalNormalCount += nNormal;
		}
				
		return true;
	}

	void ObjMeshLoader::_PreReadObject( std::ifstream& file, DWORD& nPos, DWORD& nUv, DWORD& nNormal, DWORD& nFace )
	{
		//记录下当前位置,返回的时候回退到该位置
		streamoff pos = file.tellg();

		bool bFlush = false;
		STRING command;
		while (1)
		{
			if(file.eof())
			{
				//NB: 到达末尾后需要清除状态再回退
				file.clear();
				file.seekg(pos, ios_base::beg);
				return;
			}

			file >> command;

			if (strcmp(command.c_str(), "v") == 0)
			{
				if(bFlush)
				{
					file.seekg(pos, ios_base::beg);
					return;
				}
				++nPos;
			}
			else if (strcmp(command.c_str(), "vt") == 0)
			{
				++nUv;
			}
			else if (strcmp(command.c_str(), "vn") == 0)
			{
				++nNormal;
			}
			else if (strcmp(command.c_str(), "f") == 0)
			{
				++nFace;
			}
			else if (strcmp(command.c_str(), "g") == 0)
			{
				bFlush = true;
			}

			//读下一行
			file.ignore(1000, '\n');
		}
	}

	void ObjMeshLoader::_DefineVertex( const SR::SVertex& vert, const SVertCompare& comp, SR::RenderObject& obj, SR::Index& retIdx )
	{
		//.obj这种面的定义不是直接给顶点索引,而是各成分都可自由组合.所以构建m_verts要麻烦些.
		//1. 在当前顶点列表中查找,是否有相同顶点
		auto iter = std::find(m_vecComp.begin(), m_vecComp.end(), comp);

		if(iter != m_vecComp.end())
		{
			//2. 如果有,则可重复利用
			retIdx = std::distance(m_vecComp.begin(), iter);
		}
		else
		{
			//3. 如果没有,则插入新顶点到末尾
			obj.m_verts.push_back(vert);
			m_vecComp.push_back(comp);
			retIdx = obj.m_verts.size() - 1;
		}
	}

	bool ObjMeshLoader::_ReadMtrl( const STRING& filename )
	{
		const STRING filepath = GetResPath(filename);
		ifstream file(filepath);
		if(file.fail())
			return false;

		STRING command;
		SR::SMaterial* pNewMaterial = nullptr;
		STRING matName;

		//each command
		for(;;)
		{
			if(file.eof())
				break;

			file >> command;

			if (strcmp(command.c_str(), "newmtl") == 0)
			{
				//flush last material to MatLib
				if(pNewMaterial)
				{
					g_env.renderer->AddMaterial(matName, pNewMaterial);
				}

				file >> matName;
				pNewMaterial = new SR::SMaterial;
			}
			else if (strcmp(command.c_str(), "map_Kd") == 0)
			{
				STRING texName;
				file >> texName;
				pNewMaterial->pDiffuseMap = new SR::STexture;
				pNewMaterial->pDiffuseMap->LoadTexture(GetResPath(texName));
			}
			else if (strcmp(command.c_str(), "bump") == 0)
			{
				STRING texName;
				file >> texName;
				pNewMaterial->pNormalMap = new SR::STexture;
				pNewMaterial->pNormalMap->LoadTexture(GetResPath(texName));
			}
			else if (strcmp(command.c_str(), "Ka") == 0)
			{
				file >> pNewMaterial->ambient.r >> pNewMaterial->ambient.g >> pNewMaterial->ambient.b;
			}
			else if (strcmp(command.c_str(), "Kd") == 0)
			{
				file >> pNewMaterial->diffuse.r >> pNewMaterial->diffuse.g >> pNewMaterial->diffuse.b;
			}
			else if (strcmp(command.c_str(), "Ks") == 0)
			{
				file >> pNewMaterial->specular.r >> pNewMaterial->specular.g >> pNewMaterial->specular.b;
			}
			else if (strcmp(command.c_str(), "Ns") == 0)
			{
				file >> pNewMaterial->shiness;
			}

			//读下一行
			file.ignore(1000, '\n');
		}

		//manually add last one
		if(pNewMaterial)
		{
			g_env.renderer->AddMaterial(matName, pNewMaterial);
		}

		return true;
	}

}
