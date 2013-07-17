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
		PixelBox(BITMAP* bm, bool bCopyData);
		~PixelBox();

	public:
		inline void*	GetDataPointer()	{ return (void*)m_data; }
		inline int		GetWidth() const	{ return m_width; }
		inline int		GetHeight() const	{ return m_height; }
		inline int		GetPitch() const	{ return m_pitch; }
		inline int		GetBitsPerPixel() const	{ return m_bytesPerPixel * 8; }
		inline int		GetBytesPerPixel() const	{ return m_bytesPerPixel; }

	private:
		char*		m_data;
		bool		m_ownData;			//数据是否是自己分配的.是则需要自己销毁,否则不用
		int			m_width;
		int			m_height;
		int			m_pitch;
		int			m_bytesPerPixel;
	};
}


#endif // PixelBox_h__