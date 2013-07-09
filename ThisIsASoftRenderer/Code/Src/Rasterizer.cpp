#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"

namespace SR
{
	void RasWireFrame::RasterizeTriangleList( const VertexBuffer& vb, const IndexBuffer& ib )
	{
		//each triangle
		int nFace = ib.size() / 3;

		for (int iFace=0; iFace<nFace; ++iFace)
		{
			const Index idx1 = ib[iFace * 3 + 0];
			const Index idx2 = ib[iFace * 3 + 1];
			const Index idx3 = ib[iFace * 3 + 2];

			const VEC4& p0 = vb[idx1].pos;
			const VEC4& p1 = vb[idx2].pos;
			const VEC4& p2 = vb[idx3].pos;

			//each line
			RenderUtil::DrawLine_Bresenahams(p0.x, p0.y, p1.x, p1.y, 0xffffffff, true);
			RenderUtil::DrawLine_Bresenahams(p1.x, p1.y, p2.x, p2.y, 0xffffffff, true);
			RenderUtil::DrawLine_Bresenahams(p0.x, p0.y, p2.x, p2.y, 0xffffffff, true);
		}
	}
 }