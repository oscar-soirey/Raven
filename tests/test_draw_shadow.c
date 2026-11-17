/* Public custom-widget shadows use the existing batched Border primitive. */
#include "test.h"
#include "../src/render/draw.h"

static void test_shadow(void) {
	rvn_drawlist_t list;
	RPainter painter;
	const RDrawInstance* instance;
	int i;
	rvn_drawlist_init(&list);
	memset(&painter, 0, sizeof(painter));
	painter.list = &list;
	painter.ox = 10;
	painter.oy = 20;
	painter.scale = 2;
	painter.opacity = 0.5f;
	painter.clip = rvn_box(0, 0, 1000, 1000);
	Rdraw_shadow(&painter, 1, 2, 40, 20, 100, 6, 1, 0.5f, 0, 0.8f);
	CHECK(list.instance_count == 1 && list.command_count == 1);
	instance = &list.instances[0];
	CHECK(instance->flags == R_PRIM_SHADOW);
	CHECK_NEAR(instance->rect[0], 22);
	CHECK_NEAR(instance->rect[1], 44);
	CHECK_NEAR(instance->rect[2], 102);
	CHECK_NEAR(instance->rect[3], 84);
	CHECK_NEAR(instance->border_width, 12);
	for (i = 0; i < 4; i++) CHECK_NEAR(instance->radii[i], 20); /* half the shortest side */
	CHECK(instance->color == 0x660080FFu); /* inherited opacity, unpacked RGBA */
	Rdraw_rect(&painter, 1, 2, 40, 20, 3, 1, 1, 1, 1);
	Rdraw_shadow(&painter, 1, 2, 40, 20, -3, 6, 0, 1, 0, 1);
	CHECK(list.instance_count == 3 && list.command_count == 1);
	CHECK(list.commands[0].type == R_DRAW_INSTANCES && list.commands[0].instance_count == 3);
	CHECK_NEAR(list.instances[2].radii[0], 0);

	rvn_drawlist_reset(&list);
	painter.scale = 1;
	painter.ox = painter.oy = 0;
	painter.opacity = 1;
	painter.clip = rvn_box(0, 0, 20, 20);
	/* The shape is outside the clip, but its blurred footprint reaches it. */
	Rdraw_shadow(&painter, 25, 5, 10, 10, 0, 8, 1, 1, 1, 1);
	CHECK(list.instance_count == 1);
	CHECK_NEAR(list.instances[0].clip[2], 20);
	Rdraw_shadow(&painter, 50, 5, 10, 10, 0, 8, 1, 1, 1, 1);
	CHECK(list.instance_count == 1);
	Rdraw_shadow(NULL, 0, 0, 10, 10, 0, 8, 1, 1, 1, 1);
	Rdraw_shadow(&painter, 0, 0, 0, 10, 0, 8, 1, 1, 1, 1);
	Rdraw_shadow(&painter, 0, 0, 10, -1, 0, 8, 1, 1, 1, 1);
	Rdraw_shadow(&painter, 0, 0, 10, 10, 0, 0, 1, 1, 1, 1);
	Rdraw_shadow(&painter, 0, 0, 10, 10, 0, -1, 1, 1, 1, 1);
	Rdraw_shadow(&painter, 0, 0, 10, 10, 0, 8, 1, 1, 1, 0);
	painter.opacity = 0;
	Rdraw_shadow(&painter, 0, 0, 10, 10, 0, 8, 1, 1, 1, 1);
	CHECK(list.instance_count == 1);
	rvn_drawlist_free(&list);
}

int main(void) {
	RInitDesc desc = {0};
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_TRUE;
	desc.worker_threads = -1;
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	RUN(test_shadow);
	Rshutdown();
	return test_report("test_draw_shadow");
}
