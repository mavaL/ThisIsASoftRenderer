#include "stdafx.h"
#include "PixelBox.h"

namespace Common
{
	PixelBox::PixelBox( int width, int height, int bytesPerPixel )
	:m_width(width)
	,m_height(height)
	,m_bytesPerPixel(bytesPerPixel)
	{
		m_data.resize(width * height * bytesPerPixel);
	}


	void* PixelBox::GetDataPointer()
	{
		return &m_data[0];
	}
}

