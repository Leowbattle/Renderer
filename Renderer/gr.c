#include <stdlib.h>

#include "util.h"

#include "gr.h"

grFramebuffer* grFramebuffer_Create(int width, int height) {
	grFramebuffer* fb = xmalloc(sizeof(grFramebuffer));
	fb->width = width;
	fb->height = height;
	fb->colour = xmalloc(width * height * sizeof(rgb));
	fb->depth = xmalloc(width * height * sizeof(float));
	return fb;
}

void grFramebuffer_Destroy(grFramebuffer* fb) {
	free(fb->colour);
	free(fb->depth);
	free(fb);
}

//grTexture* grTexture_Create(int width, int height) {
//	grTexture* tex = xmalloc(sizeof(grTexture));
//	return tex;
//}

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
			fb->colour[i * fb->width + j] = colour;
			fb->depth[i * fb->width + j] = 1;
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
	fb->colour[y * fb->width + x] = colour;
}

typedef struct {
	int x;
	int y;
	float z;
	vec3 c;
	vec2 uv;
} VertexAttr;

static void tri(grDevice* dev, VertexAttr attr[3]) {
	grFramebuffer* fb = dev->fb;

	int x0 = attr[0].x;
	int y0 = attr[0].y;
	float z0 = attr[0].z;
	vec3 c0 = attr[0].c;
	vec2 uv0 = attr[0].uv;

	int x1 = attr[1].x;
	int y1 = attr[1].y;
	float z1 = attr[1].z;
	vec3 c1 = attr[1].c;
	vec2 uv1 = attr[1].uv;

	int x2 = attr[2].x;
	int y2 = attr[2].y;
	float z2 = attr[2].z;
	vec3 c2 = attr[2].c;
	vec2 uv2 = attr[2].uv;

	int A = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
	if (A > 0) {
		return;
		int tmp;

		tmp = x1;
		x1 = x2;
		x2 = tmp;

		tmp = y1;
		y1 = y2;
		y2 = tmp;

		vec3 tmpc;
		tmpc = c1;
		c1 = c2;
		c2 = tmpc;
	}
	
	int left = min(min(x0, x1), x2) & ~15;
	int right = max(max(x0, x1), x2) & ~15;
	int top = min(min(y0, y1), y2) & ~15;
	int bottom = max(max(y0, y1), y2) & ~15;

	left = max(left, 0);
	right = min(right, (fb->width - 1) * 16);
	top = max(top, 0);
	bottom = min(bottom, (fb->height - 1) * 16);

	c0.x /= z0;
	c0.y /= z0;
	c0.z /= z0;
	c1.x /= z1;
	c1.y /= z1;
	c1.z /= z1;
	c2.x /= z2;
	c2.y /= z2;
	c2.z /= z2;

	for (int y = top; y <= bottom; y += 16) {
		for (int x = left; x <= right; x += 16) {
			int px = x / 16;
			int py = y / 16;

			int e01 = ((x + 8) - x0) * (y1 - y0) - ((y + 8) - y0) * (x1 - x0) + ((y0 == y1 && x1 < x0) || (y1 < y0));
			int e12 = ((x + 8) - x1) * (y2 - y1) - ((y + 8) - y1) * (x2 - x1) + ((y1 == y2 && x2 < x1) || (y2 < y1));
			int e20 = ((x + 8) - x2) * (y0 - y2) - ((y + 8) - y2) * (x0 - x2) + ((y2 == y0 && x0 < x2) || (y0 < y2));

			if (e01 > 0 && e12 > 0 && e20 > 0) {
				float l0 = (float)e12 / (e01 + e12 + e20);
				float l1 = (float)e20 / (e01 + e12 + e20);
				float l2 = (float)e01 / (e01 + e12 + e20);

				float z = 1/(1/z0 * l0 + 1/z1 * l1 + 1/z2 * l2);
				//z = clampf(z, 0, 1);

				if (z > fb->depth[py * fb->width + px]) {
					continue;
				}

				//rgb c = { z * 255, z * 255, z * 255 };

				rgb c = {
					z * (c0.x * l0 + c1.x * l1 + c2.x * l2) * 255,
					z * (c0.y * l0 + c1.y * l1 + c2.y * l2) * 255,
					z * (c0.z * l0 + c1.z * l1 + c2.z * l2) * 255,
				};

				fb->colour[py * fb->width + px] = c;
				fb->depth[py * fb->width + px] = z;
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
		a_pos.w /= a_pos.w;

		vec4 b_pos = mat4_mul_vec4(&mvp, (vec4) { b.pos.x, b.pos.y, b.pos.z, 1 });
		b_pos.x /= b_pos.w;
		b_pos.y /= b_pos.w;
		b_pos.z /= b_pos.w;
		b_pos.w /= b_pos.w;

		vec4 c_pos = mat4_mul_vec4(&mvp, (vec4) { c.pos.x, c.pos.y, c.pos.z, 1 });
		c_pos.x /= c_pos.w;
		c_pos.y /= c_pos.w;
		c_pos.z /= c_pos.w;
		c_pos.w /= c_pos.w;

		//printf("%f %f %f\n", a_pos.z, b_pos.z, c_pos.z);

		int x0 = remapf(a_pos.x, -1, 1, 0, dev->fb->width) * 16;
		int y0 = remapf(a_pos.y, -1, 1, dev->fb->height, 0) * 16;

		int x1 = remapf(b_pos.x, -1, 1, 0, dev->fb->width) * 16;
		int y1 = remapf(b_pos.y, -1, 1, dev->fb->height, 0) * 16;

		int x2 = remapf(c_pos.x, -1, 1, 0, dev->fb->width) * 16;
		int y2 = remapf(c_pos.y, -1, 1, dev->fb->height, 0) * 16;

		VertexAttr attr[3] = {
			{x0, y0, a_pos.z, a.colour, a.uv},
			{x1, y1, b_pos.z, b.colour, b.uv},
			{x2, y2, c_pos.z, c.colour, c.uv},
		};

		tri(dev, attr);
	}
}
