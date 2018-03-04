#include <stdlib.h>
#include <string.h>
#include "triangle.h"
#include "quaternion.h"


// a specification for the triangles
// making up a cube by indexing into
// an array of the 8 points in the cube
// the cube's points are indexed like:
//
//    4----5
//    |\   |\
//    | 0----1
//    | |  | |
//    6-|--7 |
//     \|   \|
//      2----3
//
tidx cidxs[] = {
	// front
	{0, 1, 2},
	{3, 2, 1},
	// back
	{5, 4, 7},
	{6, 7, 4},
	// left
	{4, 0, 6},
	{2, 6, 0},
	// right
	{1, 5, 3},
	{7, 3, 5},
	// top
	{4, 5, 0},
	{1, 0, 5},
	// bottom
	{2, 3, 6},
	{7, 6, 3}
};

// The "identity" triangles that can be formed from
// the four points of a square/rectangle.
//
//   0    |    1   |    2   |    3
//--------|--------|--------|--------
//  2     |  0--1  |     0  |  1--2
//  |\    |   \ |  |    /|  |  | /
//  | \   |    \|  |   / |  |  |/
//  1--0  |     2  |  2--1  |  0
//
// they're indexed to wind clockwise, and the 1
// index is always the square corner.
tri id_tris[] = {
	{1, 0, 0, 0, 0, 0, 0, 1, 0,  1, 1, 0, 1, 0, 0},
	{0, 1, 0, 1, 1, 0, 1, 0, 0,  0, 0, 1, 0, 1, 1},
	{1, 1, 0, 1, 0, 0, 0, 0, 0,  1, 0, 1, 1, 0, 1},
	{0, 0, 0, 0, 1, 0, 1, 1, 0,  0, 1, 0, 0, 1, 0}
};


// convenience method to print out a triangle
void print_tri(tri t) {
	printf("(%f, %f, %f) ", t.p[0].x, t.p[0].y, t.p[0].z);
	printf("(%f, %f, %f) ", t.p[1].x, t.p[1].y, t.p[1].z);
	printf("(%f, %f, %f)\n", t.p[2].x, t.p[2].y, t.p[2].z);
}

// convenience method to print out a point/vector
void print_pt(const char *txt, pt p) {
	printf("%s (%f, %f, %f)\n", txt, p.x, p.y, p.z);
}

// used by qsort() to sort points clockwise around a center point
pt tmp_center;
pt tmp_normal;
int cw_cmp (const void * a, const void * b) {
	pt *p1 = (pt *)a;
	pt *p2 = (pt *)b;
	pt cross = v3_cross(v3_sub(*p1, tmp_center), v3_sub(*p2, tmp_center));
	float dot = v3_dot(cross, tmp_normal);
	if (dot > 0.001f) {
		return -1;
	} else if (dot < -0.001f) {
		return 1;
	}
	return 0;
}

// given a list of points that are created by slicing
// a bunch of triangles in the same space by the same
// plane, reduce those points by getting rid of ones
// that are the same and ones that fall on a line
// between two other points.
// @src - the list of points
// @dst - a list to put the reduced set of points
// @scnt - the number of points in @src
// returns the number of points in @dst
int reduce_pts(pt *src, pt *dst, int scnt) {
	pt idst[scnt];
	int icnt = 0;
	for (int i=0; i<scnt; i++) {
		bool use = true;
		for (int j=0; j<icnt; j++) {
			float dist = distance(src[i], idst[j]);
			if (dist < 0.01f) {
				use = false;
				break;
			}
		}
		if (use) {
			idst[icnt] = src[i];
			icnt++;
		}
	}
	//printf("there were %d intermediate points\n", icnt);
	int ocnt = 0;
	for (int i=0; i<icnt; i++) {
		bool use = true;
		for (int j=0; j<icnt; j++) {
			if (j == i) continue;
			for (int k=0; k<icnt; k++) {
				if (k == i || k == j) continue;
				float d = line_distance(idst[j], idst[k], idst[i]);
				if (d < 0.01f) {
					float d1 = distance(idst[j], idst[k]);
					float d2 = distance(idst[i], idst[j]);
					float d3 = distance(idst[i], idst[k]);
					if (d2 < d1 && d3 < d1) {
						use = false;
						break;
					}
				}
			}
			if (!use) break;
		}
		if (use) {
			dst[ocnt] = idst[i];
			ocnt++;
		}
	}
	return ocnt;
}

// Slice a list of triangles where they intersects with a plane
// and throw out the parts facing away from the plane's normal vector.
// Return a list of triangles that will replace the sliced triangles
// and a list of new points created by slicing into the edges of
// the triangles.
// @src - the triangle to slice
// @dst - a buffer to put the triangles resulting from the slice
// @dpnts - a buffer to put the points created
// @scnt - the number of triangles in @src
// @pp - a point on the plane that's used to clip
// @pnorm - the normal vector to the clip plane
// returns the number of triangles added to @dst
int slice(tri *src, tri *dst, pt *dpts, int scnt, pt pp, pt pnorm) {
	tri_clip_buf tb;
	tb.out = dst;
	tb.opts = dpts;
	// clip all the triangles and collect the resulting triangles
	// as well as the intersection points
	int didx = 0;
	int dpidx = 0;
	for (int i=0; i<scnt; i++) {
		tb.oidx = didx;
		tb.opidx = dpidx;
		clip(src[i], pp, pnorm, &tb);
		didx += tb.ocnt;
		dpidx += tb.opcnt;
	}
	printf("there were %d intersect points\n", dpidx);
	// reduce the intersection points by getting rid of points that
	// are either really close to each other or fall on a line between
	// other points
	pt apts[dpidx];
	int acnt = reduce_pts(dpts, apts, dpidx);
	printf("after reduction there are %d intersect points\n", acnt);
	// sort the points so that they are listed in a clockwise direction.
	// we do that by first finding the center point of all the points...
	tmp_center = vec3(0, 0, 0);
	for (int i=0; i<acnt; i++) {
		tmp_center = v3_add(tmp_center, apts[i]);
	}
	tmp_center = v3_divs(tmp_center, (float)acnt);
	// ...then we sort the list of points based on the cross product
	// of any two points with respect to the center. We use the dot product
	// between that cross product and the normal of the slicing plane to
	// determine which direction they wind with respect to the plane.
	tmp_normal = pnorm;
	qsort(apts, (size_t)acnt, sizeof(pt), cw_cmp);
	/*
	for (int i=0; i<acnt; i++) {
		print_pt(apts[i]);
	}
	*/
	// now we make triangles out of the list of points. for now we just handle
	// the two easy cases of 3 or 4 points.
	if (acnt == 3) {
		tri fill = {apts[0], apts[1], apts[2]};
		dst[didx++] = fill;
	} else if (acnt == 4) {
		tri fill1 = {apts[0], apts[1], apts[2]};
		tri fill2 = {apts[2], apts[3], apts[0]};
		dst[didx++] = fill1;
		dst[didx++] = fill2;
	} else {
		printf("ERROR: we had too many points (%d) to fill in the intersection\n",acnt);
	}
	return didx;
}

int plane_tris(pt pp, pt *ps, float scale, tri *dst) {
	tri d0 = {v3_add(v3_muls(ps[0], scale), pp), v3_add(v3_muls(ps[1], scale), pp), v3_add(v3_muls(ps[2], scale), pp)};
	tri d1 = {v3_add(v3_muls(ps[2], scale), pp), v3_add(v3_muls(ps[3], scale), pp), v3_add(v3_muls(ps[0], scale), pp)};
	dst[0] = d0;
	dst[1] = d1;
	return 2;
}

pt plane_axis(pt pp, pt pnorm, pt *ps, int dir) {
	switch(dir) {
		case DIR_U: return v3_norm(ps[0]);
		case DIR_R: return v3_norm(ps[1]);
		case DIR_D: return v3_norm(ps[2]);
		case DIR_L: return v3_norm(ps[3]);
		case DIR_F: return v3_norm(pnorm);
		case DIR_B: return v3_norm(v3_muls(pnorm, -1));
		default: return ps[0];
	}
}

// calculate the normal vector of a triangle that's wound clockwise
pt tri_normal(tri tri) {
	return v3_norm(v3_cross(v3_sub(tri.p[0], tri.p[1]), v3_sub(tri.p[2], tri.p[1])));
}

int inc_tri_idx(int idx, bool cw) {
	int dir = (cw) ? 1 : -1;
	int idxp = idx + dir;
	if (idxp > 2) idxp = 0; else if (idxp < 0) idxp = 2;
	return idxp;
}

// Rotates one vertex of a triangle around the axis formed by
// the other two verices.
// @tri - the triangle
// @idx - the index (0, 1, or 2) of the vertex to rotate
// @cw - tells the function what direction the axis should point
// relative to the one that's rotating
// @rad - how far to rotate the point
void rotate_tri(tri *tri, int idx, bool cw, float rad) {
	int ax_idx0 = inc_tri_idx(idx, cw);
	int ax_idx1 = inc_tri_idx(ax_idx0, cw);
	pt ax = v3_norm(v3_sub(tri->p[ax_idx0], tri->p[ax_idx1]));

	pt rp = v3_sub(tri->p[idx], tri->p[ax_idx0]);
	float len = v3_length(rp);
	rp = v3_norm(rp);
	versor pcurv = {0, rp.x, rp.y, rp.z};
	versor protv = quat_from_axis_rad(rad, ax.x ,ax.y, ax.z);
	versor protv_prime = quat_from_axis_rad(rad, -ax.x ,-ax.y, -ax.z);
	versor pv = q_mul(q_mul(protv, pcurv), protv_prime);
	tri->p[idx].x = (pv.q[1] * len) + tri->p[ax_idx0].x;
	tri->p[idx].y = (pv.q[2] * len) + tri->p[ax_idx0].y;
	tri->p[idx].z = (pv.q[3] * len) + tri->p[ax_idx0].z;
}

pt rotate(pt pnorm, pt *ps, pt ax, float rad) {
	versor curv = {0, pnorm.x, pnorm.y, pnorm.z};
	versor rotv = quat_from_axis_rad(0.02f, ax.x ,ax.y, ax.z);
	versor rotv_prime = quat_from_axis_rad(0.02f, -ax.x ,-ax.y, -ax.z);
	versor v = q_mul(q_mul(rotv, curv), rotv_prime);
	pnorm.x = v.q[1];
	pnorm.y = v.q[2];
	pnorm.z = v.q[3];
	pnorm = v3_norm(pnorm);
	for (int i=0; i<4; i++) {
		versor pcurv = {0, ps[i].x, ps[i].y, ps[i].z};
		versor protv = quat_from_axis_rad(rad, ax.x ,ax.y, ax.z);
		versor protv_prime = quat_from_axis_rad(rad, -ax.x ,-ax.y, -ax.z);
		versor pv = q_mul(q_mul(protv, pcurv), protv_prime);
		ps[i].x = pv.q[1];
		ps[i].y = pv.q[2];
		ps[i].z = pv.q[3];
		//printf("ps[%d]=(%f,%f,%f)\n", i, ps[i].x, ps[i].y, ps[i].z);
	}
	return pnorm;
}

// convenience function to add a triangle to a buffer
// @tri - the triangle to add
// @btri - the buffer to add to
// @idx - the index at which to add the triangle
// returns the index for the next triangle to add
int add_tri(tri t, tri *btri, int idx) {
	memcpy(&btri[idx], &t, sizeof(tri));
	return idx+1;
}

tri flipped_tri(tri t) {
	tri nt = {
		t.p[2].x, t.p[2].y, t.p[2].z,
		t.p[1].x, t.p[1].y, t.p[1].z,
		t.p[0].x, t.p[0].y, t.p[0].z,
		t.uv[2].u, t.uv[2].v,
		t.uv[1].u, t.uv[1].v,
		t.uv[0].u, t.uv[0].v
	};
	return nt;
}

// flip the winding order of a triangle
void flip_wind(tri *t) {
	float hx = t->p[2].x;
	float hy = t->p[2].y;
	float hz = t->p[2].z;
	float hu = t->uv[2].u;
	float hv = t->uv[2].v;

	t->p[2].x = t->p[0].x;
	t->p[2].y = t->p[0].y;
	t->p[2].z = t->p[0].z;
	t->uv[2].u = t->uv[0].u;
	t->uv[2].v = t->uv[0].v;

	t->p[0].x = hx;
	t->p[0].y = hy;
	t->p[0].z = hz;
	t->uv[0].u = hu;
	t->uv[0].v = hv;

	for (int i=0; i<3; i++) {
		t->p[i].x = roundf(t->p[i].x);
		t->p[i].y = roundf(t->p[i].y);
		t->p[i].z = 0;
	}
}

// set the uvs of a triangle based on one of the identity triangles
// @t - the triangle to set uvs on
// @which - which identity triangle the triangle should be based on
// @sx - the staring x point (0 to 1) on the sprite sheet where the uv begins
// @sy - the staring y point (0 to 1) on the sprite sheet where the uv begins
// @sw - the width of the sprite (0 to 1) on the sprite sheet
// @sh - the height of the sprite (0 to 1) on the sprite sheet
void set_tri_sprite_uv(tri *t, int which, float sx, float sy, float sw, float sh) {
	tri uvt = id_tris[which];
	t->uv[0].u = (uvt.uv[0].u * sw) + sx;
	t->uv[0].v = (uvt.uv[0].v * sh) + sy;
	t->uv[1].u = (uvt.uv[1].u * sw) + sx;
	t->uv[1].v = (uvt.uv[1].v * sh) + sy;
	t->uv[2].u = (uvt.uv[2].u * sw) + sx;
	t->uv[2].v = (uvt.uv[2].v * sh) + sy;
}

// set the positions of the points on a triangle based on  one of the identity triangles
// @t - the triangle to set positions on
// @which - which identity triangle the triangle should be based on
// @sx - the x pos of the bottom left corner where the triangle should start
// @sy - the y pos of the bottom left corner where the triangle should start
// @z - the z position of the triangle
void set_tri_pos(tri *t, int which, float sx, float sy, float z) {
	tri uvt = id_tris[which];
	t->p[0].x = sx + uvt.p[0].x;
	t->p[0].y = sy + uvt.p[0].y;
	t->p[0].z = z;
	t->p[1].x = sx + uvt.p[1].x;
	t->p[1].y = sy + uvt.p[1].y;
	t->p[1].z = z;
	t->p[2].x = sx + uvt.p[2].x;
	t->p[2].y = sy + uvt.p[2].y;
	t->p[2].z = z;
}

// a simple int-based version of set_tri_pos() for tile-based situations
void set_tri_at(tri *t, int which, int x, int y) {
	set_tri_pos(t, which, (float)x, (float)y, 0.0f);
}

// makes a cube out of 8 points specified in this order:
//
//    4----5
//    |\   |\
//    | 0----1
//    | |  | |
//    6-|--7 |
//     \|   \|
//      2----3
//
// the cube will be a list of 12 triangles.
// @pts - the 8 points specifying the cube
// @tris - a buffer in which to output the triangles making up the cube
// @sidx - where in the buffer to start outputting the triangles
// returns the index where the next thing would be added
// (i.e. the new size of the buffer after adding the triangles)
int make_cube(pt *pts, tri *tris, int sidx) {
	for (int i=0; i<12; i++) {
		int idx = i + sidx;
		tris[idx].p[0] = pts[cidxs[i].pidx[0]];
		tris[idx].p[1] = pts[cidxs[i].pidx[1]];
		tris[idx].p[2] = pts[cidxs[i].pidx[2]];
		if ((i % 2) == 0) {
			tris[idx].uv[0].u = 0;
			tris[idx].uv[0].v = 0;
			tris[idx].uv[1].u = 1;
			tris[idx].uv[1].v = 0;
			tris[idx].uv[2].u = 0;
			tris[idx].uv[2].v = 1;
		} else {
			tris[idx].uv[0].u = 1;
			tris[idx].uv[0].v = 1;
			tris[idx].uv[1].u = 0;
			tris[idx].uv[1].v = 1;
			tris[idx].uv[2].u = 1;
			tris[idx].uv[2].v = 0;
		}
	}
	return sidx + 12;
}

// Build a unit cube from its center point.
// The resulting cube is square with the x, y, and z axes
// @x - the x position of the cube's center
// @y - the y position of the cube's center
// @z - the z position of the cube's center
// @tris - a buffer to hold the cube's constituent triangles
// @sidx - where in the buffer to start outputting the triangles
// returns the index where the next thing would be added
// (i.e. the new size of the buffer after adding the triangles)
int cube_at(float x, float y, float z, tri *tris, int sidx) {
	pt cube[] = {
		{x-0.5f, y+0.5f, z+0.5f},
		{x+0.5f, y+0.5f, z+0.5f},
		{x-0.5f, y-0.5f, z+0.5f},
		{x+0.5f, y-0.5f, z+0.5f},
		{x-0.5f, y+0.5f, z-0.5f},
		{x+0.5f, y+0.5f, z-0.5f},
		{x-0.5f, y-0.5f, z-0.5f},
		{x+0.5f, y-0.5f, z-0.5f}
	};
	return make_cube(cube, tris, sidx);
}

// distance between two points
float distance(pt p1, pt p2) {
	float xx = p1.x - p2.x;
	float yy = p1.y - p2.y;
	float zz = p1.z - p2.z;
	return sqrtf((xx*xx)+(yy*yy)+(zz*zz));
}

// the shortest distance between a point and a line formed
// by two other points.
// @lp1 and @lp2 - two points making up a line
// @p - the point to measure the distance from
// returns the shortest distance from @p to the line formed by @lp1 and @lp2
float line_distance(pt lp1, pt lp2, pt p) {
	pt num = v3_cross(v3_sub(lp2, lp1), v3_sub(lp1, p));
	pt den = v3_sub(lp2, lp1);
	return v3_length(num) / v3_length(den);
}

// find the point on a line that intersects with a plane.
// @v1 and @v2 - the endpoints of the line
// @pp - a point on the plane
// @pnorm - the normal vector of the plane
pt intersect(pt v1, pt v2, pt pp, pt pnorm) {
	pt ray = v3_sub(v2, v1);
	float cosA = v3_dot(ray, pnorm);
	float deltaD = v3_dot(pp, pnorm) - v3_dot(v1, pnorm);
	float length = deltaD / cosA;
	pt np = v3_muls(ray, length);
	return v3_add(np, v1);
}

// clip a triangle where it intersects with a plane
// @t - the triangle to clip
// @pp - a point on the plane that's used to clip
// @pnorm - the normal vector to the clip plane
// @tb - a buffer to hold the resulting triangles and new points
int clip(tri t, pt pp, pt pnorm, tri_clip_buf *tb) {
	tri *out = tb->out;
	int oidx = tb->oidx;
	pt *opts = tb->opts;
	int opidx = tb->opidx;
	tb->opcnt = 0;

	float ds[3];
	int cnt = 0;
	int ridx = 0;
	int cidx = 0;
	for (int i=0; i<3; i++) {
		ds[i] = v3_dot(pnorm, v3_sub(t.p[i], pp));
		if (ds[i] > 0.0001f) {
			ridx = i;
			cnt++;
		} else {
			cidx = i;
		}
	}
	if (cnt == 0) {
		// everything was sliced out
		tb->ocnt = 0;
	} else if (cnt == 1) {
		// one point remains
		//          2
		//      \  . .
		//       \.   .
		//       .\    .
		//      .  \    .
		//     1....\....0
		//           \
			//
		// we're going to output 1 triangle
		// it will be the 0/1 intersect, 1, and the 1/2 intersect
		// those are in order - we need to have clockwise wrapping
		int pidx0 = ridx-1;
		if (pidx0 < 0) pidx0 = 2;
		int pidx1 = ridx;
		int pidx2 = ridx + 1;
		if (pidx2 > 2) pidx2 = 0;
		out[oidx].p[0] = intersect(t.p[pidx0], t.p[pidx1], pp, pnorm);
		out[oidx].p[1] = t.p[pidx1];
		out[oidx].p[2] = intersect(t.p[pidx1], t.p[pidx2], pp, pnorm);
		tb->ocnt = 1;

		opts[opidx++] = out[oidx].p[0];
		opts[opidx++] = out[oidx].p[2];
		tb->opcnt += 2;
	} else if (cnt == 2) {
		// two points remain
		// 0 and 1 are the points that remain.
		// 2 is the one that will be sliced
		//          2
		//         . .
		//        .   .    normal
		//  -----x-----x-----|-----
		//      .       .    v
		//     1.........0
		//
		// we're going to output 2 triangles
		// the first will be the 2/0 intersect, 0 and 1
		// the second will be 1, the 1/2 intersect and the 2/0 intersect
		// those are in order - we need to have clockwise wrapping
		int pidx2 = cidx;
		int pidx0 = cidx+1;
		if (pidx0 > 2) pidx0 = 0;
		int pidx1 = pidx0+1;
		if (pidx1 > 2) pidx1 = 0;
		// first triangle
		out[oidx].p[0] = intersect(t.p[pidx2], t.p[pidx0], pp, pnorm);
		out[oidx].p[1] = t.p[pidx0];
		out[oidx].p[2] = t.p[pidx1];
		// second triangle
		out[oidx+1].p[0] = t.p[pidx1];
		out[oidx+1].p[1] = intersect(t.p[pidx2], t.p[pidx1], pp, pnorm);
		out[oidx+1].p[2] = intersect(t.p[pidx2], t.p[pidx0], pp, pnorm);
		tb->ocnt = 2;

		opts[opidx++] = out[oidx].p[0];
		opts[opidx++] = out[oidx+1].p[1];
		opts[opidx++] = out[oidx+1].p[2];
		tb->opcnt += 3;
	} else if (cnt == 3) {
		// the whole triangle remains
		out[oidx] = t;
		tb->ocnt = 1;
	}
	return tb->ocnt;
}
