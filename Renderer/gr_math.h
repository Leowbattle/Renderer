#ifndef GR_MATH_H
#define GR_MATH_H

#include <stdint.h>

float squaref(float x);
float clampf(float x, float a, float b);
float fractf(float x);
float lerpf(float a, float b, float t);
float remapf(float x, float a1, float b1, float a2, float b2);
float deg2rad(float d);

typedef struct {
	float x;
	float y;
} vec2;

typedef struct {
	float x;
	float y;
	float z;
} vec3;

vec3 vec3_add(vec3 a, vec3 b);
vec3 vec3_sub(vec3 a, vec3 b);
float vec3_dot(vec3 a, vec3 b);
vec3 vec3_cross(vec3 a, vec3 b);
float vec3_length(vec3 v);
vec3 vec3_normalize(vec3 v);

typedef struct {
	float x;
	float y;
	float z;
	float w;
} vec4;

typedef struct {
	float m[16];
} mat4;

mat4 mat4_identity();
mat4 mat4_mul(mat4* a, mat4* b);
vec4 mat4_mul_vec4(mat4* m, vec4 v);

mat4 mat4_scale(vec3 v);
mat4 mat4_translate(vec3 v);
mat4 mat4_rotate_zyx(float z, float y, float x);
mat4 mat4_frustum(float l, float r, float b, float t, float n, float f);
mat4 mat4_perspective(float fovY, float aspect, float n, float f);
mat4 mat4_lookat(vec3 eye, vec3 target, vec3 up);

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb;

#endif
