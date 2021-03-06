#include "io.h"
#ifdef VIDEO_GDI
#define video cvideo
#undef bind
#include <windows.h>
#define bind bind_func

#define this This

//this file is based on ruby by byuu

struct video_gdi {
	struct video i;
	
	unsigned int screenwidth;
	unsigned int screenheight;
	
	HDC maindc;
	HDC bitmapdc;
	
	unsigned int bmpwidth;
	unsigned int bmpheight;
	void* bmppixels;
	unsigned int bmppitch;
	HBITMAP bitmap;
	
	videoformat depth;
};

static void reinit(struct video * this_, unsigned int screen_width, unsigned int screen_height, videoformat depth, double fps)
{
	struct video_gdi * this=(struct video_gdi*)this_;
	
	this->screenwidth=screen_width;
	this->screenheight=screen_height;
	
	this->depth=depth;
	
	if (this->bitmap) DeleteObject(this->bitmap);
	this->bitmap=NULL;
	
	this->bmpwidth=0;
	this->bmpheight=0;
	
	free(this->bmppixels);
	this->bmppixels=NULL;
}

static void draw(struct video * this_, unsigned int width, unsigned int height, const void * data, unsigned int pitch)
{
	struct video_gdi * this=(struct video_gdi*)this_;
	
	if (!data) return;
	
	if (width>this->bmpwidth || height>this->bmpheight)
	{
		if (width>this->bmpwidth) this->bmpwidth=width;
		if (height>this->bmpheight) this->bmpheight=height;
		if (this->bitmap) DeleteObject(this->bitmap);
		
		this->bitmap=CreateCompatibleBitmap(this->maindc, this->bmpwidth, this->bmpheight);
		SelectObject(this->bitmapdc, this->bitmap);
		
		if (this->depth==15)
		{
			this->bmppitch=(sizeof(uint16_t)*this->bmpwidth + 2) & ~3;
		}
		else
		{
			this->bmppitch=sizeof(uint32_t)*this->bmpwidth;
		}
		
		if (this->bmppixels) free(this->bmppixels);
		this->bmppixels=malloc(this->bmppitch*this->bmpheight);
	}
	
	struct image src;
	src.width=width;
	src.height=height;
	src.pixels=(void*)data;
	src.pitch=pitch;
	
	struct image dst;
	dst.width=width;
	dst.height=height;
	dst.pixels=this->bmppixels;
	dst.pitch=this->bmppitch;
	
	if (this->depth==fmt_xrgb1555)
	{
		src.format=fmt_xrgb1555;
		dst.format=fmt_xrgb1555;
	}
	if (this->depth==fmt_rgb565)
	{
		src.format=fmt_rgb565;
		dst.format=fmt_xrgb8888;
	}
	if (this->depth==fmt_xrgb8888)
	{
		src.format=fmt_xrgb8888;
		dst.format=fmt_xrgb8888;
	}
	
	//memset(this->pixels,255,this->pitch*this->screenheight);
	image_convert(&src, &dst);
	
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = width;
	bmi.bmiHeader.biHeight      = -height;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = (this->depth==15)?16:32; //biBitCount of 15 is invalid, biBitCount of 16 is really RGB555
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage   = 0;
	
	SetDIBits(this->bitmapdc, this->bitmap, 0, height, (void*)this->bmppixels, &bmi, DIB_RGB_COLORS);
	StretchBlt(this->maindc, 0,0, this->screenwidth,this->screenheight,  this->bitmapdc, 0,0, width,height,  SRCCOPY);
}

static bool set_sync(struct video * this_, bool sync)
{
	//can't do this
	return false;
}

static bool has_sync(struct video * this_)
{
	return false;
}

static void free_(struct video * this_)
{
	struct video_gdi * this=(struct video_gdi*)this_;
	
	if (this->bitmap) DeleteObject(this->bitmap);
	if (this->bitmapdc) DeleteDC(this->bitmapdc);
	ReleaseDC(WindowFromDC(this->maindc), this->maindc);
	free(this->bmppixels);
	
	free(this);
}

struct video * cvideo_create_gdi(uintptr_t windowhandle, unsigned int screen_width, unsigned int screen_height,
                                   videoformat depth, double fps)
{
	struct video_gdi * this=malloc(sizeof(struct video_gdi));
	this->i.reinit=reinit;
	this->i.draw=draw;
	this->i.set_sync=set_sync;
	this->i.has_sync=has_sync;
	this->i.free=free_;
	
	this->maindc=NULL;
	this->bitmapdc=NULL;
	this->bitmap=NULL;
	this->bmppixels=NULL;
	
	this->maindc=GetDC((HWND)windowhandle);
	this->bitmapdc=CreateCompatibleDC(this->maindc);
	if (!this->bitmapdc) goto cancel;
	
	reinit((struct video*)this, screen_width, screen_height, depth, fps);
	
	return (struct video*)this;
	
cancel:
	free_((struct video*)this);
	return NULL;
}

#undef video
static video* video_create_gdi(uintptr_t windowhandle)
{
	return video_create_compat(cvideo_create_gdi(windowhandle, 256, 256, fmt_xrgb1555, 60));
}
const video::driver video::driver_gdi = {"GDI", video_create_gdi, NULL, 0};
#endif
