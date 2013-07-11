#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"

namespace SR
{
	void RasWireFrame::RasterizeTriangleList( const VertexBuffer& vb, FaceList& faces )
	{
		//each triangle
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			if(faces[iFace].IsBackface)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const VEC4& p0 = vb[idx1].pos;
			const VEC4& p1 = vb[idx2].pos;
			const VEC4& p2 = vb[idx3].pos;

			assert(vb[idx1].bActive && vb[idx2].bActive && vb[idx3].bActive && "Shit, this can't be true!");

			//each line
			RenderUtil::DrawLine_Bresenahams(p0.x, p0.y, p1.x, p1.y, 0xffffffff, true);
			RenderUtil::DrawLine_Bresenahams(p1.x, p1.y, p2.x, p2.y, 0xffffffff, true);
			RenderUtil::DrawLine_Bresenahams(p0.x, p0.y, p2.x, p2.y, 0xffffffff, true);
		}
	}

	void RasFlat::RasterizeTriangleList( const VertexBuffer& vb, FaceList& faces )
	{
		//用画家算法对所有面进行排序(根据三角面3个顶点的平均z值)
		//NB: 该算法在某些面重叠的情况下是不正确的,见<<3D编程大师技巧>>
		std::sort(faces.begin(), faces.end(), [&](const SFace& face1, const SFace& face2)->bool
		{
			const Index idx1 = face1.index1;
			const Index idx2 = face1.index2;
			const Index idx3 = face1.index3;

			float z1 = vb[idx1].pos.z + vb[idx2].pos.z + vb[idx3].pos.z;
			z1 *= 0.33333f;

			const Index idx4 = face2.index1;
			const Index idx5 = face2.index2;
			const Index idx6 = face2.index3;

			float z2 = vb[idx4].pos.z + vb[idx5].pos.z + vb[idx6].pos.z;
			z2 *= 0.33333f;

			return z1 > z2;
		});

		//each triangle
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			if(faces[iFace].IsBackface)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const VEC4& p0 = vb[idx1].pos;
			const VEC4& p1 = vb[idx2].pos;
			const VEC4& p2 = vb[idx3].pos;

			assert(vb[idx1].bActive && vb[idx2].bActive && vb[idx3].bActive && "Shit, this can't be true!");

			RenderUtil::DrawTriangle_Scanline(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, faces[iFace].color);			
		}
	}

}