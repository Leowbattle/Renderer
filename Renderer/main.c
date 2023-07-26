#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

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

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb;

rgb* pixels;
int pitch;

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
			/*rgb c = {
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

void MS_tri(MultisampleFramebuffer* fb, int x0, int y0, int x1, int y1, int x2, int y2, rgb c) {
	x0 *= 16;
	y0 *= 16;
	x1 *= 16;
	y1 *= 16;
	x2 *= 16;
	y2 *= 16;

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
			
			for (int i = 0; i < 4; i++) {
				int sx = SAMPLE_PATTERN[i][0];
				int sy = SAMPLE_PATTERN[i][1];

				if ((e01 + sx * (y1 - y0) + sy * (x1 - x0)) > 0 &&
					(e12 + sx * (y2 - y1) + sy * (x2 - x1)) > 0 &&
					(e20 + sx * (y0 - y2) + sy * (x0 - x2)) > 0) {
					fb->data[py * fb->width + px][i] = c;
				}
			}
		}
	}

	/*fb->data[y0 * fb->width + x0][0] = (rgb){ 0, 255, 0 };
	fb->data[y1 * fb->width + x1][0] = (rgb){ 0, 255, 0 };
	fb->data[y2 * fb->width + x2][0] = (rgb){ 0, 255, 0 };*/
}

MultisampleFramebuffer fb;

void render() {
	MS_clear(&fb, (rgb) {255, 255, 255});

	int mx;
	int my;
	SDL_GetMouseState(&mx, &my);
	MS_tri(&fb, 0, 0, mx, my, 400, 50, (rgb) { 255, 0, 0 });
	MS_tri(&fb, 400, 50, mx, my, 500, 300, (rgb) { 0, 255, 0 });

	MS_multisample_resolve(&fb, pixels);
}

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);

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
