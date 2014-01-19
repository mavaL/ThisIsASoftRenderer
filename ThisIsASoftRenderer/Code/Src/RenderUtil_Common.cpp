#include "stdafx.h"
#include "RenderUtil.h"
#include "Renderer.h"
#include "PixelBox.h"
#include "RenderObject.h"
#include "Profiler.h"

namespace SR
{
	void RenderUtil::DoBackfaceCulling( VertexBuffer& vb, FaceList& workingFaces, RenderObject& obj )
	{
		vb.assign(obj.m_verts.begin(), obj.m_verts.end());

		const VEC4& camPos = g_env.renderer->m_camera.GetPos();

		for (size_t i=0; i<workingFaces.size(); ++i)
		{
			SFace& face = workingFaces[i];
			
			//fetch vertexs
			const SR::Index idx1 = face.index1;
			const SR::Index idx2 = face.index2;
			const SR::Index idx3 = face.index3;

			const VEC4& pos1 = obj.m_verts[idx1].pos;
			const VEC4& pos2 = obj.m_verts[idx2].pos;
			const VEC4& pos3 = obj.m_verts[idx3].pos;

			VEC4 faceToCam = Common::Add_Vec4_By_Vec4(Common::Add_Vec4_By_Vec4(pos1, pos2), pos3);
			faceToCam = Common::Multiply_Vec4_By_K(faceToCam, 0.33333f);
			faceToCam.w = 1;
			Common::Transform_Vec4_By_Mat44(faceToCam, faceToCam, obj.m_matWorld);
			faceToCam = Common::Sub_Vec4_By_Vec4(camPos, faceToCam);

			VEC4 faceWorldNormal = Common::Transform_Vec3_By_Mat44(face.faceNormal, obj.m_matWorldIT, false);

			if(!obj.m_pMaterial->bTwoSide &&	// Two side geometry not do back face culling. [1/18/2014 mavaL]
				Common::DotProduct_Vec3_By_Vec3(faceToCam.GetVec3(), faceWorldNormal.GetVec3()) <= 0.0f)
			{
				face.IsBackface = true;
#if USE_PROFILER == 1
				++g_env.profiler->m_frameStatics.nBackFace;
#endif
			}
			else
			{
				vb[idx1].bActive = true;
				vb[idx2].bActive = true;
				vb[idx3].bActive = true;
			}
		}
	}

	void RenderUtil::Do3DClipping( VertexBuffer& VB, FaceList& faces )
	{
		Camera* cam = &g_env.renderer->m_camera;

		const float n = cam->GetNearClip();
		const float f = cam->GetFarClip();
		const float fov = cam->GetFov();
		const float half_w = n * std::tan(fov/2);
		const float half_h = half_w / cam->GetAspectRatio();

		size_t nFaces = faces.size();
		for (size_t i=0; i<nFaces; ++i)
		{
			SFace& face = faces[i];

			if(face.IsBackface)
				continue;

			//fetch vertexs
			const SR::Index idx1 = face.index1;
			const SR::Index idx2 = face.index2;
			const SR::Index idx3 = face.index3;

			SVertex& vert1 = VB[idx1];
			SVertex& vert2 = VB[idx2];
			SVertex& vert3 = VB[idx3];

			//左右面
			float x1 = half_w * vert1.pos.z / -n;
			float x2 = half_w * vert2.pos.z / -n;
			float x3 = half_w * vert3.pos.z / -n;

			if(	(vert1.pos.x < -x1 && vert2.pos.x < -x2 && vert3.pos.x < -x3) ||
				(vert1.pos.x > x1 && vert2.pos.x > x2 && vert3.pos.x > x3)	)
			{
				face.bCulled = true;
#if USE_PROFILER == 1
				++g_env.profiler->m_frameStatics.nFaceCulled;
#endif
				continue;
			}

			//上下面
			float y1 = half_h * vert1.pos.z / -n;
			float y2 = half_h * vert2.pos.z / -n;
			float y3 = half_h * vert3.pos.z / -n;

			if(	(vert1.pos.y < -y1 && vert2.pos.y < -y2 && vert3.pos.y < -y3) ||
				(vert1.pos.y > y1 && vert2.pos.y > y2 && vert3.pos.y > y3)	)
			{
				face.bCulled = true;
#if USE_PROFILER == 1
				++g_env.profiler->m_frameStatics.nFaceCulled;
#endif
				continue;
			}

			//远裁面
			if(-vert1.pos.z > f && -vert2.pos.z > f && -vert3.pos.z > f)
			{
				face.bCulled = true;
#if USE_PROFILER == 1
				++g_env.profiler->m_frameStatics.nFaceCulled;
#endif
				continue;
			}

			// 近裁面,需要处理3种情况:
			int nVertOut = 0;
			bool flags[3] = { false };
			if(-vert1.pos.z < n) { flags[0] = true; ++nVertOut; }
			if(-vert2.pos.z < n) { flags[1] = true; ++nVertOut; }
			if(-vert3.pos.z < n) { flags[2] = true; ++nVertOut; }

			/*	1.完全在视锥外,则剔除:

				  ___________ near clip plane
					  p0
					  /\
					 /  \
					/____\
				  p1	  p2					*/

			if(flags[0] && flags[1] && flags[2])
			{
				face.bCulled = true;
#if USE_PROFILER == 1
				++g_env.profiler->m_frameStatics.nFaceCulled;
#endif
				continue;
			}

			/*	2.2个顶点在视锥内,1个顶点在视锥外,裁剪后需要分割为2个三角面:

				   p0______p2
				     \    /
				 _____\__/_____ near clip plane
				       \/
				       p1							*/

			else if(nVertOut == 1)
			{
				//找出内外的顶点
				SVertex *p0, *p1, *p2; 
				Index idxp0, idxp1, idxp2;
				if(flags[0])		{ p1 = &vert1; p0 = &vert2; p2 = &vert3; idxp1 = idx1; idxp0 = idx2; idxp2 = idx3; }
				else if(flags[1])	{ p1 = &vert2; p0 = &vert1; p2 = &vert3; idxp1 = idx2; idxp0 = idx1; idxp2 = idx3; }
				else				{ p1 = &vert3; p0 = &vert1; p2 = &vert2; idxp1 = idx3; idxp0 = idx1; idxp2 = idx2; }

				//直线参数化方程求t
				const VEC4 line1 = Common::Sub_Vec4_By_Vec4(p1->pos, p0->pos);
				float t1 = (-n - p0->pos.z)/(line1.z);
				//交点1
				float newX1 = p0->pos.x + line1.x * t1;
				float newY1 = p0->pos.y + line1.y * t1;

				//交点2得创建新的顶点和切割新的面
				const VEC4 line2 = Common::Sub_Vec4_By_Vec4(p1->pos, p2->pos);
				float t2 = (-n - p2->pos.z)/(line2.z);
				float newX2 = p2->pos.x + line2.x * t2;
				float newY2 = p2->pos.y + line2.y * t2;
	
				SVertex newVert;
				newVert.bActive = true;
				newVert.pos.Set(newX2, newY2, -n, 1.0f);
				newVert.normal = p1->normal;

				SFace newFace;
				newFace.faceNormal = face.faceNormal;
				newFace.color = face.color;
				newFace.index1 = idxp2;
				newFace.index2 = idxp1;
				newFace.index3 = VB.size();

				//需要重新插值的属性
				g_env.renderer->GetCurRas()->LerpVertexAttributes(&newVert, p2, p1, t2, eLerpType_Linear);
				g_env.renderer->GetCurRas()->LerpVertexAttributes(p1, p0, p1, t1, eLerpType_Linear);

				//交点1覆盖原来的p1
				p1->pos.Set(newX1, newY1, -n, 1);

				//NB: 最后进行插入操作,不然会使指向元素的指针无效化
				VB.push_back(newVert);
				faces.push_back(newFace);
			}

			/*    3.1个顶点在视锥内,2个顶点在视锥外,裁剪后还是1个三角面:

						p0
						/\
				  _____/__\_____ near clip plane
					  /____\
					 p1	   p2

													*/
			else if(nVertOut == 2)
			{
				//找出内外的顶点
				SVertex *p0, *p1, *p2; 
				if(flags[0] && flags[1])		{ p1 = &vert1; p2 = &vert2; p0 = &vert3; }
				else if(flags[0] && flags[2])	{ p1 = &vert1; p2 = &vert3; p0 = &vert2; }
				else							{ p1 = &vert2; p2 = &vert3; p0 = &vert1; }

				//直线参数化方程求t
				const VEC4 line1 = Common::Sub_Vec4_By_Vec4(p1->pos, p0->pos);
				float t1 = (-n - p0->pos.z)/(line1.z);
				float newX1 = p0->pos.x + line1.x * t1;
				float newY1 = p0->pos.y + line1.y * t1;
				//覆盖原来的p1
				p1->pos.Set(newX1, newY1, -n, 1);

				//另一个交点
				const VEC4 line2 = Common::Sub_Vec4_By_Vec4(p2->pos, p0->pos);
				float t2 = (-n - p0->pos.z)/(line2.z);
				float newX2 = p0->pos.x + line2.x * t2;
				float newY2 = p0->pos.y + line2.y * t2;
				//覆盖原来的p2
				p2->pos.Set(newX2, newY2, -n, 1);

				//需要重新插值的属性
				g_env.renderer->GetCurRas()->LerpVertexAttributes(p2, p0, p2, t2, eLerpType_Linear);
				g_env.renderer->GetCurRas()->LerpVertexAttributes(p1, p0, p1, t1, eLerpType_Linear);
			}
		}
	}

	void RenderUtil::ObjectTnL( RenderObject& obj, SRenderContext& context )
	{
		Camera* cam = &g_env.renderer->m_camera;
		VertexBuffer& workingVB = context.verts;
		FaceList& faces = context.faces;

		/////////////////////////////////////////////////
		///////// 视锥裁减
		if(cam->ObjectFrustumCulling(obj))
		{
#if USE_PROFILER == 1
			++g_env.profiler->m_frameStatics.nObjCulled;
#endif
			return;
		}

		faces.assign(obj.m_faces.begin(), obj.m_faces.end());

		/////////////////////////////////////////////////
		///////// 世界空间进行背面剔除
		DoBackfaceCulling(workingVB, faces, obj);

		/////////////////////////////////////////////////
		///////// 世界空间逐顶点光照
		g_env.renderer->GetCurRas()->DoPerVertexLighting(workingVB, faces, obj);

		//transform each vertex
		for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
		{
			SVertex& vert = workingVB[iVert];

			if(!vert.bActive)
				continue;

			VEC4& vertPos = vert.pos;

			/////////////////////////////////////////////////
			///////// 世界变换
			Common::Transform_Vec4_By_Mat44(vertPos, vertPos, obj.m_matWorld);

			//保存世界坐标
			vert.worldPos = vertPos;
			//保存世界法线
			vert.worldNormal = Common::Transform_Vec3_By_Mat44(vert.normal, obj.m_matWorldIT, false).GetVec3();

			/////////////////////////////////////////////////
			///////// 相机变换
			auto matView = cam->GetViewMatrix();
			Common::Transform_Vec4_By_Mat44(vertPos, vertPos, matView);

			vert.viewSpaceZ = vertPos.z;
		}

		/////////////////////////////////////////////////
		///////// 相机空间进行三角面级别的3D裁减
		///////// 必须在透视变换之前,不然小于近裁面的顶点会被反转
		Do3DClipping(workingVB, faces);

		//继续顶点变换流水线..
		for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
		{
			SVertex& vert = workingVB[iVert];

			if(!vert.bActive)
				continue;

			VEC4& vertPos = vert.pos;

			/////////////////////////////////////////////////
			///////// 透视投影变换
			auto matProj = cam->GetProjMatrix();
			Common::Transform_Vec4_By_Mat44(vertPos, vertPos, matProj);

			/////////////////////////////////////////////////
			///////// 齐次除法
			vertPos.w = 1 / vertPos.w;
			vertPos.x *= vertPos.w;
			vertPos.y *= vertPos.w;
			vertPos.z *= vertPos.w;

			/////////////////////////////////////////////////
			///////// 视口映射 [-1,1] -> [0, Viewport W/H]
			float a = 0.5f * SCREEN_WIDTH;
			float b = 0.5f *SCREEN_HEIGHT;

			vertPos.x = a + a * vertPos.x;
			vertPos.y = b - b * vertPos.y;
		}
	}

	void RenderUtil::DrawText( float x, float y, const STRING& text, const Gdiplus::Color& color )
	{
		static bool bInit = false;
		static Gdiplus::Graphics g(g_env.renderer->m_bmBackBuffer.get());
		static Gdiplus::Font font(L"Arial", 8);
		static Gdiplus::StringFormat format;

		if(!bInit)
		{
			format.SetAlignment(Gdiplus::StringAlignmentNear);
			bInit = true;
		}

		Gdiplus::SolidBrush bru(color);
		Gdiplus::PointF origin(x, y);
		std::wstring wstr = Ext::AnsiToUnicode(text.c_str());

		g.DrawString(wstr.c_str(), -1, &font, origin, &format, &bru);
	}

	void RenderUtil::ComputeAABB( RenderObject& obj )
	{
		AABB& aabb = obj.m_localAABB;
		const VertexBuffer& verts = obj.m_verts;

		//AABB start with the first pos
		aabb.m_minCorner = verts[0].pos.GetVec3();
		aabb.m_maxCorner = verts[0].pos.GetVec3();

		for (size_t i=1; i<verts.size(); ++i)
		{
			aabb.Merge(verts[i].pos);
		}
	}

	void RenderUtil::SortTris_PainterAlgorithm( const VertexBuffer& verts, FaceList& faces )
	{
		//用画家算法对所有面进行排序(根据三角面3个顶点的平均z值)
		//NB: 该算法在某些面重叠的情况下是不正确的,见截图Compare.jpg
		std::sort(faces.begin(), faces.end(), [&](const SFace& face1, const SFace& face2)->bool
		{
			const Index idx1 = face1.index1;
			const Index idx2 = face1.index2;
			const Index idx3 = face1.index3;

			float z1 = verts[idx1].pos.z + verts[idx2].pos.z + verts[idx3].pos.z;
			z1 *= 0.33333f;

			const Index idx4 = face2.index1;
			const Index idx5 = face2.index2;
			const Index idx6 = face2.index3;

			float z2 = verts[idx4].pos.z + verts[idx5].pos.z + verts[idx6].pos.z;
			z2 *= 0.33333f;

			return z1 > z2;
		});
	}

	void RenderUtil::DoLambertLighting( SColor& result, const VEC3& Normal, const VEC3& lightDir, const SMaterial* pMaterial )
	{
		float nl = Common::DotProduct_Vec3_By_Vec3(Normal, lightDir);

		//use half-lambert?
		if(pMaterial->bUseHalfLambert)
		{
			nl = nl * 0.5f + 0.5f;
			//square
			nl *= nl;
			result = pMaterial->diffuse * nl;
		}
		else
		{
			result = SColor::BLACK;
			if(nl > 0)
			{
				result = pMaterial->diffuse * nl;
			}
		}

		//环境光
		result += pMaterial->ambient;
	}
}