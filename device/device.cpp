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

#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "device_intern.h"

//#include "util_cuda.h"
#include "util_debug.h"
#include "util_foreach.h"
#include "util_math.h"
#include "util_opencl.h"
#include "util_opengl.h"
#include "util_types.h"
#include "util_vector.h"

#ifdef USE_SDL
#include <SDL/SDL.h>
#endif

CCL_NAMESPACE_BEGIN

/* Device Task */

DeviceTask::DeviceTask(Type type_)
: type(type_), x(0), y(0), w(0), h(0), rng_state(0), rgba(0), buffer(0),
  sample(0), resolution(0),
  shader_input(0), shader_output(0),
  shader_eval_type(0), shader_x(0), shader_w(0)
{
}

void DeviceTask::split_max_size(list<DeviceTask>& tasks, int max_size)
{
	int num;

	if(type == SHADER) {
		num = (shader_w + max_size - 1)/max_size;
	}
	else {
		max_size = max(1, max_size/w);
		num = (h + max_size - 1)/max_size;
	}

	split(tasks, num);
}

void DeviceTask::split(ThreadQueue<DeviceTask>& queue, int num)
{
	list<DeviceTask> tasks;
	split(tasks, num);

	foreach(DeviceTask& task, tasks)
		queue.push(task);
}

void DeviceTask::split(list<DeviceTask>& tasks, int num)
{
	if(type == SHADER) {
		num = min(shader_w, num);

		for(int i = 0; i < num; i++) {
			int tx = shader_x + (shader_w/num)*i;
			int tw = (i == num-1)? shader_w - i*(shader_w/num): shader_w/num;

			DeviceTask task = *this;

			task.shader_x = tx;
			task.shader_w = tw;

			tasks.push_back(task);
		}
	}
	else {
		num = min(h, num);

		for(int i = 0; i < num; i++) {
			int ty = y + (h/num)*i;
			int th = (i == num-1)? h - i*(h/num): h/num;

			DeviceTask task = *this;

			task.y = ty;
			task.h = th;

			tasks.push_back(task);
		}
	}
}

/* Device */

void Device::pixels_alloc(device_memory& mem)
{
	mem_alloc(mem, MEM_READ_WRITE);
}

void Device::pixels_copy_from(device_memory& mem, int y, int w, int h)
{
	mem_copy_from(mem, y, w, h, sizeof(uint8_t)*4);
}

void Device::pixels_free(device_memory& mem)
{
	mem_free(mem);
}

void Device::draw_pixels(device_memory& rgba, int y, int w, int h, int dy, int width, int height, bool transparent)
{
	pixels_copy_from(rgba, y, w, h);

	uint8_t *pixels = (uint8_t*)rgba.data_pointer;
	/* for multi devices, this assumes the ineffecient method that we allocate
	   all pixels on the device even though we only render to a subset */
	pixels += 4*y*w;

	printf("I think we have an image of size %d %d\n", w, h);
#ifdef USE_SDL
	SDL_Surface *screen = SDL_SetVideoMode(w,h, 32, SDL_SWSURFACE);
	if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
	for (int i = 0; i < h; i++) {
	  char *target = (char*)screen->pixels + i*w*4;
	  char *src = (char *)pixels + ((h-i-1)*w*4);
	  for (int j = 0; j < w; j++) {
	    target[0] = src[2];
	    target[1] = src[1];
	    target[2] = src[0];
	    target[3] = src[3];
	    target += 4;
	    src += 4;
	  }
	}
	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen); 
#else

#ifndef NO_FILE_OUTPUT
	static int ct = 0;
	char buf[1000];
	sprintf(buf,"test_%06d.ppm",ct);
	ct++;
	FILE *fp = fopen(buf, "wb");
	if (!fp) {
	  printf("cannot open file for writing\n");
	  return;
        } else {
	  const int inc = w*4;
	  fprintf(fp, "P6\n%d %d\n%d\n", w, h, 255);
	  for (int yy = 0; yy < h; yy++) {
	    char *src = (char *)pixels + ((h-yy-1)*w*4);
	    for (int xx = 0; xx < w; xx++) {
	      fwrite((void *) src, 1, (size_t) (3), fp);
	      src += 4;
	    }
	  }
	  fclose(fp);
        }
	printf("wrote to %s (hacked in %s)\n", buf, __FILE__);
#endif

	if (w<200) {
	  for (int yy = 0; yy < h; yy++) {
	    printf("993 ");
	    char *src = (char*)rgba.device_pointer + ((h-yy-1)*w*4);
	    for (int xx = 0; xx < w; xx++) {
	      printf("%d ", (unsigned char)src[0]);
	      printf("%d ", (unsigned char)src[1]);
	      printf("%d ", (unsigned char)src[2]);
	      src += 4;
	    }
	    printf("\n");
	  }
	}
	printf("999 \n");
	exit(0);


#endif
#ifndef NO_VIEWER
	if(transparent) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}

	glPixelZoom((float)width/(float)w, (float)height/(float)h);
	glRasterPos2f(0, dy);

	glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	glRasterPos2f(0.0f, 0.0f);
	glPixelZoom(1.0f, 1.0f);

	if(transparent)
		glDisable(GL_BLEND);
#endif
}

Device *Device::create(DeviceInfo& info, bool background, int threads)
{
	Device *device;

	switch(info.type) {
		case DEVICE_CPU:
			device = device_cpu_create(info, threads);
			break;
#ifdef WITH_CUDA
		case DEVICE_CUDA:
			if(cuLibraryInit())
				device = device_cuda_create(info, background);
			else
				device = NULL;
			break;
#endif
#ifdef WITH_MULTI
		case DEVICE_MULTI:
			device = device_multi_create(info, background);
			break;
#endif
#ifdef WITH_NETWORK
		case DEVICE_NETWORK:
			device = device_network_create(info, "127.0.0.1");
			break;
#endif
#ifdef WITH_OPENCL
		case DEVICE_OPENCL:
			if(clLibraryInit())
				device = device_opencl_create(info, background);
			else
				device = NULL;
			break;
#endif
		default:
			return NULL;
	}

	if(device)
		device->info = info;

	return device;
}

DeviceType Device::type_from_string(const char *name)
{
	if(strcmp(name, "cpu") == 0)
		return DEVICE_CPU;
	else if(strcmp(name, "cuda") == 0)
		return DEVICE_CUDA;
	else if(strcmp(name, "opencl") == 0)
		return DEVICE_OPENCL;
	else if(strcmp(name, "network") == 0)
		return DEVICE_NETWORK;
	else if(strcmp(name, "multi") == 0)
		return DEVICE_MULTI;
	
	return DEVICE_NONE;
}

string Device::string_from_type(DeviceType type)
{
	if(type == DEVICE_CPU)
		return "cpu";
	else if(type == DEVICE_CUDA)
		return "cuda";
	else if(type == DEVICE_OPENCL)
		return "opencl";
	else if(type == DEVICE_NETWORK)
		return "network";
	else if(type == DEVICE_MULTI)
		return "multi";
	
	return "";
}

vector<DeviceType>& Device::available_types()
{
	static vector<DeviceType> types;
	static bool types_init = false;

	if(!types_init) {
		types.push_back(DEVICE_CPU);

#ifdef WITH_CUDA
		if(cuLibraryInit())
			types.push_back(DEVICE_CUDA);
#endif

#ifdef WITH_OPENCL
		if(clLibraryInit())
			types.push_back(DEVICE_OPENCL);
#endif

#ifdef WITH_NETWORK
		types.push_back(DEVICE_NETWORK);
#endif
#ifdef WITH_MULTI
		types.push_back(DEVICE_MULTI);
#endif

		types_init = true;
	}

	return types;
}

vector<DeviceInfo>& Device::available_devices()
{
	static vector<DeviceInfo> devices;
	static bool devices_init = false;

	if(!devices_init) {
#ifdef WITH_CUDA
		if(cuLibraryInit())
			device_cuda_info(devices);
#endif

#ifdef WITH_OPENCL
		if(clLibraryInit())
			device_opencl_info(devices);
#endif

#ifdef WITH_MULTI
		device_multi_info(devices);
#endif

		device_cpu_info(devices);

#ifdef WITH_NETWORK
		device_network_info(devices);
#endif

		devices_init = true;
	}

	return devices;
}

CCL_NAMESPACE_END

