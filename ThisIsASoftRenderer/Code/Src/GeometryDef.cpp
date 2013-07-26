#include "stdafx.h"
#include "GeometryDef.h"

namespace SR
{
	SColor SColor::WHITE	=	SColor(0xffffffff);
	SColor SColor::BLACK	=	SColor(0);

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

	SR::SColor STexture::Tex2D_Point( VEC2& uv ) const
	{
		DWORD* pTexData = (DWORD*)pData->GetDataPointer();

		int w = pData->GetWidth() - 1;
		int h = pData->GetHeight() - 1;

		//Wrap mode
		uv.x -= Ext::Floor32_Fast(uv.x);
		uv.y -= Ext::Floor32_Fast(uv.y);

		int x = Ext::Ftoi32_Fast(uv.x * w);
		int y = Ext::Ftoi32_Fast(uv.y * h);

		return std::move(SColor(pTexData[y * pData->GetWidth() + x]));
	}

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
}