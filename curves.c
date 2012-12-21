#include <string.h>
#include <sparrow3d.h>

//#define SCALE_UP
spFontPointer font = NULL;
#ifdef SCALE_UP
SDL_Surface* real_screen;
#endif
SDL_Surface* screen = NULL;
#define CLOUD_COUNT 16

typedef struct sPoint *pPoint;
typedef struct sPoint
{
	float x,y,z;
	pPoint next;
} tPoint;

typedef enum eType
{
	lagrange = 0
} tType;

tType type = lagrange;

pPoint firstPoint = NULL;
pPoint  lastPoint = NULL;

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

Sint32 rotation = 0;

void draw(void)
{
	Sint32* modellViewMatrix=spGetMatrix();

	spClearTarget(spGetRGB(0,0,32));
	spResetZBuffer();
	spIdentity();
	spTranslate(0,0,-10<<SP_ACCURACY);
	
	spRotateX(rotation);
	spRotateY(rotation>>1);
	spRotateZ(rotation>>2);
	
	spLine3D(0,0,0,1<<SP_ACCURACY,0,0,spGetFastRGB(255,0,0));
	spLine3D(0,0,0,0,1<<SP_ACCURACY,0,spGetFastRGB(0,255,0));
	spLine3D(0,0,0,0,0,1<<SP_ACCURACY,spGetFastRGB(0,0,255));
	
	pPoint point = firstPoint;
	
	while (point)
	{
		spEllipse3D(spFloatToFixed(point->x),spFloatToFixed(point->y),spFloatToFixed(point->z),1<<SP_ACCURACY-4,1<<SP_ACCURACY-4,spGetFastRGB(255,255,0));
		point = point->next;
	}

	spFontDrawMiddle(screen->w>>1,screen->h-font->maxheight-2,-1,"Press [R] to quit",font);
	#ifdef SCALE_UP
	spScale2XSmooth(screen,real_screen);
	#endif
	spFlip();
}

int calc(Uint32 steps)
{
	rotation += steps*32;
	PspInput engineInput = spGetInput();
	if (engineInput->button[SP_BUTTON_START])
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	printf("Use curves.sh kind control-point-1 control-point-2 [control-point-3..n]\n");
	printf("* kind can be \"lagrange\".\n");
	printf("* a point has to have this form: {x,y,z} with x,y,z floating points numbers.\n");
	int i;
	if (argc < 8)
	{
		printf("Too few arguments.\n");
		return 1;
	}
	if (strcmp(argv[1],"lagrange") == 0)
		type = lagrange;
	else
	{
		printf("Unknown argument %s.\n",argv[1]);
		return 1;
	}
	if ((argc-2) % 3 != 0)
	{
		printf("Every point needs 3 floats!\n");
		return 1;
	}
	for (i = 2; i < argc; i+=3)
	{
		pPoint point = (pPoint)malloc(sizeof(tPoint));
		point->x = atof(argv[i+0]);
		point->y = atof(argv[i+1]);
		point->z = atof(argv[i+2]);
		point->next = NULL;
		if (lastPoint)
			lastPoint->next = point;
		else
			firstPoint = point;
		lastPoint = point;
		printf("Added point ( %6.2f | %6.2f | %6.2f )\n",point->x,point->y,point->z);
	}
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
