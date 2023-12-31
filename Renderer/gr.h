#ifndef GR_H
#define GR_H

#include "gr_math.h"

// TODO Allow this to be set at runtime
#define MSAA_SAMPLES 4

typedef struct {
	int width;
	int height;
	rgb (*colour)[MSAA_SAMPLES];
	float (*depth)[MSAA_SAMPLES];
} grFramebuffer;

grFramebuffer* grFramebuffer_Create(int width, int height);
void grFramebuffer_Destroy(grFramebuffer* fb);

typedef enum {
	GR_NEAREST,
	GR_LINEAR,
	GR_NEAREST_MIPMAP_NEAREST,
	GR_LINEAR_MIPMAP_LINEAR,
} grTextureFilter;

typedef enum {
	GR_CLAMP,
	GR_REPEAT,
} grTextureWrapMode;

typedef struct {
	int width;
	int height;
	rgb* data;
} grMipmapLevel;

typedef struct {
	int width;
	int height;
	grMipmapLevel* mipmaps;
	int numMipmaps;

	// For now these are unimplemented and just ignored
	grTextureFilter filter;
	grTextureWrapMode wrapU;
	grTextureWrapMode wrapV;
} grTexture;

grTexture* grTexture_Create(int width, int height);
void grTexture_SetData(grTexture* tex, rgb* data, int width, int height);

typedef struct {
	grFramebuffer* fb;
	mat4 proj;
	mat4 view;

	grTexture* tex;
} grDevice;

grDevice* grDevice_Create(void);
void grDevice_Destroy(grDevice* dev);

void grClear(grDevice* dev, rgb colour);
void grPoint(grDevice* dev, float x, float y, rgb colour);
void grPixel(grDevice* dev, int x, int y, rgb colour);

typedef struct {
	vec3 pos;
	vec2 uv;
} grVertex;

typedef struct {
	grVertex* verts;
	int* indices;
	int count;
	mat4 modelMat;
} grMesh;

void grDraw(grDevice* dev, grMesh* mesh);

#endif
