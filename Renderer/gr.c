#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "util.h"

#include "gr.h"

grFramebuffer* grFramebuffer_Create(int width, int height) {
	grFramebuffer* fb = xmalloc(sizeof(grFramebuffer));
	fb->width = width;
	fb->height = height;
	fb->colour = xmalloc(width * height * sizeof(rgb) * MSAA_SAMPLES);
	fb->depth = xmalloc(width * height * sizeof(float) * MSAA_SAMPLES);
	return fb;
}

void grFramebuffer_Destroy(grFramebuffer* fb) {
	free(fb->colour);
	free(fb->depth);
	free(fb);
}

grTexture* grTexture_Create(int width, int height) {
	grTexture* tex = xmalloc(sizeof(grTexture));
	tex->width = width;
	tex->height = height;
	tex->filter = GR_LINEAR;
	tex->wrapU = GR_CLAMP;
	tex->wrapV = GR_CLAMP;
	tex->mipmaps = NULL;
	tex->numMipmaps = 0;
	return tex;
}

int IsPowerOfTwo(int x) {
	return (x != 0) && ((x & (x - 1)) == 0);
}

int int_log2(int x) {
	int l = 0;
	while (x >>= 1) ++l;
	return l;
}

void grTexture_SetData(grTexture* tex, rgb* data, int width, int height) {
	// For now I will only support square power of 2 textures
	// it just makes things easier
	assert(IsPowerOfTwo(width));
	assert(IsPowerOfTwo(height));
	assert(width == height);

	// Calculate number of mipmaps required
	int numLevels = int_log2(width) + 1;
	tex->numMipmaps = numLevels;
	tex->mipmaps = xmalloc(numLevels * sizeof(grMipmapLevel));

	// Allocate mipmaps
	int mipSize = width;
	for (int i = 0; i < numLevels; i++) {
		grMipmapLevel* mip = &tex->mipmaps[i];
		mip->width = mipSize;
		mip->height = mipSize;
		mip->data = xmalloc(mipSize * mipSize * sizeof(rgb));
		mipSize /= 2;
	}

	// First mipmap is just the original data
	memcpy(tex->mipmaps[0].data, data, width * height * sizeof(rgb));

	// Generate mipmap chain
	// I want to use i and j for y and x like in every other loop
	for (int i_m = 1; i_m < numLevels; i_m++) {
		grMipmapLevel* mip = &tex->mipmaps[i_m];
		grMipmapLevel* prev = &tex->mipmaps[i_m - 1];

		for (int i = 0; i < mip->height; i++) {
			for (int j = 0; j < mip->width; j++) {
				rgb a = prev->data[i * 2 * prev->width + j * 2];
				rgb b = prev->data[i * 2 * prev->width + j * 2 + 1];
				rgb c = prev->data[(i * 2 + 1) * prev->width + j * 2];
				rgb d = prev->data[(i * 2 + 1) * prev->width + j * 2 + 1];

				int R = (a.r + b.r + c.r + d.r) / 4;
				int G = (a.g + b.g + c.g + d.g) / 4;
				int B = (a.b + b.b + c.b + d.b) / 4;
				mip->data[i * mip->width + j] = (rgb){ R, G, B };
			}
		}
	}
}

grDevice* grDevice_Create(void) {
	grDevice* dev = xmalloc(sizeof(grDevice));
	dev->fb = NULL;
	return dev;
}

void grDevice_Destroy(grDevice* dev) {
	free(dev);
}

void grClear(grDevice* dev, rgb colour) {
	grFramebuffer* fb = dev->fb;
	for (int i = 0; i < fb->height; i++) {
		for (int j = 0; j < fb->width; j++) {
			rgb* p = fb->colour[i * fb->width + j];
			float* d = fb->depth[i * fb->width + j];
			for (int i = 0; i < 4; i++) {
				p[i] = colour;
				d[i] = 1;
			}
		}
	}

}

void grPoint(grDevice* dev, float x, float y, rgb colour) {
	int X = remapf(x, -1, 1, 0, dev->fb->width);
	int Y = remapf(y, -1, 1, 0, dev->fb->height);
	grPixel(dev, X, Y, colour);
}

void grPixel(grDevice* dev, int x, int y, rgb colour) {
	grFramebuffer* fb = dev->fb;
	if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) {
		return;
	}

	rgb* p = fb->colour[y * fb->width + x];
	for (int i = 0; i < MSAA_SAMPLES; i++) {
		p[i] = colour;
	}
}

// GLSL: textureLOD
rgb Texture_sample(grTexture* tex, float u, float v, int level) {
	grMipmapLevel* mip = &tex->mipmaps[level];

	u *= mip->width;
	v *= mip->height;

	float t1;
	if (u < 0.5f) t1 = 0;
	else if (u > mip->width - 0.5f) t1 = 1;
	else t1 = fractf(u - 0.5f);

	float t2;
	if (v < 0.5f) t2 = 0;
	else if (v > mip->height - 0.5f) t2 = 1;
	else t2 = fractf(v - 0.5f);

	int x0 = u - 0.5f;
	int x1 = u + 0.5f;

	int y0 = v - 0.5f;
	int y1 = v + 0.5f;

	x0 = clampf(x0, 0, mip->width - 1);
	x1 = clampf(x1, 0, mip->width - 1);

	y0 = clampf(y0, 0, mip->height - 1);
	y1 = clampf(y1, 0, mip->height - 1);

	rgb c00 = mip->data[y0 * mip->width + x0];
	rgb c10 = mip->data[y0 * mip->width + x1];
	rgb c01 = mip->data[y1 * mip->width + x0];
	rgb c11 = mip->data[y1 * mip->width + x1];

	rgb c = {
		lerpf(lerpf(c00.r, c10.r, t1), lerpf(c01.r, c11.r, t1), t2),
		lerpf(lerpf(c00.g, c10.g, t1), lerpf(c01.g, c11.g, t1), t2),
		lerpf(lerpf(c00.b, c10.b, t1), lerpf(c01.b, c11.b, t1), t2),
	};

	return c;
}

typedef struct {
	int x;
	int y;
	float z;
	float w;
	vec2 uv;
} VertexAttr;

int SAMPLE_PATTERN[4][2] = {
	{-2, -6},
	{6, -2},
	{-6, 2},
	{2, 6},
};

// TODO allocate extra border memory for shading
// so if part of the quad is off the left or bottom of the screen it doesn't crash

static void tri(grDevice* dev, VertexAttr attr[3]) {
	grFramebuffer* fb = dev->fb;

	int x0 = attr[0].x;
	int y0 = attr[0].y;
	float z0 = attr[0].z;
	vec2 uv0 = attr[0].uv;

	int x1 = attr[1].x;
	int y1 = attr[1].y;
	float z1 = attr[1].z;
	vec2 uv1 = attr[1].uv;

	int x2 = attr[2].x;
	int y2 = attr[2].y;
	float z2 = attr[2].z;
	vec2 uv2 = attr[2].uv;

	// Cull backfaces
	int A = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
	if (A > 0) {
		return;
	}

	// Compute bounding box of the triangle.
	// TODO: Implement proper triangle clipping
	// Current implementation will waste time shading pixels
	// that will be discarded because their z values are not in [0-1]
	// BTW my program doesn't actually discard these pixels anyway
	// because this is kind of bodged together.
	int left = min(min(x0, x1), x2) & ~15;
	int right = max(max(x0, x1), x2) & ~15;
	int top = min(min(y0, y1), y2) & ~15;
	int bottom = max(max(y0, y1), y2) & ~15;

	left = max(left, 0);
	right = min(right, (fb->width - 1) * 16);
	top = max(top, 0);
	bottom = min(bottom, (fb->height - 1) * 16);

	for (int y = top; y <= bottom; y += 32) {
		for (int x = left; x <= right; x += 32) {
			// 0: x, y
			// 1: x + 1, y
			// 2: x, y + 1
			// 3: x + 1, y + 1

			int px[4] = { x / 16, x / 16 + 1,x / 16,x / 16 + 1 };
			int py[4] = { y / 16,y / 16,y / 16 + 1,y / 16 + 1 };

			int e01[4];
			e01[0] = ((x + 8) - x0) * (y1 - y0) - ((y + 8) - y0) * (x1 - x0) + ((y0 == y1 && x1 < x0) || (y1 < y0));
			e01[1] = (((x + 16) + 8) - x0) * (y1 - y0) - ((y + 8) - y0) * (x1 - x0) + ((y0 == y1 && x1 < x0) || (y1 < y0));
			e01[2] = ((x + 8) - x0) * (y1 - y0) - (((y + 16) + 8) - y0) * (x1 - x0) + ((y0 == y1 && x1 < x0) || (y1 < y0));
			e01[3] = (((x + 16) + 8) - x0) * (y1 - y0) - (((y + 16) + 8) - y0) * (x1 - x0) + ((y0 == y1 && x1 < x0) || (y1 < y0));

			int e12[4];
			e12[0] = ((x + 8) - x1) * (y2 - y1) - ((y + 8) - y1) * (x2 - x1) + ((y1 == y2 && x2 < x1) || (y2 < y1));
			e12[1] = (((x + 16) + 8) - x1) * (y2 - y1) - ((y + 8) - y1) * (x2 - x1) + ((y1 == y2 && x2 < x1) || (y2 < y1));
			e12[2] = ((x + 8) - x1) * (y2 - y1) - (((y + 16) + 8) - y1) * (x2 - x1) + ((y1 == y2 && x2 < x1) || (y2 < y1));
			e12[3] = (((x + 16) + 8) - x1) * (y2 - y1) - (((y + 16) + 8) - y1) * (x2 - x1) + ((y1 == y2 && x2 < x1) || (y2 < y1));

			int e20[4];
			e20[0] = ((x + 8) - x2) * (y0 - y2) - ((y + 8) - y2) * (x0 - x2) + ((y2 == y0 && x0 < x2) || (y0 < y2));
			e20[1] = (((x + 16) + 8) - x2) * (y0 - y2) - ((y + 8) - y2) * (x0 - x2) + ((y2 == y0 && x0 < x2) || (y0 < y2));
			e20[2] = ((x + 8) - x2) * (y0 - y2) - (((y + 16) + 8) - y2) * (x0 - x2) + ((y2 == y0 && x0 < x2) || (y0 < y2));
			e20[3] = (((x + 16) + 8) - x2) * (y0 - y2) - (((y + 16) + 8) - y2) * (x0 - x2) + ((y2 == y0 && x0 < x2) || (y0 < y2));

			int coverage[4] = { 0 };
			float l0[4];
			float l1[4];
			float l2[4];
			float z[4];
			vec2 uv[4];
			vec2 uvv[4];

			for (int q = 0; q < 4; q++) {
				for (int i = 0; i < 4; i++) {
					int sx = SAMPLE_PATTERN[i][0];
					int sy = SAMPLE_PATTERN[i][1];

					if ((e01[q] + sx * (y1 - y0) + sy * (x1 - x0)) > 0 &&
						(e12[q] + sx * (y2 - y1) + sy * (x2 - x1)) > 0 &&
						(e20[q] + sx * (y0 - y2) + sy * (x0 - x2)) > 0) {
						coverage[q] |= 1 << i;
					}
				}

				l0[q] = (float)e12[q] / (e01[q] + e12[q] + e20[q]);
				l1[q] = (float)e20[q] / (e01[q] + e12[q] + e20[q]);
				l2[q] = (float)e01[q] / (e01[q] + e12[q] + e20[q]);

				z[q] = 1 / (1 / z0 * l0[q] + 1 / z1 * l1[q] + 1 / z2 * l2[q]);

				for (int i = 0; i < MSAA_SAMPLES; i++) {
					if (z[q] > fb->depth[py[q] * fb->width + px[q]][i]) {
						coverage[q] &= ~(1 << i);
					}
				}

				float W = 1 / (1 / attr[0].w * l0[q] + 1 / attr[1].w * l1[q] + 1 / attr[2].w * l2[q]);
				uv[q] = (vec2){
					W * (uv0.x * l0[q] + uv1.x * l1[q] + uv2.x * l2[q])/* * dev->tex->width*/,
					W * (uv0.y * l0[q] + uv1.y * l1[q] + uv2.y * l2[q])/* * dev->tex->width*/,
				};
				uvv[q] = (vec2){
					W * (uv0.x * l0[q] + uv1.x * l1[q] + uv2.x * l2[q]) * dev->tex->width,
					W * (uv0.y * l0[q] + uv1.y * l1[q] + uv2.y * l2[q]) * dev->tex->width,
				};
			}

			// Screen-space partial derivatives of uvs required for mipmapping
			vec2 dFdx_uv[4] = {
				{uvv[1].x - uvv[0].x, uvv[1].y - uvv[0].y},
				{uvv[1].x - uvv[0].x, uvv[1].y - uvv[0].y},
				{uvv[3].x - uvv[2].x, uvv[3].y - uvv[2].y},
				{uvv[3].x - uvv[2].x, uvv[3].y - uvv[2].y},
			};
			vec2 dFdy_uv[4] = {
				{uvv[2].x - uvv[0].x, uvv[2].y - uvv[0].y},
				{uvv[3].x - uvv[1].x, uvv[3].y - uvv[1].y},
				{uvv[2].x - uvv[0].x, uvv[2].y - uvv[0].y},
				{uvv[3].x - uvv[1].x, uvv[3].y - uvv[1].y},
			};

			rgb MIPCOLOURS[] = {
				{0, 0, 0},
				{255, 0, 0},
				{0, 255, 0},
				{0, 0, 255},
				{255, 255, 0},
				{255, 0, 255},
				{0, 255, 255},
				{127, 127, 127}
			};

			for (int q = 0; q < 4; q++) {
				float fx = squaref(dFdx_uv[q].x) + squaref(dFdx_uv[q].y);
				float fy = squaref(dFdy_uv[q].x) + squaref(dFdy_uv[q].y);
				float level = log2f(fmaxf(fx, fy)) / 2.f;
				level = fmaxf(level, 0.);

				rgb tc1 = Texture_sample(dev->tex, uv[q].x, uv[q].y, fminf((int)level, dev->tex->numMipmaps - 1));
				rgb tc2 = Texture_sample(dev->tex, uv[q].x, uv[q].y, fminf((int)level + 1, dev->tex->numMipmaps - 1));
				/*rgb tc1 = MIPCOLOURS[(int)level];
				rgb tc2 = MIPCOLOURS[(int)level + 1];*/
				rgb tc = (rgb){
					lerpf(tc1.r, tc2.r, fmodf(level, 1.f)),
					lerpf(tc1.g, tc2.g, fmodf(level, 1.f)),
					lerpf(tc1.b, tc2.b, fmodf(level, 1.f)),
				};
				//tc = Texture_sample(dev->tex, uv[q].x, uv[q].y, 0);
				
				rgb* c = fb->colour[py[q] * fb->width + px[q]];
				float* d = fb->depth[py[q] * fb->width + px[q]];

				for (int i = 0; i < MSAA_SAMPLES; i++) {
					if (coverage[q] & (1 << i)) {
						c[i] = tc;
						d[i] = z[q];
					}
				}
			}
		}
	}
}

#include <stdio.h>

void grDraw(grDevice* dev, grMesh* mesh) {
	mat4 vp = mat4_mul(&dev->proj, &dev->view);
	mat4 mvp = mat4_mul(&vp, &mesh->modelMat);

	for (int i = 0; i < mesh->count * 3; i += 3) {
		grVertex a = mesh->verts[mesh->indices[i + 0]];
		grVertex b = mesh->verts[mesh->indices[i + 1]];
		grVertex c = mesh->verts[mesh->indices[i + 2]];

		vec4 a_pos = mat4_mul_vec4(&mvp, (vec4) { a.pos.x, a.pos.y, a.pos.z, 1 });
		a_pos.x /= a_pos.w;
		a_pos.y /= a_pos.w;
		a_pos.z /= a_pos.w;
		//a_pos.w /= a_pos.w;

		vec4 b_pos = mat4_mul_vec4(&mvp, (vec4) { b.pos.x, b.pos.y, b.pos.z, 1 });
		b_pos.x /= b_pos.w;
		b_pos.y /= b_pos.w;
		b_pos.z /= b_pos.w;
		//b_pos.w /= b_pos.w;

		vec4 c_pos = mat4_mul_vec4(&mvp, (vec4) { c.pos.x, c.pos.y, c.pos.z, 1 });
		c_pos.x /= c_pos.w;
		c_pos.y /= c_pos.w;
		c_pos.z /= c_pos.w;
		//c_pos.w /= c_pos.w;

		//printf("%f %f %f\n", a_pos.z, b_pos.z, c_pos.z);

		int x0 = remapf(a_pos.x, -1, 1, 0, dev->fb->width) * 16;
		int y0 = remapf(a_pos.y, -1, 1, dev->fb->height, 0) * 16;

		int x1 = remapf(b_pos.x, -1, 1, 0, dev->fb->width) * 16;
		int y1 = remapf(b_pos.y, -1, 1, dev->fb->height, 0) * 16;

		int x2 = remapf(c_pos.x, -1, 1, 0, dev->fb->width) * 16;
		int y2 = remapf(c_pos.y, -1, 1, dev->fb->height, 0) * 16;

		a.uv.x /= a_pos.w;
		a.uv.y /= a_pos.w;
		b.uv.x /= b_pos.w;
		b.uv.y /= b_pos.w;
		c.uv.x /= c_pos.w;
		c.uv.y /= c_pos.w;

		VertexAttr attr[3] = {
			{x0, y0, a_pos.z, a_pos.w, a.uv},
			{x1, y1, b_pos.z, b_pos.w, b.uv},
			{x2, y2, c_pos.z, c_pos.w, c.uv},
		};

		tri(dev, attr);
	}
}
