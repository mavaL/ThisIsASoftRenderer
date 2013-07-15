/********************************************************************
	created:	2:7:2013   20:13
	filename	PixelBox.h
	author:		maval

	purpose:	PixelBox是包含一个数据块的数据结构
*********************************************************************/
#ifndef PixelBox_h__
#define PixelBox_h__

namespace Common
{
	class PixelBox
	{
	public:
		PixelBox(int width, int height, int bytesPerPixel);
		PixelBox(BITMAP* bm);

		typedef std::vector<char>	DataBuffer;	

	public:
		void*		GetDataPointer();
		int			GetWidth() const	{ return m_width; }
		int			GetHeight() const	{ return m_height; }
		int			GetPitch() const	{ return m_pitch; }
		int			GetBitsPerPixel() const	{ return m_bytesPerPixel * 8; }
		int			GetBytesPerPixel() const	{ return m_bytesPerPixel; }

	private:
		DataBuffer	m_data;
		int			m_width;
		int			m_height;
		int			m_pitch;
		int			m_bytesPerPixel;
	};
}


#endif // PixelBox_h__