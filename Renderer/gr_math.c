#define _USE_MATH_DEFINES
#include <math.h>

#include "gr_math.h"

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

float remapf(float x, float a1, float b1, float a2, float b2) {
	float t = (x - a1) / (b1 - a1);
	return a2 + t * (b2 - a2);
}

float deg2rad(float d) {
	return d * M_PI / 180.f;
}

vec3 vec3_add(vec3 a, vec3 b) {
	return (vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

vec3 vec3_sub(vec3 a, vec3 b) {
	return (vec3) { a.x - b.x, a.y - b.y, a.z - b.z };
}

float vec3_dot(vec3 a, vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3_cross(vec3 a, vec3 b) {
	return (vec3) {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

float vec3_length(vec3 v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 vec3_normalize(vec3 v) {
	float l = vec3_length(v);
	return (vec3) { v.x / l, v.y / l, v.z / l };
}

mat4 mat4_identity() {
	return (mat4) {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
}

mat4 mat4_mul(mat4* a, mat4* b) {
	float* A = a->m;
	float* B = b->m;
	return (mat4) {
		A[0] * B[0] + A[1] * B[4] + A[2] * B[8] + A[3] * B[12],
		A[0] * B[1] + A[1] * B[5] + A[2] * B[9] + A[3] * B[13],
		A[0] * B[2] + A[1] * B[6] + A[2] * B[10] + A[3] * B[14],
		A[0] * B[3] + A[1] * B[7] + A[2] * B[11] + A[3] * B[15],

		A[4] * B[0] + A[5] * B[4] + A[6] * B[8] + A[7] * B[12],
		A[4] * B[1] + A[5] * B[5] + A[6] * B[9] + A[7] * B[13],
		A[4] * B[2] + A[5] * B[6] + A[6] * B[10] + A[7] * B[14],
		A[4] * B[3] + A[5] * B[7] + A[6] * B[11] + A[7] * B[15],

		A[8] * B[0] + A[9] * B[4] + A[10] * B[8] + A[11] * B[12],
		A[8] * B[1] + A[9] * B[5] + A[10] * B[9] + A[11] * B[13],
		A[8] * B[2] + A[9] * B[6] + A[10] * B[10] + A[11] * B[14],
		A[8] * B[3] + A[9] * B[7] + A[10] * B[11] + A[11] * B[15],

		A[12] * B[0] + A[13] * B[4] + A[14] * B[8] + A[15] * B[12],
		A[12] * B[1] + A[13] * B[5] + A[14] * B[9] + A[15] * B[13],
		A[12] * B[2] + A[13] * B[6] + A[14] * B[10] + A[15] * B[14],
		A[12] * B[3] + A[13] * B[7] + A[14] * B[11] + A[15] * B[15],
	};
}

vec4 mat4_mul_vec4(mat4* m, vec4 v) {
	float* M = m->m;
	return (vec4) {
		v.x * M[0] + v.y * M[1] + v.z * M[2] + v.w * M[3],
		v.x * M[4] + v.y * M[5] + v.z * M[6] + v.w * M[7],
		v.x * M[8] + v.y * M[9] + v.z * M[10] + v.w * M[11],
		v.x * M[12] + v.y * M[13] + v.z * M[14] + v.w * M[15],
	};
}

mat4 mat4_translate(vec3 v) {
	return (mat4) {
		1, 0, 0, v.x,
		0, 1, 0, v.y,
		0, 0, 1, v.z,
		0, 0, 0, 1,
	};
}

mat4 mat4_rotate_zyx(float z, float y, float x) {
	float cosa = cosf(z);
	float sina = sinf(z);
	float cosb = cosf(y);
	float sinb = sinf(y);
	float cosc = cosf(x);
	float sinc = sinf(x);

	return (mat4) {
		cosa * cosb, cosa * sinb * sinc - sina * cosc, cosa * sinb * cosc + sina * sinc, 0,
		sina * cosb, sina * sinb * sinc + cosa * cosc, sina * sinb * cosc - cosa * sinc, 0,
		-sinb, cosb * sinc, cosb * cosc, 0,
		0, 0, 0, 1,
	};
}

mat4 mat4_frustum(float l, float r, float b, float t, float n, float f) {
	return (mat4) {
		2 * n / (r - l), 0, -(r + l) / (r - l), 0,
		0, 2 * n / (t - b), -(t + b) / (t - b), 0,
		0, 0, f / (f - n), -f * n / (f - n),
		0, 0, 1, 0,
	};
}

mat4 mat4_perspective(float fovY, float aspect, float n, float r) {
	float t = n * tanf(fovY / 2);
	return mat4_frustum(-aspect * t, aspect * t, -t, t, n, r);
}

mat4 mat4_lookat(vec3 eye, vec3 target, vec3 up) {
	vec3 forward = vec3_normalize(vec3_sub(target, eye));
	vec3 right = vec3_normalize(vec3_cross(forward, up));
	vec3 up2 = vec3_cross(right, forward);

	return (mat4) {
		right.x, right.y, right.z, -vec3_dot(right, eye),
		up2.x, up2.y, up2.z, -vec3_dot(up2, eye),
		-forward.x, -forward.y, -forward.z, -vec3_dot(forward, eye),
		0, 0, 0, 1,
	};
}
