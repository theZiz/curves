#include <string.h>
#include <sparrow3d.h>

//#define SCALE_UP
spFontPointer font = NULL;
#ifdef SCALE_UP
SDL_Surface* real_screen;
#endif
SDL_Surface* screen = NULL;
#define CLOUD_COUNT 16

void resize( Uint16 w, Uint16 h )
{
	#ifdef SCALE_UP
	if (screen)
		spDeleteSurface(screen);
	screen = spCreateSurface(real_screen->w/2,real_screen->h/2);
	#endif
	spSelectRenderTarget(screen);
	//Setup of the new/resized window
	spSetPerspective( 50.0, ( float )screen->w / ( float )screen->h, 0.1, 100 );

	int scale = 0;
	#ifdef SCALE_UP
	scale++;
	#endif
	//Font Loading
	if ( font )
		spFontDelete( font );
	font = spFontLoad( "./data/LondrinaOutline-Regular.ttf", 10 * spGetSizeFactor() >> SP_ACCURACY+scale );
	spFontAdd( font, SP_FONT_GROUP_ASCII, 65535 ); //whole ASCII
	spFontAddButton( font, 'R', SP_BUTTON_START_NAME, 65535, SP_ALPHA_COLOR ); //Return == START
	spFontAddButton( font, 'B', SP_BUTTON_SELECT_NAME, 65535, SP_ALPHA_COLOR ); //Backspace == SELECT
	spFontAddButton( font, 'q', SP_BUTTON_L_NAME, 65535, SP_ALPHA_COLOR ); // q == L
	spFontAddButton( font, 'e', SP_BUTTON_R_NAME, 65535, SP_ALPHA_COLOR ); // e == R
	spFontAddButton( font, 'a', SP_BUTTON_LEFT_NAME, 65535, SP_ALPHA_COLOR ); //a == left button
	spFontAddButton( font, 'd', SP_BUTTON_RIGHT_NAME, 65535, SP_ALPHA_COLOR ); // d == right button
	spFontAddButton( font, 'w', SP_BUTTON_UP_NAME, 65535, SP_ALPHA_COLOR ); // w == up button
	spFontAddButton( font, 's', SP_BUTTON_DOWN_NAME, 65535, SP_ALPHA_COLOR ); // s == down button
	
	spSelectRenderTarget(screen);
}

void draw(void)
{
	Sint32* modellViewMatrix=spGetMatrix();

	spClearTarget(spGetRGB(0,0,32));
	spResetZBuffer();
	spIdentity();

	spFontDrawMiddle(screen->w>>1,screen->h-font->maxheight-2,-1,"Press [R] to quit",font);
	#ifdef SCALE_UP
	spScale2XSmooth(screen,real_screen);
	#endif
	spFlip();
}

int calc(Uint32 steps)
{
	PspInput engineInput = spGetInput();
	if (engineInput->button[SP_BUTTON_START])
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	spSetDefaultWindowSize( 800, 480 );
	spInitCore();
	//Setup
	#ifdef SCALE_UP
	real_screen = spCreateDefaultWindow();
	resize( real_screen->w, real_screen->h );
	#else
	screen = spCreateDefaultWindow();
	resize( screen->w, screen->h );
	#endif
	spLoop(draw,calc,10,resize,NULL);
	#ifdef SCALE_UP
	spDeleteSurface(screen);
	#endif
	spQuitCore();
	return 0;
}
