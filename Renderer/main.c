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

grMesh mesh;

char texName[1024] = { 0 };
void loadMesh(const char* path) {
	// This function has a terminal case of C Programmer's Disease
	// http://catb.org/jargon/html/C/C-Programmers-Disease.html

	FILE* f = fopen(path, "r");
	if (!f) {
		printf("Unable to open file %s\n", path);
		exit(EXIT_FAILURE);
	}

	char mtllib[1024] = { 0 };

#define LINE_MAX 1024
	char line[LINE_MAX];

#define MAX_VERTS 2048
	vec3* verts = malloc(MAX_VERTS * sizeof(vec3));
	vec2* uvs = malloc(MAX_VERTS * sizeof(vec2));
	int nverts = 0;
	int nuvs = 0;

	mesh.verts = malloc(MAX_VERTS * sizeof(grVertex));
	mesh.indices = malloc(MAX_VERTS * 3 * sizeof(int));
	mesh.count = 0;
	int NNN = 0;

	while (fgets(line, LINE_MAX, f)) {
		vec3 v;
		vec2 uv;

		int v0, uv0, v1, uv1, v2, uv2;

		// This code has a buffer overrun if the mtllib name is 1023 characters or longer
		// But for this example it should be fine
		// scanf sucks, libc sucks
		if (sscanf(line, "mtllib %s", mtllib) == 1) {
			FILE* mtlf = fopen(mtllib, "r");

			char line2[LINE_MAX];
			while (fgets(line2, LINE_MAX, mtlf)) {
				if (sscanf(line2, "map_Kd %s", texName) == 1) {

				}
			}

			fclose(mtlf);
		}
		else if (sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z)) {
			if (nverts == MAX_VERTS) {
				printf("Too many vertices\n");
				exit(EXIT_FAILURE);
			}

			verts[nverts++] = v;
		}
		else if (sscanf(line, "vt %f %f", &uv.x, &uv.y)) {
			if (nuvs == MAX_VERTS) {
				printf("Too many vertices\n");
				exit(EXIT_FAILURE);
			}

			uv.y = 1 - uv.y;
			uvs[nuvs++] = uv;
		}
		else if (sscanf(line, "f %d/%d %d/%d %d/%d", &v0, &uv0, &v1, &uv1, &v2, &uv2)) {
			mesh.verts[NNN] = (grVertex){ verts[v0 - 1], uvs[uv0 - 1] };
			mesh.indices[NNN] = NNN++;

			mesh.verts[NNN] = (grVertex){ verts[v2 - 1], uvs[uv2 - 1] };
			mesh.indices[NNN] = NNN++;
			
			mesh.verts[NNN] = (grVertex){ verts[v1 - 1], uvs[uv1 - 1] };
			mesh.indices[NNN] = NNN++;

			mesh.count++;
		}
	}

	fclose(f);
}

void render() {
	grClear(device, (rgb) { 255, 255, 255 });

	grDraw(device, &mesh);

	grFramebuffer* fb = device->fb;

	for (int i = 0; i < fb->height; i++) {
		for (int j = 0; j < fb->width; j++) {
			rgb* c = fb->colour[i * screenWidth + j];
			int r = 0, g = 0, b = 0;
			for (int i = 0; i < MSAA_SAMPLES; i++) {
				r += c[i].r;
				g += c[i].g;
				b += c[i].b;
			}
			r /= MSAA_SAMPLES;
			g /= MSAA_SAMPLES;
			b /= MSAA_SAMPLES;
			/*r = c[0].r;
			g = c[0].g;
			b = c[0].b;*/
			pixels[i * screenWidth + j] = (rgb){ r,g,b };
		}
	}
	//memcpy(pixels, device->fb->colour, screenWidth * screenHeight * sizeof(rgb));
}

int frame = 0;

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);

	device = grDevice_Create();
	device->fb = grFramebuffer_Create(screenWidth, screenHeight);

	device->proj = mat4_perspective(deg2rad(90), (float)screenWidth / screenHeight, 0.1f, 100);
	device->view = mat4_lookat((vec3) { 0, -3, 4 }, (vec3) { 0, 0, 0 }, (vec3) { 0, 1, 0 });

	loadMesh("cactus.obj");
	mesh.modelMat = mat4_identity();

	int tw;
	int th;
	int comp;
	rgb* texData = stbi_load(texName, &tw, &th, &comp, 3);
	device->tex = grTexture_Create(tw, th);
	grTexture_SetData(device->tex, texData, tw, th);

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
		mesh.modelMat = mat4_rotate_zyx(0, T, 0);
		mat4 tr = mat4_translate((vec3) { 0, -1, 0 });
		mesh.modelMat = mat4_mul(&tr, &mesh.modelMat);

		render();

		SDL_UnlockTexture(texture);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		frame++;
	}

	return 0;
}
