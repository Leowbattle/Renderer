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
		int e01_0 = ((y0 == y1 && x1 < x0) || (y1 < y0));
		int e12_0 = ((y1 == y2 && x2 < x1) || (y2 < y1));
		int e20_0 = ((y2 == y0 && x0 < x2) || (y0 < y2));

		int e01_1 = ((y0 == y1 && x1 < x0) || (y1 < y0));
		int e12_1 = ((y1 == y2 && x2 < x1) || (y2 < y1));
		int e20_1 = ((y2 == y0 && x0 < x2) || (y0 < y2));

		int e01_2 = ((y0 == y1 && x1 < x0) || (y1 < y0));
		int e12_2 = ((y1 == y2 && x2 < x1) || (y2 < y1));
		int e20_2 = ((y2 == y0 && x0 < x2) || (y0 < y2));

		int e01_3 = ((y0 == y1 && x1 < x0) || (y1 < y0));
		int e12_3 = ((y1 == y2 && x2 < x1) || (y2 < y1));
		int e20_3 = ((y2 == y0 && x0 < x2) || (y0 < y2));

		for (int x = left; x <= right; x += 16) {
			int px = x / 16;
			int py = y / 16;

			e01_0 += ((x + 2) - x0) * (y1 - y0) - ((y + 10) - y0) * (x1 - x0);
			e12_0 += ((x + 2) - x1) * (y2 - y1) - ((y + 10) - y1) * (x2 - x1);
			e20_0 += ((x + 2) - x2) * (y0 - y2) - ((y + 10) - y2) * (x0 - x2);
			if (e01_0 > 0 && e12_0 > 0 && e20_0 > 0) {
				fb->data[py * fb->width + px][0] = c;
			}

			e01_1 += ((x + 6) - x0) * (y1 - y0) - ((y + 2) - y0) * (x1 - x0);
			e12_1 += ((x + 6) - x1) * (y2 - y1) - ((y + 2) - y1) * (x2 - x1);
			e20_1 += ((x + 6) - x2) * (y0 - y2) - ((y + 2) - y2) * (x0 - x2);
			if (e01_1 > 0 && e12_1 > 0 && e20_1 > 0) {
				fb->data[py * fb->width + px][1] = c;
			}

			e01_2 += ((x + 10) - x0) * (y1 - y0) - ((y + 14) - y0) * (x1 - x0);
			e12_2 += ((x + 10) - x1) * (y2 - y1) - ((y + 14) - y1) * (x2 - x1);
			e20_2 += ((x + 10) - x2) * (y0 - y2) - ((y + 14) - y2) * (x0 - x2);
			if (e01_2 > 0 && e12_2 > 0 && e20_2 > 0) {
				fb->data[py * fb->width + px][2] = c;
			}

			e01_3 += ((x + 14) - x0) * (y1 - y0) - ((y + 6) - y0) * (x1 - x0);
			e12_3 += ((x + 14) - x1) * (y2 - y1) - ((y + 6) - y1) * (x2 - x1);
			e20_3 += ((x + 14) - x2) * (y0 - y2) - ((y + 6) - y2) * (x0 - x2);
			if (e01_3 > 0 && e12_3 > 0 && e20_3 > 0) {
				fb->data[py * fb->width + px][3] = c;
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
