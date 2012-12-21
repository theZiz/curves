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
	float x,y,z,u;
	pPoint next;
} tPoint;

typedef enum eType
{
	lagrange = 0
} tType;

tType type = lagrange;

pPoint firstPoint = NULL;
pPoint  lastPoint = NULL;
pPoint* pointArray;
int pointCount;

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

float L_g_i(float t,int g,int i)
{
	int k;
	float result = 1.0f;
	for (k = 0; k <= g; k++)
	{
		if (k == i)
			continue;
		result *= ((float)(t)-pointArray[k]->u)/(pointArray[i]->u-pointArray[k]->u);
	}
	return result;
}

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
	//control polygon
	int i;
	for (i = 1; i < pointCount; i++)
		spLine3D(spFloatToFixed(pointArray[i-1]->x),spFloatToFixed(pointArray[i-1]->y),spFloatToFixed(pointArray[i-1]->z),
						 spFloatToFixed(pointArray[i  ]->x),spFloatToFixed(pointArray[i  ]->y),spFloatToFixed(pointArray[i  ]->z),spGetFastRGB(127,127,127));

  //curve
  float t;
  tPoint prev;
  switch (type)
  {
		case lagrange:
			for (t = 0.0f; t <= (float)((pointCount-1)*2-1); t+=0.01f)
			{
				tPoint mom = {0.0f,0.0f,0.0f};
				for (i = 0; i < pointCount; i++)
				{
					float L = L_g_i(t,pointCount-1,i);
					mom.x += L * pointArray[i]->x;
					mom.y += L * pointArray[i]->y;
					mom.z += L * pointArray[i]->z;
				}
				if (t!=0)
					spLine3D(spFloatToFixed(prev.x),spFloatToFixed(prev.y),spFloatToFixed(prev.z),
					         spFloatToFixed( mom.x),spFloatToFixed( mom.y),spFloatToFixed( mom.z),spGetFastRGB(0,255,255));
				prev = mom;
			}
				
		break;
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
	printf("* a point has to have this form: {x,y,z,u} with x,y,z,u floating points numbers.\n");
	printf("  \"u\" is a (not always used) parameter value.\n");
	int i;
	if (argc < 10)
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
	if ((argc-2) % 4 != 0)
	{
		printf("Every point needs 4 floats!\n");
		return 1;
	}
	pointCount = 0;
	for (i = 2; i < argc; i+=4)
	{
		pPoint point = (pPoint)malloc(sizeof(tPoint));
		point->x = atof(argv[i+0]);
		point->y = atof(argv[i+1]);
		point->z = atof(argv[i+2]);
		point->u = atof(argv[i+3]);
		point->next = NULL;
		if (lastPoint)
			lastPoint->next = point;
		else
			firstPoint = point;
		lastPoint = point;
		printf("Added point ( %6.2f | %6.2f | %6.2f )\n",point->x,point->y,point->z);
		pointCount++;
	}
	pointArray = (pPoint*)malloc(sizeof(pPoint)*pointCount);
	i = 0;
	pPoint point = firstPoint;
	while (point)
	{
		pointArray[i]	= point;
		i++;
		point = point->next;
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
	
	spSetZSet(0);
	spSetZTest(0);
	
	spLoop(draw,calc,10,resize,NULL);
	#ifdef SCALE_UP
	spDeleteSurface(screen);
	#endif
	spQuitCore();
	return 0;
}
