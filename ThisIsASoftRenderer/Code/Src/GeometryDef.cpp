#include "stdafx.h"
#include "GeometryDef.h"

namespace SR
{
	SColor SColor::WHITE	=	SColor(0xffffffff);
	SColor SColor::BLACK	=	SColor(0);
	SColor SColor::BLUE		=	SColor(0xff0000ff);

	STexture::STexture( const STexture& rhs )
		:texName("")
		,pData(nullptr)
	{
		*this = rhs;
	}

	STexture& STexture::operator=( const STexture& rhs )
	{
		SAFE_DELETE(pData);
		if(!rhs.texName.empty())
			LoadTexture(rhs.texName);
		return *this;
	}

	void STexture::LoadTexture( const STRING& filename )
	{
		assert(texName.empty() && !pData);
		Gdiplus::Bitmap bm(Ext::AnsiToUnicode(filename.c_str()).c_str(), TRUE);
		HBITMAP hbm;
		bm.GetHBITMAP(Gdiplus::Color::Black, &hbm);

		CBitmap* cbm;
		BITMAP bitmap;
		cbm = CBitmap::FromHandle(hbm);
		cbm->GetBitmap(&bitmap);

		pData = new Common::PixelBox(&bitmap, true);
		texName = filename;
	}

	void STexture::Tex2D_Point( VEC2& uv, SColor& ret ) const
	{
		DWORD* pTexData = (DWORD*)pData->GetDataPointer();

		//Wrap mode
		uv.x -= Ext::Floor32_Fast(uv.x);
		uv.y -= Ext::Floor32_Fast(uv.y);

		int w = pData->GetWidth() - 1;
		int h = pData->GetHeight() - 1;

		int x = Ext::Ftoi32_Fast(uv.x * w);
		int y = Ext::Ftoi32_Fast(uv.y * h);

		ret.color = pTexData[y * pData->GetWidth() + x];
	}

	void STexture::Tex2D_Bilinear( VEC2& uv, SColor& ret ) const
	{
		DWORD* pTexData = (DWORD*)pData->GetDataPointer();

		//Wrap mode
		uv.x -= Ext::Floor32_Fast(uv.x);
		uv.y -= Ext::Floor32_Fast(uv.y);

		float intU = (pData->GetWidth() - 1) * uv.x;
		float intV = (pData->GetHeight() - 1) * uv.y;

		int u_l = Ext::Floor32_Fast(intU);
		int u_r = Ext::Ceil32_Fast(intU);
		int v_t = Ext::Floor32_Fast(intV);
		int v_b = Ext::Ceil32_Fast(intV);

		static SColor color[4];
		color[0].color = pTexData[v_t * pData->GetWidth() + u_l];
		color[1].color = pTexData[v_t * pData->GetWidth() + u_r];
		color[2].color = pTexData[v_b * pData->GetWidth() + u_l];
		color[3].color = pTexData[v_b * pData->GetWidth() + u_r];

		float frac_u = intU - u_l;
		float frac_v = intV - v_t;

		float weight[4];
		weight[0] = (1 - frac_u)	* (1 - frac_v);
		weight[1] = frac_u			* (1 - frac_v);
		weight[2] = (1 - frac_u)	* frac_v;
		weight[3] = frac_u			* frac_v;

		ret = SColor::BLACK;
		//blend the four neighbour colors
		for (int i=0; i<4; ++i)
		{
			ret.a += Ext::Ftoi32_Fast(weight[i] * color[i].a);
			ret.r += Ext::Ftoi32_Fast(weight[i] * color[i].r);
			ret.g += Ext::Ftoi32_Fast(weight[i] * color[i].g);
			ret.b += Ext::Ftoi32_Fast(weight[i] * color[i].b);
		}
	}

}