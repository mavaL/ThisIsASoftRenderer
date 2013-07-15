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

		pData = new Common::PixelBox(&bitmap);
		texName = filename;
	}

}