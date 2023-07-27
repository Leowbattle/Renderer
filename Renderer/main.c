#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

#include "stb_image.h"

void* xmalloc(size_t size) {
	void* p = malloc(size);
	if (p == NULL) {
		fprintf(stderr, "Error allocating %zu bytes\n", size);
		exit(EXIT_FAILURE);
	}
	return p;
}

float clampf(float x, float a, float b) {
	if (x < a) return a;
	if (x > b) return b;
	return x;
}

float fractf(float x) {
	return x - floorf(x);
}

float lerpf(float a, float b, float t) {
	return a * (1 - t) + b * t;
}

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;

int screenWidth = 640;
int screenHeight = 480;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb;

rgb* pixels;
int pitch;

typedef struct {
	int x;
	int y;
	float u;
	float v;
} Vertex;

typedef struct {
	int width;
	int height;
	rgb* data;
} Texture;

void Texture_load_file(Texture* tex, const char* file) {
	// quick and dirty

	int channels;
	tex->data = stbi_load(file, &tex->width, &tex->height, &channels, 3);
	if (tex->data == NULL) {
		fprintf("Error loading texture %s\n", file);
		exit(EXIT_FAILURE);
	}
}

rgb Texture_sample(Texture* tex, float u, float v) {
	u *= tex->width;
	v *= tex->height;

	float t1;
	if (u < 0.5f) t1 = 0;
	else if (u > tex->width - 0.5f) t1 = 1;
	else t1 = fractf(u - 0.5f);

	float t2;
	if (v < 0.5f) t2 = 0;
	else if (v > tex->height - 0.5f) t2 = 1;
	else t2 = fractf(v - 0.5f);

	int x0 = u - 0.5f;
	int x1 = u + 0.5f;

	int y0 = v - 0.5f;
	int y1 = v + 0.5f;

	x0 = clampf(x0, 0, tex->width - 1);
	x1 = clampf(x1, 0, tex->width - 1);

	y0 = clampf(y0, 0, tex->height - 1);
	y1 = clampf(y1, 0, tex->height - 1);
	
	rgb c00 = tex->data[y0 * tex->width + x0];
	rgb c10 = tex->data[y0 * tex->width + x1];
	rgb c01 = tex->data[y1 * tex->width + x0];
	rgb c11 = tex->data[y1 * tex->width + x1];

	rgb c = {
		lerpf(lerpf(c00.r, c10.r, t1), lerpf(c01.r, c11.r, t1), t2),
		lerpf(lerpf(c00.g, c10.g, t1), lerpf(c01.g, c11.g, t1), t2),
		lerpf(lerpf(c00.b, c10.b, t1), lerpf(c01.b, c11.b, t1), t2),
	};

	//c = (rgb){
	//	t1 * 255,
	//	t2 * 255,
	//	0,
	//};

	return c;
}

typedef struct {
	int width;
	int height;
	rgb(*data)[4];
} MultisampleFramebuffer;

void MS_init(MultisampleFramebuffer* fb, int width, int height) {
	fb->width = width;
	fb->height = height;
	fb->data = xmalloc(width * height * sizeof(rgb) * 4);
}

void MS_clear(MultisampleFramebuffer* fb, rgb c) {
	for (int i = 0; i < fb->height; i++) {
		for (int j = 0; j < fb->width; j++) {
			fb->data[i * fb->width + j][0] = c;
			fb->data[i * fb->width + j][1] = c;
			fb->data[i * fb->width + j][2] = c;
			fb->data[i * fb->width + j][3] = c;
		}
	}
	//memset(fb->data, 0, fb->width * fb->height * sizeof(rgb) * 4);
}

// Convert a multisampled image into a regular one.
void MS_multisample_resolve(MultisampleFramebuffer* fb, rgb* dest) {
	for (int i = 0; i < fb->height; i++) {
		for (int j = 0; j < fb->width; j++) {
			rgb(*ms_pixel)[4] = &fb->data[i * fb->width + j];
			rgb c = {
				((*ms_pixel)[0].r + (*ms_pixel)[1].r + (*ms_pixel)[2].r + (*ms_pixel)[3].r) / 4,
				((*ms_pixel)[0].g + (*ms_pixel)[1].g + (*ms_pixel)[2].g + (*ms_pixel)[3].g) / 4,
				((*ms_pixel)[0].b + (*ms_pixel)[1].b + (*ms_pixel)[2].b + (*ms_pixel)[3].b) / 4,
			};
			/*c = (rgb){
				(*ms_pixel)[0].r,
				(*ms_pixel)[0].g,
				(*ms_pixel)[0].b,
			};*/
			dest[i * fb->width + j] = c;
		}
	}
}

int SAMPLE_PATTERN[4][2] = {
	{-2, -6},
	{6, -2},
	{-6, 2},
	{2, 6},
};

void MS_tri(MultisampleFramebuffer* fb, Vertex v[3], Texture* tex) {
	int x0 = v[0].x * 16;
	int y0 = v[0].y * 16;
	int x1 = v[1].x * 16;
	int y1 = v[1].y * 16;
	int x2 = v[2].x * 16;
	int y2 = v[2].y * 16;

	int left = min(min(x0, x1), x2) & ~15;
	int right = max(max(x0, x1), x2) & ~15;
	int top = min(min(y0, y1), y2) & ~15;
	int bottom = max(max(y0, y1), y2) & ~15;

	for (int y = top; y <= bottom; y += 16) {
		for (int x = left; x <= right; x += 16) {
			int px = x / 16;
			int py = y / 16;

			int e01 = ((x + 8) - x0) * (y1 - y0) - ((y + 8) - y0) * (x1 - x0) + ((y0 == y1 && x1 < x0) || (y1 < y0));
			int e12 = ((x + 8) - x1) * (y2 - y1) - ((y + 8) - y1) * (x2 - x1) + ((y1 == y2 && x2 < x1) || (y2 < y1));
			int e20 = ((x + 8) - x2) * (y0 - y2) - ((y + 8) - y2) * (x0 - x2) + ((y2 == y0 && x0 < x2) || (y0 < y2));

			int coverage = 0;
			for (int i = 0; i < 4; i++) {
				int sx = SAMPLE_PATTERN[i][0];
				int sy = SAMPLE_PATTERN[i][1];

				if ((e01 + sx * (y1 - y0) + sy * (x1 - x0)) > 0 &&
					(e12 + sx * (y2 - y1) + sy * (x2 - x1)) > 0 &&
					(e20 + sx * (y0 - y2) + sy * (x0 - x2)) > 0) {
					coverage |= 1 << i;
				}
			}

			if (coverage != 0) {
				float l0 = (float)e12 / (e01 + e12 + e20);
				float l1 = (float)e20 / (e01 + e12 + e20);
				float l2 = (float)e01 / (e01 + e12 + e20);

				float U = l0 * v[0].u + l1 * v[1].u + l2 * v[2].u;
				float V = l0 * v[0].v + l1 * v[1].v + l2 * v[2].v;

				rgb c;// = (rgb){ U * 255, V * 255, 0 };
				c = Texture_sample(tex, U, V);

				for (int i = 0; i < 4; i++) {
					if (coverage & (1 << i)) {
						fb->data[py * fb->width + px][i] = c;
					}
				}
			}
		}
	}
}

MultisampleFramebuffer fb;

Texture TEX;

void render() {
	MS_clear(&fb, (rgb) { 255, 255, 255 });

	int mx;
	int my;
	SDL_GetMouseState(&mx, &my);

	Vertex tri1[3] = {
		{0, 0, 0, 0},
		{mx, my, 0, 1},
		{400, 0, 1, 0},
	};
	MS_tri(&fb, tri1, &TEX);

	Vertex tri2[3] = {
		{400, 0, 1, 0},
		{mx, my, 0, 1},
		{400, 300, 1, 1},
	};
	MS_tri(&fb, tri2, &TEX);

	MS_multisample_resolve(&fb, pixels);
}

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);

	Texture_load_file(&TEX, "mario.png");

	MS_init(&fb, screenWidth, screenHeight);

	bool running = true;
	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}

		SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

		render();

		SDL_UnlockTexture(texture);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	return 0;
}
