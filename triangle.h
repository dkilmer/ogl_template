#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <glad/glad.h>
#include <stdbool.h>
#include "math_3d.h"

// constants for the cardinal directions
#define DIR_N  0
#define DIR_L  1
#define DIR_U  2
#define DIR_R  4
#define DIR_D  8
#define DIR_F  16
#define DIR_B  32

// use math_3d's version of a point
typedef vec3_t pt;

// struct representing a UV point (2d)
typedef struct {
	GLfloat u;
	GLfloat v;
} uv_pt;

// struct representing a color
typedef struct {
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
} clr;

// A struct to hold all the things OpenGL will need to render
// a vertex. The color is normalized unsigned bytes, the normal
// is a normalized 2_10_10_10 signed int, and the UV is a normalized
// short int. All that to save buffer space.
typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLubyte r;
	GLubyte g;
	GLubyte b;
	GLubyte a;
	GLuint n;
	GLushort u;
	GLushort v;
} vbo_pt;

// A struct to hold a triangle. Three points and three UVs
typedef struct {
	pt p[3];
	uv_pt uv[3];
} tri;

// A struct to hold an OpenGL-renderable triangle
typedef struct {
	vbo_pt p[3];
} vbo_tri;

/*
typedef struct {
	pt p[4];
} t_quad;

typedef struct {
	GLuint e[6];
} t_quad_el;

typedef struct {
	uv_pt p[4];
} t_uv;
*/

// a struct to hold the indices of a triangle
typedef struct {
	int pidx[3];
} tidx;

// an internal struct to hold the results of clipping triangles.
// it's used by the clip() function.
// @out - list of triangles to add to when doing a clip
// @ocnt - the number of triangles in @out
// @oidx - the index in @out to start putting triangles when clipping
// @opts - list of points added by bisecting triangle edges
// @opcnt - the number of points in @opts
// @opidx - the index in @opts to start putting points when clipping
typedef struct {
	tri *out;
	int ocnt;
	int oidx;
	pt *opts;
	int opcnt;
	int opidx;
} tri_clip_buf;

void print_tri(tri t);
void print_pt(const char *txt, pt p);
int slice(tri *src, tri *dst, pt *dpts, int scnt, pt pp, pt pnorm);
int plane_tris(pt pp, pt *ps, float scale, tri *dst);
pt plane_axis(pt pp, pt pnorm, pt *ps, int dir);
pt tri_normal(tri tri);
void rotate_tri(tri *tri, int idx, bool cw, float rad);
pt rotate(pt pnorm, pt *ps, pt ax, float rad);
int add_tri(tri t, tri *btri, int idx);
tri flipped_tri(tri t);
void flip_wind(tri *t);
void set_tri_sprite_uv(tri *t, int which, float sx, float sy, float sw, float sh);
void set_tri_pos(tri *t, int which, float sx, float sy, float z);
void set_tri_at(tri *t, int which, int x, int y);
int make_cube(pt *pts, tri *tris, int sidx);
int cube_at(float x, float y, float z, tri *tris, int sidx);
float distance(pt p1, pt p2);
float line_distance(pt lp1, pt lp2, pt p);
pt intersect(pt v1, pt v2, pt pp, pt pnorm);
int clip(tri t, pt pp, pt pnorm, tri_clip_buf *tb);

#endif //TRIANGLE_H
