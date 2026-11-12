/*
 * Raven SDK Installer - the project templates, for programs written in C
 * (Raven Designer): the same templates and the same code as the "New Raven
 * Project" menu, so that a project is made the same way everywhere.
 *
 *     int n = raven_templates_load("<SDK>");          // number of templates
 *     raven_template_name(i), raven_template_id(i), raven_template_available(i)
 *     raven_project_create("python", "/home/me/code", NULL, out, sizeof(out), error, sizeof(error));
 *
 * Link with the raven_installer_templates library (C++).
 */
#ifndef RAVEN_TEMPLATES_H
#define RAVEN_TEMPLATES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Reads the templates of an SDK (<sdk>/templates). Returns their number, 0 when there are none. */
int raven_templates_load(const char* sdk_dir);
const char* raven_template_id(int index);
const char* raven_template_name(int index);
const char* raven_template_description(int index);
/** 1 when the template can be used with this SDK (its binding and library are installed). */
int raven_template_available(int index);

/**
 * Makes a project in `parent_dir` (in it when it is empty, in a new folder
 * `name` otherwise; name may be NULL). Returns 1 and the folder of the project
 * in `out_dir`, or 0 and the reason in `error`.
 */
int raven_project_create(const char* template_id, const char* parent_dir, const char* name, char* out_dir, size_t out_size,
	char* error, size_t error_size);

#ifdef __cplusplus
}
#endif

#endif
