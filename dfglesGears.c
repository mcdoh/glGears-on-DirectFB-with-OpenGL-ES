/*
	(c) Copyright 2001  convergence integrated media GmbH.
	All rights reserved.

	Written by Denis Oliver Kropp <dok@convergence.de> and
	Andreas Hundt <andi@convergence.de>.

	Updated by Gavin McDonald <gavinmcdoh@gmail.com> to support
	OpenGL ES.

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the
	Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <directfb.h>
#include <directfbgl.h>

#include <GLES/gl.h>


// the super interface
IDirectFB *dfb;

// the primary surface (surface of primary layer)
IDirectFBSurface *primary;

// the GL context
IDirectFBGL *primary_gl;

// our font
IDirectFBFont *font;

// event buffer
IDirectFBEventBuffer *events;

// macro for a safe call to DirectFB functions
#define DFBCHECK(x...)                                         \
{                                                              \
	err = x;                                                   \
	if (err != DFB_OK) {                                       \
		fprintf(stderr, "%s <%d>:\n\t", __FILE__, __LINE__);   \
		DirectFBErrorFatal(#x, err);                           \
	}                                                          \
}

static int screen_width, screen_height;

static unsigned long T0 = 0;
static GLint Frames = 0;
static GLfloat fps = 0;

static inline unsigned long get_millis()
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


#ifndef M_PI
#define M_PI 3.14159265
#endif

static GLint autoexit = 0;

typedef struct
{
	GLfloat pos[3];
	GLfloat norm[3];
} vertex_t;

typedef struct
{
	vertex_t *vertices;
	GLshort *indices;
	GLfloat color[4];
	int nvertices;
	int nindices;
	GLuint ibo;
} gear_t;

//
//	Draw a gear wheel.  You'll probably want to call this function when
//	building a display list since we do a lot of trig here.
//
//	Input:
//		inner_radius - radius of hole at center
//		outer_radius - radius at center of teeth
//		width - width of gear
//		teeth - number of teeth
//		tooth_depth - depth of tooth
//

static gear_t* gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
					GLint teeth, GLfloat tooth_depth, GLfloat color[])
{
	GLint i, j;
	GLfloat r0, r1, r2;
	GLfloat ta, da;
	GLfloat u1, v1, u2, v2, len;
	GLfloat cos_ta, cos_ta_1da, cos_ta_2da, cos_ta_3da, cos_ta_4da;
	GLfloat sin_ta, sin_ta_1da, sin_ta_2da, sin_ta_3da, sin_ta_4da;
	GLshort ix0, ix1, ix2, ix3, ix4, ix5;
	vertex_t *vt, *nm;
	GLshort *ix;

	gear_t *gear = calloc(1, sizeof(gear_t));
	gear->nvertices = teeth * 40;
	gear->nindices = teeth * 66 * 3;
	gear->vertices = calloc(gear->nvertices, sizeof(vertex_t));
	gear->indices = calloc(gear->nindices, sizeof(GLshort));
	memcpy(&gear->color[0], &color[0], sizeof(GLfloat) * 4);

	r0 = inner_radius;
	r1 = outer_radius - tooth_depth / 2.0;
	r2 = outer_radius + tooth_depth / 2.0;
	da = 2.0 * M_PI / teeth / 4.0;

	vt = gear->vertices;
	nm = gear->vertices;
	ix = gear->indices;

#define VERTEX(x,y,z) ((vt->pos[0] = x),(vt->pos[1] = y),(vt->pos[2] = z), (vt++ - gear->vertices))
#define NORMAL(x,y,z) ((nm->norm[0] = x),(nm->norm[1] = y),(nm->norm[2] = z), (nm++ - gear->vertices))
#define INDEX(a,b,c) ((*ix++ = a),(*ix++ = b),(*ix++ = c))

	for (i = 0; i < teeth; i++)
	{
		ta = i * 2.0 * M_PI / teeth;

		cos_ta = cos(ta);
		cos_ta_1da = cos(ta + da);
		cos_ta_2da = cos(ta + 2 * da);
		cos_ta_3da = cos(ta + 3 * da);
		cos_ta_4da = cos(ta + 4 * da);
		sin_ta = sin(ta);
		sin_ta_1da = sin(ta + da);
		sin_ta_2da = sin(ta + 2 * da);
		sin_ta_3da = sin(ta + 3 * da);
		sin_ta_4da = sin(ta + 4 * da);

		u1 = r2 * cos_ta_1da - r1 * cos_ta;
		v1 = r2 * sin_ta_1da - r1 * sin_ta;
		len = sqrt(u1 * u1 + v1 * v1);
		u1 /= len;
		v1 /= len;
		u2 = r1 * cos_ta_3da - r2 * cos_ta_2da;
		v2 = r1 * sin_ta_3da - r2 * sin_ta_2da;

		// front face
		ix0 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          width * 0.5);
		ix1 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          width * 0.5);
		ix2 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          width * 0.5);
		ix3 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
		ix4 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      width * 0.5);
		ix5 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      width * 0.5);
		for (j = 0; j < 6; j++) {
			NORMAL(0.0,                  0.0,                  1.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);
		INDEX(ix2, ix3, ix4);
		INDEX(ix3, ix5, ix4);

		// front sides of teeth
		ix0 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          width * 0.5);
		ix1 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      width * 0.5);
		ix2 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
		ix3 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      width * 0.5);
		for (j = 0; j < 4; j++) {
			NORMAL(0.0,                  0.0,                  1.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);

		// back face
		ix0 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          -width * 0.5);
		ix1 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          -width * 0.5);
		ix2 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
		ix3 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          -width * 0.5);
		ix4 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      -width * 0.5);
		ix5 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      -width * 0.5);
		for (j = 0; j < 6; j++) {
			NORMAL(0.0,                  0.0,                  -1.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);
		INDEX(ix2, ix3, ix4);
		INDEX(ix3, ix5, ix4);

		// back sides of teeth
		ix0 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
		ix1 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      -width * 0.5);
		ix2 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          -width * 0.5);
		ix3 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      -width * 0.5);
		for (j = 0; j < 4; j++) {
			NORMAL(0.0,                  0.0,                  -1.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);

		// draw outward faces of teeth
		ix0 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          width * 0.5);
		ix1 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          -width * 0.5);
		ix2 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      width * 0.5);
		ix3 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      -width * 0.5);
		for (j = 0; j < 4; j++) {
			NORMAL(v1,                   -u1,                  0.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);
		ix0 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      width * 0.5);
		ix1 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      -width * 0.5);
		ix2 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      width * 0.5);
		ix3 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      -width * 0.5);
		for (j = 0; j < 4; j++) {
			NORMAL(cos_ta,               sin_ta,               0.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);
		ix0 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      width * 0.5);
		ix1 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      -width * 0.5);
		ix2 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
		ix3 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
		for (j = 0; j < 4; j++) {
			NORMAL(v2,                   -u2,                  0.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);
		ix0 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
		ix1 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
		ix2 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      width * 0.5);
		ix3 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      -width * 0.5);
		for (j = 0; j < 4; j++) {
			NORMAL(cos_ta,               sin_ta,               0.0);
		}
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);

		// draw inside radius cylinder
		ix0 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          -width * 0.5);
		ix1 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          width * 0.5);
		ix2 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      -width * 0.5);
		ix3 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      width * 0.5);
		NORMAL(-cos_ta,              -sin_ta,              0.0);
		NORMAL(-cos_ta,              -sin_ta,              0.0);
		NORMAL(-cos_ta_4da,          -sin_ta_4da,          0.0);
		NORMAL(-cos_ta_4da,          -sin_ta_4da,          0.0);
		INDEX(ix0, ix1, ix2);
		INDEX(ix1, ix3, ix2);
	}

	return gear;
}

static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLfloat inc_rotx = 0, inc_roty = 0, inc_rotz = 0;
static gear_t *gear1, *gear2, *gear3;
static GLfloat angle = 0.0;
/*
static void draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	glRotatef(view_rotx, 1.0, 0.0, 0.0);
	glRotatef(view_roty, 0.0, 1.0, 0.0);
	glRotatef(view_rotz, 0.0, 0.0, 1.0);

	glPushMatrix();
	glTranslatef(-3.0, -2.0, 0.0);
	glRotatef(angle, 0.0, 0.0, 1.0);
	glCallList(gear1);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.1, -2.0, 0.0);
	glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
	glCallList(gear2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.1, 4.2, 0.0);
	glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
	glCallList(gear3);
	glPopMatrix();

	glPopMatrix();
}
*/
// new window size or exposure
static void reshape(int width, int height)
{
	GLfloat h = (GLfloat) height / (GLfloat) width;

	glViewport(0, 0, (GLint) width, (GLint) height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustumf(-1.0, 1.0, -h, h, 5.0, 60.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -40.0);
}

static void init(int argc, char *argv[])
{
	static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};
	static GLfloat red[4] = {0.8, 0.1, 0.0, 1.0};
	static GLfloat green[4] = {0.0, 0.8, 0.2, 1.0};
	static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
	GLint i;

	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	glShadeModel(GL_SMOOTH);

	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	for (i=1; i<argc; i++)
	{
		if (strcmp(argv[i],"-info") == 0)
		{
			printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
			printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
			printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
			printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
		}
		else if (strcmp(argv[i],"-exit") == 0)
		{
			autoexit = 30;
			printf("Auto Exit after %i seconds.\n", autoexit );
		}
	}

	// make the gears
	gear1 = gear(1.0, 4.0, 1.0, 20, 0.7, red);
	gear2 = gear(0.5, 2.0, 2.0, 10, 0.7, green);
	gear3 = gear(1.3, 2.0, 0.5, 10, 0.7, blue);
}

int main(int argc, char *argv[])
{
	int quit = 0;
	DFBResult err;
	DFBSurfaceDescription dsc;

	DFBCHECK(DirectFBInit(&argc, &argv));

	// create the super interface
	DFBCHECK(DirectFBCreate(&dfb));

	// create an event buffer for all devices with these caps
	DFBCHECK(dfb->CreateInputEventBuffer(dfb, DICAPS_KEYS | DICAPS_AXES, DFB_FALSE, &events));

	// set our cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer
	dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN);

	// get the primary surface, i.e. the surface of the primary layer we have exclusive access to
	dsc.flags = DSDESC_CAPS;
	dsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE | DSCAPS_OPENGL_HINT;

	DFBCHECK(dfb->CreateSurface(dfb, &dsc, &primary));

	// get the size of the surface and fill it
	DFBCHECK(primary->GetSize(primary, &screen_width, &screen_height));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));
	primary->Flip(primary, NULL, 0);

	// create the default font and set it
	DFBCHECK(dfb->CreateFont(dfb, NULL, NULL, &font));
	DFBCHECK(primary->SetFont(primary, font));

	// get the GL context
	DFBCHECK(primary->GetGL(primary, &primary_gl));

	DFBCHECK(primary_gl->Lock(primary_gl));

	init(argc, argv);
	reshape(screen_width, screen_height);

	DFBCHECK(primary_gl->Unlock(primary_gl));

	T0 = get_millis();

	while (!quit)
	{
		DFBInputEvent evt;
		unsigned long t;

		DFBCHECK(primary_gl->Lock(primary_gl));

//		draw();

		DFBCHECK(primary_gl->Unlock(primary_gl));

		if (fps)
		{
			char buf[64];

			snprintf(buf, 64, "%4.1f FPS\n", fps);

			primary->SetColor(primary, 0xff, 0, 0, 0xff);
			primary->DrawString(primary, buf, -1, screen_width - 5, 5, DSTF_TOPRIGHT);
		}

		primary->Flip(primary, NULL, 0);
		Frames++;


		t = get_millis();
		if (t - T0 >= 2000)
		{
			GLfloat seconds = (t - T0) / 1000.0;

			fps = Frames / seconds;

			T0 = t;
			Frames = 0;
		}


		while (events->GetEvent(events, DFB_EVENT(&evt)) == DFB_OK)
		{
			switch (evt.type)
			{
				case DIET_KEYPRESS:
					switch (evt.key_symbol)
					{
						case DIKS_ESCAPE:
							quit = 1;
							break;
						case DIKS_CURSOR_UP:
							inc_rotx = 5.0;
							break;
						case DIKS_CURSOR_DOWN:
							inc_rotx = -5.0;
							break;
						case DIKS_CURSOR_LEFT:
							inc_roty = 5.0;
							break;
						case DIKS_CURSOR_RIGHT:
							inc_roty = -5.0;
							break;
						case DIKS_PAGE_UP:
							inc_rotz = 5.0;
							break;
						case DIKS_PAGE_DOWN:
							inc_rotz = -5.0;
							break;
						default:
							;
					}
					break;
				case DIET_KEYRELEASE:
					switch (evt.key_symbol)
					{
						case DIKS_CURSOR_UP:
							inc_rotx = 0;
							break;
						case DIKS_CURSOR_DOWN:
							inc_rotx = 0;
							break;
						case DIKS_CURSOR_LEFT:
							inc_roty = 0;
							break;
						case DIKS_CURSOR_RIGHT:
							inc_roty = 0;
							break;
						case DIKS_PAGE_UP:
							inc_rotz = 0;
							break;
						case DIKS_PAGE_DOWN:
							inc_rotz = 0;
							break;
						default:
							;
					}
					break;
				case DIET_AXISMOTION:
					if (evt.flags & DIEF_AXISREL)
					{
						switch (evt.axis)
						{
							case DIAI_X:
								view_roty += evt.axisrel / 2.0;
								break;
							case DIAI_Y:
								view_rotx += evt.axisrel / 2.0;
								break;
							case DIAI_Z:
								view_rotz += evt.axisrel / 2.0;
								break;
							default:
								;
						}
					}
					break;
				default:
					;
			}
		}

		angle += 2.0;

		view_rotx += inc_rotx;
		view_roty += inc_roty;
		view_rotz += inc_rotz;
	}

	// release our interfaces to shutdown DirectFB
	primary_gl->Release(primary_gl);
	primary->Release(primary);
	font->Release(font);
	events->Release(events);
	dfb->Release(dfb);

	return 0;
}

