#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

#include "stb_image.h"
#include "gr.h"

void* xmalloc(size_t size) {
	void* p = malloc(size);
	if (p == NULL) {
		fprintf(stderr, "Error allocating %zu bytes\n", size);
		exit(EXIT_FAILURE);
	}
	return p;
}

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;

int screenWidth = 640;
int screenHeight = 480;

rgb* pixels;
int pitch;

grDevice* device;

grVertex verts[] = {
	{{-1, -1, -1}, {0, 0, 0}},
	{{1, -1, -1}, {1, 0, 0}},
	{{1, -1, 1}, {0, 1, 0}},
	{{-1, -1, 1}, {1, 1, 0}},

	{{-1, 1, -1}, {0, 0, 1}},
	{{1, 1, -1}, {1, 0, 1}},
	{{1, 1, 1}, {0, 1, 1}},
	{{-1, 1, 1}, {1, 1, 1}},
};
int indices[] = { 
	0, 1, 2,
	0, 2, 3,
	4, 5, 6,
	4, 6, 7,
	0, 1, 5,
	0, 5, 4,
	1, 2, 5,
	2, 5, 6,
	2, 3, 6,
	3, 6, 7,
	3, 0, 4,
	3, 4, 7,
};
grMesh mesh;

void render() {
	grClear(device, (rgb) { 255, 255, 255 });

	grDraw(device, &mesh);

	memcpy(pixels, device->fb->colour, screenWidth * screenHeight * sizeof(rgb));
}

int frame = 0;

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);

	device = grDevice_Create();
	device->fb = grFramebuffer_Create(screenWidth, screenHeight);

	device->proj = mat4_perspective(deg2rad(90), (float)screenWidth / screenHeight, 0.1f, 10);
	device->view = mat4_lookat((vec3) { 0, 0, 3 }, (vec3) { 0, 0, 0 }, (vec3) { 0, 1, 0 });

	mesh.verts = verts;
	mesh.indices = indices;
	mesh.count = 12;
	mesh.modelMat = mat4_identity();

	bool running = true;
	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}

		SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

		float T = frame / 60.0f;
		mesh.modelMat = mat4_rotate_zyx(T, T, T);

		render();

		SDL_UnlockTexture(texture);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		frame++;
	}

	return 0;
}
