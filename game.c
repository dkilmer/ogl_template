#include <unistd.h>
#include "window.h"
#include "game.h"
#include "misc_util.h"
#include "quaternion.h"
#define MATH_3D_IMPLEMENTATION
#include "triangle.h"
#include "render_util.h"
#include "easing.h"

void run() {
	char pwd[1024];
	getcwd(pwd, 1024);
	printf("pwd: %s\n", pwd);

	screen_w = 800;
	screen_h = 600;
	if (!init_window("ogl", screen_w, screen_h)) return;
	print_sdl_gl_attributes();

	float unit_w = (float)screen_w / 100.0f;
	float unit_h = (float)screen_h / 100.0f;
	float half_w = unit_w / 2.0f;
	float half_h = unit_h / 2.0f;
	float aspect = unit_w / unit_h;
	float far = 100.0f;
	float fov = 45.0f;
	float rads = (fov / 2.0f) * (float)ONE_DEG_IN_RAD;
	float tang = tanf(rads);
	float cam_z = (half_h / tang);
	//printf("cam_z is %f\n",cam_z);
	float near = cam_z - 4.0f;
	// perspective camera
	vec3_t cpos = { half_w, half_h, cam_z };
	vec3_t cat = {half_w, half_h, 0.0f};
	vec3_t up = {0.0f, 1.0f, 0.0f};
	mat4_t p_mat = m4_perspective(fov, aspect, near, far);
	mat4_t m_mat = m4_identity();
	mat4_t v_mat = m4_look_at(cpos, cat, up);
	mat4_t vp_mat = m4_mul(m4_mul(p_mat, v_mat), m_mat);

	/*
	// ortho camera
	vec3_t cpos = { half_w, half_h, 20.0f};
	vec3_t cat = {half_w, half_h, 0.0f};
	vec3_t up = {0.0f, 1.0f, 0.0f};
	mat4_t p_mat = m4_ortho(-half_w, half_w, -half_h, half_h, 30.0f, 8.8f);
	mat4_t m_mat = m4_identity();
	mat4_t v_mat = m4_look_at(cpos, cat, up);
	mat4_t vp_mat = m4_mul(m4_mul(p_mat, v_mat), m_mat);
	*/

	render_def buf;
	buf.num_bufs = 3;
	buf.num_items = (GLuint)(3000);
	//buf.verts_per_item = 3;
	//alloc_buffers(&buf);

	char vsname[1024];
	char fsname[1024];
	char texname[1024];
	sprintf(vsname, "%s/../shaders/vert.glsl", pwd);
	sprintf(fsname, "%s/../shaders/frag.glsl", pwd);
	sprintf(texname, "%s/../res/pencil-512.png", pwd);
	setup_render_def(&buf,
		 GL_TRIANGLES,
	   vsname,
	   fsname,
	   (GLfloat *)&vp_mat,
	   texname
	);

	bool kdown[NUM_KEYS];
	bool kpress[NUM_KEYS];
	for (int i=0; i<NUM_KEYS; i++) {
		kdown[i] = false;
		kpress[i] = false;
	}
	int key_map[NUM_KEYS];
	set_default_key_map(key_map);
	mouse_input mouse;

	GLenum err;
	glUseProgram(buf.shader);
	GLint lpUnif = glGetUniformLocation(buf.shader, "light_pos");
	//glUniform3f(lpUnif, -0.5f, 0.25f, 1.0f);
	pt lp = {0, 0, 1};
	glUniform3f(lpUnif, lp.x, lp.y, lp.z);
	err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("lpUnif is %d: %d\n", lpUnif, err);
	}

	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, screen_w, screen_h);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	clr c = { 0.8f, 0.2f, 0.2f, 1.0f };
	tri *btri = (tri *)malloc(sizeof(tri) * 3000);

	tri t1, t2;
	set_tri_pos(&t1, 0, 3.5f, 2.5f, 0);
	set_tri_sprite_uv(&t1, 0, 0, 0, 1, 1);
	set_tri_pos(&t2, 1, 3.5f, 2.5f, 0);
	set_tri_sprite_uv(&t2, 1, 0, 0, 1, 1);

	int icnt = 0;
	icnt = add_tri(t1, btri, icnt);
	icnt = add_tri(t2, btri, icnt);

	int frame = 0;
	bool loop = true;
	while (loop) {
		get_input(kdown, kpress, key_map, &mouse);
		if (kpress[KEY_QUIT]) {
			loop = false;
		}
		glBindVertexArray(buf.vao);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int i=0; i<icnt; i++) {
			render_tri(&buf, &(btri[i]), &c);
		}

		render_buffer(&buf);

		swap_window();
		frame = (frame + 1) % 60;
		render_advance(&buf);
	}

	free_render_def(&buf);
	free(btri);
}
