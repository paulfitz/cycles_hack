/*
 * Copyright 2011, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util_opengl.h"
#include "util_time.h"
#include "util_view.h"

#ifdef USE_SDL
#include <SDL/SDL.h>
#endif


#ifndef NO_VIEWER
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#endif

CCL_NAMESPACE_BEGIN

/* structs */

struct View {
	ViewInitFunc initf;
	ViewInitFunc moref;
	ViewExitFunc exitf;
	ViewResizeFunc resize;
	ViewDisplayFunc display;
	ViewKeyboardFunc keyboard;

	bool first_display;
	bool redraw;

	int width, height;
} V;

/* public */

static void view_display_text(int x, int y, const char *text)
{
#ifndef NO_VIEWER
	const char *c;

	glRasterPos3f(x, y, 0);

	for(c=text; *c != '\0'; c++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
#else
	printf("*** %s\n", text);
#endif
}

void view_display_info(const char *info)
{
#ifndef NO_VIEWER
	const int height = 20;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.1f, 0.1f, 0.1f, 0.8f);
	glRectf(0.0f, V.height - height, V.width, V.height);
	glDisable(GL_BLEND);

	glColor3f(0.5f, 0.5f, 0.5f);

	view_display_text(10, 7 + V.height - height, info);

	glColor3f(1.0f, 1.0f, 1.0f);
#else
	printf("*** INFO: %s\n", info);
#endif
}

static void view_display()
{
	if(V.first_display) {
		if(V.initf) V.initf();
		if(V.exitf) atexit(V.exitf);

		V.first_display = false;
	} else {
	  V.moref();
	}

#ifndef NO_VIEWER
	glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, V.width, 0, V.height);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRasterPos3f(0, 0, 0);
#endif

	if(V.display)
		V.display();

#ifndef NO_VIEWER
	glutSwapBuffers();
#endif
}

static void view_reshape(int width, int height)
{
#ifndef NO_VIEWER
	if(width <= 0 || height <= 0)
		return;
	
	V.width = width;
	V.height = height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

#endif

	if(V.resize)
		V.resize(width, height);
}

static void view_keyboard(unsigned char key, int x, int y)
{
#ifndef NO_VIEWER
	if(V.keyboard)
		V.keyboard(key);

	if(key == 'm')
		printf("mouse %d %d\n", x, y);
	if(key == 'q') {
		if(V.exitf) V.exitf();
		exit(0);
	}
#endif
}

void view_idle()
{
	if(V.redraw) {
		V.redraw = false;
#ifndef NO_VIEWER
		glutPostRedisplay();
#endif
	}

#ifndef NO_VIEWER
	time_sleep(0.1f);
#endif
}

void view_main_loop(const char *title, int width, int height,
	ViewInitFunc initf, ViewInitFunc moref, ViewExitFunc exitf,
	ViewResizeFunc resize, ViewDisplayFunc display,
	ViewKeyboardFunc keyboard)
{
	const char *name = "app";
	char *argv = (char*)name;
	int argc = 1;

	memset(&V, 0, sizeof(V));
	V.width = width;
	V.height = height;
	V.first_display = true;
	V.redraw = false;
	V.initf = initf;
	V.moref = moref;
	V.exitf = exitf;
	V.resize = resize;
	V.display = display;
	V.keyboard = keyboard;

#ifndef NO_VIEWER
	glutInit(&argc, &argv);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(0, 0);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
	glutCreateWindow(title);

#ifndef __APPLE__
	glewInit();
#endif
#endif

	view_reshape(width, height);

#ifndef NO_VIEWER
	glutDisplayFunc(view_display);
	glutIdleFunc(view_idle);
	glutReshapeFunc(view_reshape);
	glutKeyboardFunc(view_keyboard);

	glutMainLoop();
#else
	int keypress = 0;
	int ct = 0;
	while (!keypress) {
	  ct++;
	  view_display();
#ifdef USE_SDL
	  SDL_Event event;
	  while(SDL_PollEvent(&event)) {
	    switch (event.type) 
              {
	      case SDL_QUIT:
		keypress = 1;
		break;
	      case SDL_KEYDOWN:
		keypress = 1;
		break;
              }
	  }
#else
	  if (ct>3) keypress = 1;
#endif
	}
#endif
}

void view_redraw()
{
	V.redraw = true;
}

CCL_NAMESPACE_END

