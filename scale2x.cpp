/*
   This implements the AdvanceMAME Scale2x feature found on this page,
   http://advancemame.sourceforge.net/scale2x.html
   
   It is an incredibly simple and powerful image doubling routine that does
   an astonishing job of doubling game graphic data while interpolating out
   the jaggies. Congrats to the AdvanceMAME team, I'm very impressed and
   surprised with this code!
*/



#include <SDL.h>
#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif


#define READINT24(x)      ((x)[0]<<16 | (x)[1]<<8 | (x)[2]) 
#define WRITEINT24(x, i)  {(x)[0]=i>>16; (x)[1]=(i>>8)&0xff; x[2]=i&0xff; }

/*
  this requires a destination surface already setup to be twice as
  large as the source. oh, and formats must match too. this will just
  blindly assume you didn't flounder.
*/

void scale2x(SDL_Surface *src, SDL_Surface *dst){
    int looph, loopw;
	
    Uint8* srcpix = (Uint8*)src->pixels;
    Uint8* dstpix = (Uint8*)dst->pixels;

    const int srcpitch = src->pitch;
    const int dstpitch = dst->pitch;
    const int width = src->w;
    const int height = src->h;

    Uint16 E0, E1, E2, E3, B, D, E, F, H;
    for(looph = 0; looph < height; ++looph)
    {
        for(loopw = 0; loopw < width; ++ loopw)
        {
            B = *(Uint16*)(srcpix + (MAX(0,looph-1)*srcpitch) + (2*loopw));
            D = *(Uint16*)(srcpix + (looph*srcpitch) + (2*MAX(0,loopw-1)));
            E = *(Uint16*)(srcpix + (looph*srcpitch) + (2*loopw));
            F = *(Uint16*)(srcpix + (looph*srcpitch) + (2*MIN(width-1,loopw+1)));
            H = *(Uint16*)(srcpix + (MIN(height-1,looph+1)*srcpitch) + (2*loopw));
				
            E0 = D == B && B != F && D != H ? D : E;
            E1 = B == F && B != D && F != H ? F : E;
            E2 = D == H && D != B && H != F ? D : E;
            E3 = H == F && D != H && B != F ? F : E;

            *(Uint16*)(dstpix + looph*2*dstpitch + loopw*2*2) = E0;
            *(Uint16*)(dstpix + looph*2*dstpitch + (loopw*2+1)*2) = E1;
            *(Uint16*)(dstpix + (looph*2+1)*dstpitch + loopw*2*2) = E2;
            *(Uint16*)(dstpix + (looph*2+1)*dstpitch + (loopw*2+1)*2) = E3;
        }
    }
}

