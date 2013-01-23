#include <string.h>
#include <sparrow3d.h>
#include <math.h>

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
	float x,y,z,w,u;
	pPoint next;
} tPoint;

typedef struct s2Point *p2Point;
typedef struct s2Point
{
	tPoint left,right;
} t2Point;

#define MASK_SIZE 7
#define SUBDIVISION 2

const float mask[MASK_SIZE] = {-1.0f/16.0f,0.0f/16.0f,9.0f/16.0f,16.0f/16.0f,9.0f/16.0f,0.0f/16.0f,-1.0f/16.0f};

#define TYPE_MAX 6
typedef enum eType
{
	lagrange = 0,
	hermit = 1,
	bezier = 2,
	b_splines = 3,
	casteljau = 4,
	subdivision = 5
} tType;

tType type = lagrange;

pPoint firstPoint = NULL;
pPoint  lastPoint = NULL;
pPoint* pointArray;
int pointCount;
float maxU = 0.0f;

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
		result *= (t-pointArray[k]->u)/(pointArray[i]->u-pointArray[k]->u);
	}
	return result;
}

int fak(int i)
{
	if (i <= 1)
		return 1;
	return i*fak(i-1);
}

float g_over_i(int g,int i)
{
	if (0 <= i && i <= g)
		return (float)(fak(g))/(float)(fak(i)*fak(g-i));
	return 0;
}

float B_g_i(float t,int g,int i)
{
	return g_over_i(g,i)*pow(1.0f-t,g-i)*pow(t,i);
}

float N_g_i(float t,int g,int i)
{
	i = i%pointCount;
	if (g <= 0)
	{
		if (pointArray[i]->u <= t && t < pointArray[i+1]->u)
			return 1;
		return 0;
	}
	return (t-pointArray[i]->u    )/(pointArray[(i+g)%pointCount]->u-pointArray[i]->u    )*N_g_i(t,g-1,i)+
	       (pointArray[(i+1+g)%pointCount]->u-t)/(pointArray[(i+1+g)%pointCount]->u-pointArray[(i+1)%pointCount]->u)*N_g_i(t,g-1,i+1);
}

float H_3_i(float t,int i)
{
	switch (i)
	{
		case 0: return (1.0f-t)*(1.0f-t)*(1.0f+2.0f*t);
		case 1: return t*(1.0f-t)*(1.0f-t);
		case 2: return -t*t*(1.0f-t);
		case 3: return (3.0f-2.0f*t)*t*t;
	}
	return 0.0f;
}

float delta_i(int i)
{
	return pointArray[i+1]->u-pointArray[i]->u;
}

float s_i(float t,int i)
{
	return (t-pointArray[i]->u)/delta_i(i);
}

tPoint b_r_i(float t,int r,int i)
{
	if (r <= 0)
		return (*pointArray[i]);
	tPoint result,L,R;
	L = b_r_i(t,r-1,i);
	R = b_r_i(t,r-1,i+1);
	result.x = (1.0f-t)*L.x+t*R.x;
	result.y = (1.0f-t)*L.y+t*R.y;
	result.z = (1.0f-t)*L.z+t*R.z;
	return result;
}


t2Point sub_division(int pos)
{
	int i;
	t2Point result;
	result.left.x = 0.0f;
	result.left.y = 0.0f;
	result.left.z = 0.0f;
	result.right.x = 0.0f;
	result.right.y = 0.0f;
	result.right.z = 0.0f;
	int l = 1;
	for (i = 0; i < MASK_SIZE; i++)
	{
		if (l)
		{
			result.left.x += pointArray[pos]->x*mask[i];
			result.left.y += pointArray[pos]->y*mask[i];
			result.left.z += pointArray[pos]->z*mask[i];
			printf("%f %f %f\n",result.left.x,result.left.y,result.left.z);
		}
			else
		{
			result.right.x += pointArray[pos]->x*mask[i];
			result.right.y += pointArray[pos]->y*mask[i];
			result.right.z += pointArray[pos]->z*mask[i];
		}
		l = 1-l;
		pos++;
		if (pos >= pointCount)
			pos = 0;
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
	int i = 1;
	while (point)
	{
		spEllipse3D(spFloatToFixed(point->x),spFloatToFixed(point->y),spFloatToFixed(point->z),1<<SP_ACCURACY-4,1<<SP_ACCURACY-4,spGetFastRGB(255,255,0));
		Sint32 x,y,z;
		spProjectPoint3D(spFloatToFixed(point->x),spFloatToFixed(point->y),spFloatToFixed(point->z),&x,&y,&z,1);
		char buffer[8];
		sprintf(buffer,"%i",i);
		spFontDrawMiddle(x,y,z,buffer,font);
		i++;
		point = point->next;
	}
	//control polygon
	for (i = 1; i < pointCount; i++)
		spLine3D(spFloatToFixed(pointArray[i-1]->x),spFloatToFixed(pointArray[i-1]->y),spFloatToFixed(pointArray[i-1]->z),
						 spFloatToFixed(pointArray[i  ]->x),spFloatToFixed(pointArray[i  ]->y),spFloatToFixed(pointArray[i  ]->z),spGetFastRGB(127,127,127));
	spSetAlphaPattern4x4(127,0);
		spLine3D(spFloatToFixed(pointArray[pointCount-1]->x),spFloatToFixed(pointArray[pointCount-1]->y),spFloatToFixed(pointArray[pointCount-1]->z),
						 spFloatToFixed(pointArray[0  ]->x),spFloatToFixed(pointArray[0  ]->y),spFloatToFixed(pointArray[0  ]->z),spGetFastRGB( 63, 63, 63));
	spDeactivatePattern();
  //curve
  float t;
  tPoint prev;
  switch (type)
  {
		case lagrange:
			spSetAlphaTest(0);
			for (t = 0.0f; t <= maxU; t+=0.01f)
			{
				tPoint mom = {0.0f,0.0f,0.0f};
				for (i = 0; i < pointCount; i++)
				{
					float L = L_g_i(t,pointCount-1,i);
					mom.x += L * pointArray[i]->x;
					mom.y += L * pointArray[i]->y;
					mom.z += L * pointArray[i]->z;
					mom.w += L * pointArray[i]->w;
				}
				mom.x /= mom.w;
				mom.y /= mom.w;
				mom.z /= mom.w;
				if (t!=0)
					spLine3D(spFloatToFixed(prev.x),spFloatToFixed(prev.y),spFloatToFixed(prev.z),
					         spFloatToFixed( mom.x),spFloatToFixed( mom.y),spFloatToFixed( mom.z),spGetFastRGB(255,0,255));
				prev = mom;
			}
			spSetAlphaTest(1);
			spFontDrawMiddle(screen->w>>1,2,-1,"Lagrange",font);
		break;
		case hermit:
			for (i = 0; i < pointCount; i++)
				for (t = pointArray[i]->u; t < pointArray[(i+1)%pointCount]->u; t+=0.01f)
				{
					tPoint mom,m1,m2;
					if (i == 0)
					{
						m1.x = pointArray[(i+1)%pointCount]->x - pointArray[(i+pointCount-2)%pointCount]->x;
						m1.y = pointArray[(i+1)%pointCount]->y - pointArray[(i+pointCount-2)%pointCount]->y;
						m1.z = pointArray[(i+1)%pointCount]->z - pointArray[(i+pointCount-2)%pointCount]->z;
					}
					else
					if (i != pointCount)
					{
						m1.x = pointArray[(i+1)%pointCount]->x - pointArray[(i+pointCount-1)%pointCount]->x;
						m1.y = pointArray[(i+1)%pointCount]->y - pointArray[(i+pointCount-1)%pointCount]->y;
						m1.z = pointArray[(i+1)%pointCount]->z - pointArray[(i+pointCount-1)%pointCount]->z;
					}
					else
					{
						m1.x = pointArray[(i+2)%pointCount]->x - pointArray[(i+pointCount-1)%pointCount]->x;
						m1.y = pointArray[(i+2)%pointCount]->y - pointArray[(i+pointCount-1)%pointCount]->y;
						m1.z = pointArray[(i+2)%pointCount]->z - pointArray[(i+pointCount-1)%pointCount]->z;
					}
					/*float l = sqrt(m1.x*m1.x+m1.y*m1.y+m1.z*m1.z);
					m1.x /= l;
					m1.y /= l;
					m1.z /= l;*/
					if (i == 0)
					{
						m2.x = pointArray[(i+2)%pointCount]->x - pointArray[(i+0)%pointCount]->x;
						m2.y = pointArray[(i+2)%pointCount]->y - pointArray[(i+0)%pointCount]->y;
						m2.z = pointArray[(i+2)%pointCount]->z - pointArray[(i+0)%pointCount]->z;
					}
					else
					if (i != pointCount)
					{
						m2.x = pointArray[(i+2)%pointCount]->x - pointArray[(i+0)%pointCount]->x;
						m2.y = pointArray[(i+2)%pointCount]->y - pointArray[(i+0)%pointCount]->y;
						m2.z = pointArray[(i+2)%pointCount]->z - pointArray[(i+0)%pointCount]->z;
					}
					else
					{
						m2.x = pointArray[(i+3)%pointCount]->x - pointArray[(i+1)%pointCount]->x;
						m2.y = pointArray[(i+3)%pointCount]->y - pointArray[(i+1)%pointCount]->y;
						m2.z = pointArray[(i+3)%pointCount]->z - pointArray[(i+1)%pointCount]->z;
					}
					/*l = sqrt(m2.x*m2.x+m2.y*m2.y+m2.z*m2.z);
					m2.x /= l;
					m2.y /= l;
					m2.z /= l;*/
					float f1 = H_3_i(s_i(t,i),0);
					float f2 = delta_i(i) * H_3_i(s_i(t,i),1);
					float f3 = delta_i(i) * H_3_i(s_i(t,i),2);
					float f4 = H_3_i(s_i(t,i),3);
					mom.x = f1 * pointArray[i]->x
								+ f2 * m1.x
								+ f3 * m2.x
								+ f4 * pointArray[(i+1)%pointCount]->x;
					mom.y = f1 * pointArray[i]->y
								+ f2 * m1.y
								+ f3 * m2.y
								+ f4 * pointArray[(i+1)%pointCount]->y;
					mom.z = f1 * pointArray[i]->z
								+ f2 * m1.z
								+ f3 * m2.z
								+ f4 * pointArray[(i+1)%pointCount]->z;
					if (t!=0)
						spLine3D(spFloatToFixed(prev.x),spFloatToFixed(prev.y),spFloatToFixed(prev.z),
										 spFloatToFixed( mom.x),spFloatToFixed( mom.y),spFloatToFixed( mom.z),spGetFastRGB(127,127,255));
					prev = mom;
				}
			spFontDrawMiddle(screen->w>>1,2,-1,"Hermite",font);
		break;
		case b_splines:
			for (t = 0.0f; t <= maxU; t+=0.01f)
			{
				tPoint mom = {0.0f,0.0f,0.0f};
				int k = 2;
				for (i = k; i < pointCount-k; i++)
				{
					float N = N_g_i(t,k,i);
					mom.x += N * pointArray[i]->x;
					mom.y += N * pointArray[i]->y;
					mom.z += N * pointArray[i]->z;
				}
				if (t!=0)
					spLine3D(spFloatToFixed(prev.x),spFloatToFixed(prev.y),spFloatToFixed(prev.z),
					         spFloatToFixed( mom.x),spFloatToFixed( mom.y),spFloatToFixed( mom.z),spGetFastRGB(0,255,255));
				prev = mom;
			}
			spFontDrawMiddle(screen->w>>1,2,-1,"B Splines",font);
		break;
		case bezier:
			for (t = 0.0f; t <= 1.0f; t+=0.01f)
			{
				tPoint mom = {0.0f,0.0f,0.0f};
				for (i = 0; i < pointCount; i++)
				{
					float B = B_g_i(t,pointCount-1,i);
					mom.x += B * pointArray[i]->x;
					mom.y += B * pointArray[i]->y;
					mom.z += B * pointArray[i]->z;
				}
				if (t!=0)
					spLine3D(spFloatToFixed(prev.x),spFloatToFixed(prev.y),spFloatToFixed(prev.z),
					         spFloatToFixed( mom.x),spFloatToFixed( mom.y),spFloatToFixed( mom.z),spGetFastRGB(0,255,255));
				prev = mom;
			}
			spFontDrawMiddle(screen->w>>1,2,-1,"Bezier",font);
		break;
		case casteljau:
			for (t = 0.0f; t <= 1.0f; t+=0.01f)
			{
				tPoint mom = b_r_i(t,pointCount-1,0);
				if (t!=0)
					spLine3D(spFloatToFixed(prev.x),spFloatToFixed(prev.y),spFloatToFixed(prev.z),
					         spFloatToFixed( mom.x),spFloatToFixed( mom.y),spFloatToFixed( mom.z),spGetFastRGB(127,255,127));
				prev = mom;
			}
			spFontDrawMiddle(screen->w>>1,2,-1,"Casteljau",font);
		break;
		case subdivision:
			for (i = 0; i < pointCount; i++)
			{
				t2Point mom = sub_division(i);
				spLine3D(spFloatToFixed( mom.left.x),spFloatToFixed( mom.left.y),spFloatToFixed( mom.left.z),
								 spFloatToFixed(mom.right.x),spFloatToFixed(mom.right.y),spFloatToFixed(mom.right.z),spGetFastRGB(i*255/pointCount,255-i*255/pointCount,0));
			}
			spFontDrawMiddle(screen->w>>1,2,-1,"Subdivision",font);
		break;
	}
	spFontDraw(2,2,-1,"[q] before",font);
	spFontDrawRight(screen->w-8,2,-1,"next [e]",font);
	spFontDrawMiddle(screen->w>>1,screen->h-font->maxheight-2,-1,"Press [R] to quit, [B] to pause",font);
	#ifdef SCALE_UP
	spScale2XSmooth(screen,real_screen);
	#endif
	spFlip();
}

int pause = 0;
int calc(Uint32 steps)
{
	PspInput engineInput = spGetInput();
	if (engineInput->button[SP_BUTTON_SELECT])
	{
		pause = 1-pause;
		engineInput->button[SP_BUTTON_SELECT] = 0;
	}
	if (engineInput->button[SP_BUTTON_L])
	{
		type = (type + TYPE_MAX - 1) % TYPE_MAX;
		engineInput->button[SP_BUTTON_L] = 0;
	}
	if (engineInput->button[SP_BUTTON_R])
	{
		type = (type + 1) % TYPE_MAX;
		engineInput->button[SP_BUTTON_R] = 0;
	}
	if (!pause)
		rotation += steps*32;
	if (engineInput->button[SP_BUTTON_START])
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	printf("Use curves.sh kind control-point-1 control-point-2 [control-point-3..n]\n");
	printf("* kind can be \"lagrange\", \"bezier\".\n");
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
	if (strcmp(argv[1],"hermit") == 0)
		type = hermit;
	else
	if (strcmp(argv[1],"bezier") == 0)
		type = bezier;
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
		point->w = 1.0f;
		point->u = atof(argv[i+3]);
		if (point->u > maxU)
			maxU = point->u;
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
	pointArray[0]->w = 2.0f;
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
