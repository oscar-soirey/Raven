/**
 * Raven example - effects
 * The effects that come with Raven, and two effects of the application.
 *
 *   <Border Effect="aurora" EffectColor="#30D158" EffectParams="1,1,1.5"/>
 *   <Border Effect="wave.rfx"/>                     a shader of the application (wave.frag)
 *   <Border Effect="liquid.rfx" EffectBackdrop="true"/>   one that reads what is behind it
 *
 * The .rfx files are effect bundles (the effect compiled for Vulkan, GLES 3 / WebGL 2 and Metal in one file), made from the
 * .frag files next to them:
 *   python3 tools/rfx/rfx.py wave.frag -o wave.rfx --animated
 *   python3 tools/rfx/rfx.py liquid.frag -o liquid.rfx --animated --backdrop
 * A .spv from glslangValidator still works with the Vulkan backend; the Sokol backend needs the bundle.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

/* the application has quit: releases every window, widget and resource of Raven */
static void on_finished(int exit_code, void* user_data) {
	(void)exit_code;
	(void)user_data;
	Rshutdown();
}

int main(void) {
	Rid window;
	if (!example_init("Raven effects")) return 1;
	Rtheme_apply(R_THEME_DARK);
	window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/effects.xml");
	if (window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	/* Rrun is the loop (see examples/hello) */
	if (example_automated(window)) {
		Rshutdown();
		return 0;
	}
	return Rrun(on_finished, NULL);
}
