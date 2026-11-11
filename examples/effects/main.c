/**
 * Raven example - effects
 * The effects that come with Raven, and two effects of the application.
 *
 *   <Border Effect="aurora" EffectColor="#30D158" EffectParams="1,1,1.5"/>
 *   <Border Effect="wave.spv"/>                     a shader of the application (wave.frag)
 *   <Border Effect="liquid.spv" EffectBackdrop="true"/>   one that reads what is behind it
 *
 * The .spv files are compiled from the .frag files next to them:
 *   glslangValidator -V -I../../include/raven/shaders wave.frag -o wave.spv
 * docs/shaders.html is the guide.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#if defined(__EMSCRIPTEN__)
#include <raven/raven_emscripten.h>

/* Keep the native layout and its effect colors/parameters. WebGL receives the
 * same application effects as GLSL functions, rather than SPIR-V files. */
static void web_prepare_effects(Rid node) {
	const char* effect = Rxml_node_get_attribute(node, "Effect");
	int i, count;
	if (effect && (strcmp(effect, "wave.spv") == 0 ||
	               strcmp(effect, "liquid.spv") == 0)) {
		Rxml_node_set_attribute(node, "Name", effect);
		Rxml_node_set_attribute(node, "Effect", "");
	}
	count = Rxml_node_get_child_count(node);
	for (i = 0; i < count; i++)
		web_prepare_effects(Rxml_node_get_child(node, i));
}

static int web_attach_effect(Rid window, const char* name,
                             const char* path, Ruint flags) {
	size_t size;
	char* source = Rfile_read(path, &size);
	const char* body = source;
	Rid border = Rwidget_find(window, name);
	Rid shader;
	if (!source || border == R_INVALID_ID) {
		Rmemory_free(source);
		return R_FALSE;
	}
	/* The native files start with version, extension and include directives.
	 * Rweb_shader_create supplies their GLSL ES header and Raven effect ABI. */
	while (*body == '#') {
		body = strchr(body, '\n');
		if (!body) {
			Rmemory_free(source);
			return R_FALSE;
		}
		body++;
	}
	shader = Rweb_shader_create(body, flags);
	Rmemory_free(source);
	if (shader == R_INVALID_ID) return R_FALSE;
	Rborder_set_effect(border, shader, NULL);
	return R_TRUE;
}

static Rid web_create_effects(void) {
	Rid document = Rxml_document_load(RAVEN_EXAMPLE_DIR "/effects.xml");
	Rid window;
	if (document == R_INVALID_ID) return R_INVALID_ID;
	web_prepare_effects(Rxml_document_get_root(document));
	window = Rxml_document_create_window(document);
	Rxml_document_destroy(document);
	if (window == R_INVALID_ID) return R_INVALID_ID;
	if (!web_attach_effect(window, "wave.spv",
	                       RAVEN_EXAMPLE_DIR "/wave.frag", R_SHADER_ANIMATED) ||
	    !web_attach_effect(window, "liquid.spv",
	                       RAVEN_EXAMPLE_DIR "/liquid.frag",
	                       R_SHADER_ANIMATED | R_SHADER_BACKDROP)) {
		Rwindow_destroy(window);
		return R_INVALID_ID;
	}
	return window;
}
#endif

int main(void) {
	Rid window;
	if (!example_init("Raven effects")) return 1;
	Rtheme_apply(R_THEME_DARK);
#if defined(__EMSCRIPTEN__)
	window = web_create_effects();
#else
	window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/effects.xml");
#endif
	if (window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	example_run(window, NULL, NULL);
	return 0;
}
