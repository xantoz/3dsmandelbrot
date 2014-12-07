#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <font.h>
#include <math.h>
#include <complex.h>
#include <tgmath.h>


/****START OF CONFIGURATION****/
#define real double
#define NORMAL            0
#define NICE_COLORS       1
#define SILLY_3D          2
#define MODE NICE_COLORS
/****END OF CONFIGURATION****/
#if MODE == SILLY_3D
#define USE_3D 1
#else
#define USE_3D 0
#endif

#define max(a,b)                                \
    ({ __typeof__ (a) _a = (a);                 \
       __typeof__ (b) _b = (b);                 \
       _a > _b ? _a : _b; })

#define min(a,b)                                \
    ({ __typeof__ (a) _a = (a);                 \
       __typeof__ (b) _b = (b);                 \
       _a < _b ? _a : _b; })



//will be moved into ctrulib at some point
#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

#define RGBA8(r,g,b,a) ((((r)&0xFF)<<24) | (((g)&0xFF)<<16) | (((b)&0xFF)<<8) | (((a)&0xFF)<<0))
#define RGB8(r,g,b) ((((r)&0xFF)<<16) | (((g)&0xFF)<<8) | (((b)&0xFF)<<0))

u8* TopFB;
u8* TopRFB;
u8* BottomFB;

static inline u32 xGetPixel(int x,int y,int screen)
{
    int idx = ((x)*240) + (239-(y));
    if (screen == 0)
    {
        return RGB8(TopFB[idx*3+2], TopFB[idx*3+1], TopFB[idx*3+0]);
    }
    else if (screen == 1)
    {
        return RGB8(TopRFB[idx*3+2], TopRFB[idx*3+1], TopRFB[idx*3+0]);
    }
    else
    {
        return RGB8(BottomFB[idx*3+2], BottomFB[idx*3+1], BottomFB[idx*3+0]);
    }
}


static inline void xDrawPixel(int x,int y,u32 color,int screen)
{
    int idx = ((x)*240) + (239-(y));
    if (screen == 0)
    {
        TopFB[idx*3+0] = (color);
        TopFB[idx*3+1] = (color) >> 8;
        TopFB[idx*3+2] = (color) >> 16;
    }
    else if (screen == 1)
    {
        TopRFB[idx*3+0] = (color);
        TopRFB[idx*3+1] = (color) >> 8;
        TopRFB[idx*3+2] = (color) >> 16;
    }
    else
    {
        BottomFB[idx*3+0] = (color);
        BottomFB[idx*3+1] = (color) >> 8;
        BottomFB[idx*3+2] = (color) >> 16;
    }
}

static inline void DrawPixel(int x,int y,u32 color,int screen)
{
    if (x < 0 || ((screen == 0 || screen == 1) ? x >= 400 : x >= 320) || y < 0 || y >= 240)
        return;
    xDrawPixel(x, y, color, screen);
}


#define DrawXBM(x, y, picture, color, invert, screen) (_DrawXBM((x), (y), picture##_bits, picture##_width, picture##_height, (color), (invert), (screen)))

void _DrawXBM(int x, int y, const char* bits, u16 width, u16 height, u32 color, char invert, int screen)
{
    u16 idx, cy, cx;
    idx = 0;
    for (cy = 0; cy < height; ++cy)
    {
        for (cx = 0; cx < width; ++cx)
        {
            if (((bits[idx >> 3] >> (idx & 7)) & 1) ^ invert)
                DrawPixel(x+cx, y+cy, color, screen);
            ++idx;
        }
    }
}

void RefreshScreen()
{
    TopFB = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    if (USE_3D)
        TopRFB = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
    BottomFB = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
}

void ClearScreen()
{
    memset(TopFB, 0, 240*400*3);
    if (USE_3D)
        memset(TopRFB, 0, 240*400*3);
    memset(BottomFB, 0, 240*320*3);

}

void DrawText(int x, int y, char* str, u32 color, int screen){
    unsigned short* ptr;
    unsigned short glyphsize;
    int i, cx, cy;
    for (i = 0; str[i] != '\0'; i++)
    {
        if (str[i] < 0x21)
        {
            x += 6;
            continue;
        }
        u16 ch = str[i];
        if (ch > 0x7E) ch = 0x7F;
        ptr = &font[(ch-0x20) << 4];
        glyphsize = ptr[0];
        if (!glyphsize)
        {
            x += 6;
            continue;
        }
        x++;
        for (cy = 0; cy < 12; cy++)
        {
            unsigned short val = ptr[4+cy];
            for (cx = 0; cx < glyphsize; cx++)
            {
                if (val & (1 << cx))
                    DrawPixel(x+cx, y+cy, color, screen);
            }
        }
        x += glyphsize;
        x++;
    }
}


int main()
{
	// Initialize services
	srvInit();
	aptInit();
	hidInit(NULL);
	gfxInit();

    if (USE_3D)
        gfxSet3D(true); 

    touchPosition tposd;
    touchPosition tposu;
    /* touchPosition *tpos = &tposd; */

    u16 w = 400;
    u16 h = 240;
    u16 x = 0;
    u16 y = 0;
    
    real east = -2.5;
    real north = 1.0;
    real west = 1.0;
    real south = -1.0;

    u64 t0 = 0, t1 = 0;
    /* double fps = 0.0; */

    RefreshScreen();
    ClearScreen();
    gfxFlushBuffers();
    gfxSwapBuffers();

	// Main loop
    t0 = osGetTime();
	while (aptMainLoop())
	{
		hidScanInput();

		// Your code goes here

		u32 kDown = hidKeysDown();
        u32 kUp   = hidKeysUp();
        u32 kHeld = hidKeysHeld();
        
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

        if (kDown & KEY_A)
        {
            RefreshScreen();
            ClearScreen();
            gfxFlushBuffers();
            gfxSwapBuffers();
            
            east = -2.5;
            north = 1.0;
            west = 1.0;
            south = -1.0;
            x = 0;
            t0 = osGetTime();
        }

        if (kDown & KEY_TOUCH)
            hidTouchRead(&tposd);

        if (kHeld & KEY_TOUCH)
            hidTouchRead(&tposu);

        if (kUp & KEY_TOUCH)
        {
            RefreshScreen();
            ClearScreen();
            gfxFlushBuffers();
            gfxSwapBuffers();

            west  = ((real)(max(tposd.px, tposu.px)+40)/w)*fabs(west - east) + east;
            north = ((real)max(tposd.py, tposu.py)/h)*fabs(north - south) + south;
            east  = ((real)(min(tposd.px, tposu.px)+40)/w)*fabs(west - east) + east;
            south = ((real)min(tposd.py, tposu.py)/h)*fabs(north - south) + south;

            x = 0;
            t0 = osGetTime();
        }

        /* for (unsigned x = 0; x < w; ++x) */
        if (x < w)
        {
            real x0 = ((real)x/w)*fabs(west - east) + east;
            for (y = 0; y < h; ++y)
            {
                real y0 = ((real)y/h)*fabs(north - south) + south;
                real complex c = x0 + y0*I;
                real complex z = 0;

                if (MODE == NORMAL)
                {
                    u8 iter = 0;
                    for (; iter != 255; ++iter)
                    {
                        if (creal(z)*creal(z) + cimag(z)*cimag(z) >= 4.0) break;
                        z = z*z + c;
                    }
                    u32 color = RGB8(iter, iter, iter);
                    xDrawPixel(x, y, color, 0);
                    if (x >= 40 && x < 360) xDrawPixel(x-40, y, color, 2);
                }
                else if (MODE == NICE_COLORS)
                {
                
                    u16 iter = 0;
                    for (; iter != 2048; ++iter)
                    {
                        if (creal(z)*creal(z) + cimag(z)*cimag(z) >= 4.0) break;
                        z = z*z + c;
                    }
                    u32 color = RGB8((iter>>8)&0xFF, 0, iter&0xFF);
                    xDrawPixel(x, y, color, 0);
                    if (x >= 40 && x < 360) xDrawPixel(x-40, y, color, 2);
                }
                else if (MODE == SILLY_3D)
                {
                    u8 iter = 0;
                    for (; iter != 255; ++iter)
                    {
                        if (creal(z)*creal(z) + cimag(z)*cimag(z) >= 4.0) break;
                        z = z*z + c;
                    }
                    float shift = logf(iter)/2.0f;
                    DrawPixel((int)((x+shift)+0.5f), y, RGB8(iter, iter, iter), 0);
                    DrawPixel((int)((x-shift)+0.5f), y, RGB8(iter, iter, iter), 1);
                    DrawPixel(x-40, y, RGB8(iter, iter, iter), 2);
                }
            }
            
            ++x;
            gfxFlushBuffers();
        }
        else if (x == w)
        {
            t1 = osGetTime();
            char str[256];
            snprintf(str, 255, "Done in %f seconds", (t1 - t0)/1000.0);
            DrawText(0, 0, str, 0x00FF0033, 2);
            snprintf(str, 255, "enws: %.3g %.3g %.3g %.3g", east, north, west, south);
            DrawText(0, 15, str, 0x00FF0033, 2);
            ++x;
            gfxFlushBuffers();
        }

        const u32 marker_color = 0xFFFFFF;
        if (kHeld & KEY_TOUCH)
        {
            u16 ux  = max(tposd.px, tposu.px);
            u16 uy = max(tposd.py, tposu.py);
            u16 lx  = min(tposd.px, tposu.px);
            u16 ly = min(tposd.py, tposu.py);
            xDrawPixel(ux, uy, marker_color ^ xGetPixel(ux, uy, 2), 2);
            xDrawPixel(ux, ly, marker_color ^ xGetPixel(ux, ly, 2), 2);
            xDrawPixel(lx, uy, marker_color ^ xGetPixel(lx, uy, 2), 2);
            xDrawPixel(lx, ly, marker_color ^ xGetPixel(lx, ly, 2), 2);
            /* for (u16 x = ux; x <= lx; ++x) */
            /* { */
            /*     xDrawPixel(x, uy, marker_color ^ xGetPixel(x, uy, 2), 2); */
            /*     xDrawPixel(x, ly, marker_color ^ xGetPixel(x, ly, 2), 2); */
            /* } */
            /* for (u16 y = uy; y <= ly; ++y) */
            /* { */
            /*     xDrawPixel(ux, y, marker_color ^ xGetPixel(ux, y, 2), 2); */
            /*     xDrawPixel(lx, y, marker_color ^ xGetPixel(lx, y, 2), 2); */
            /* } */
            gfxFlushBuffers();
        }
        if (kHeld & KEY_TOUCH)
        {
            u16 ux  = max(tposd.px, tposu.px);
            u16 uy = max(tposd.py, tposu.py);
            u16 lx  = min(tposd.px, tposu.px);
            u16 ly = min(tposd.py, tposu.py);
            xDrawPixel(ux, uy, marker_color ^ xGetPixel(ux, uy, 2), 2);
            xDrawPixel(ux, ly, marker_color ^ xGetPixel(ux, ly, 2), 2);
            xDrawPixel(lx, uy, marker_color ^ xGetPixel(lx, uy, 2), 2);
            xDrawPixel(lx, ly, marker_color ^ xGetPixel(lx, ly, 2), 2);
            /* for (u16 x = ux; x <= lx; ++x) */
            /* { */
            /*     xDrawPixel(x, uy, marker_color ^ xGetPixel(x, uy, 2), 2); */
            /*     xDrawPixel(x, ly, marker_color ^ xGetPixel(x, ly, 2), 2); */
            /* } */
            /* for (u16 y = uy; y <= ly; ++y) */
            /* { */
            /*     xDrawPixel(ux, y, marker_color ^ xGetPixel(ux, y, 2), 2); */
            /*     xDrawPixel(lx, y, marker_color ^ xGetPixel(lx, y, 2), 2); */
            /* } */
        }

        /* if (kHeld & KEY_TOUCH) */
        /* { */
        /*     u16 ux  = max(tposd.px, tposu.px); */
        /*     u16 uy = max(tposd.py, tposu.py); */
        /*     u16 lx  = min(tposd.px, tposu.px); */
        /*     u16 ly = min(tposd.py, tposu.py); */
        /*     for (u16 x = ux; x <= lx; ++x) */
        /*     { */
        /*         xDrawPixel(x, uy, RGB8(255,255,255) ^ xGetPixel(x, uy, 2), 2); */
        /*         xDrawPixel(x, ly, RGB8(255,255,255) ^ xGetPixel(x, uy, 2), 2); */
        /*     } */
        /*     for (u16 y = uy; y <= ly; ++y) */
        /*     { */
        /*         xDrawPixel(ux, y, RGB8(255,255,255) ^ xGetPixel(x, uy, 2), 2); */
        /*         xDrawPixel(lx, y, RGB8(255,255,255) ^ xGetPixel(x, uy, 2), 2); */
        /*     } */
        /* } */
	}

	// Exit services
	gfxExit();
	hidExit();
	aptExit();
	srvExit();
	return 0;
}
