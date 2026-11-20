/**
 * Raven - platform/spell_unix.c
 * The spell checker of Linux and the BSDs (platform/spell.h), for the X11
 * and Wayland builds alike:
 *
 *   Enchant 2 (libenchant-2.so.2), the library GNOME, GTK and many editors
 *   use: it picks the backend the user chose (Hunspell, Nuspell, Aspell...)
 *   and keeps the words the user adds;
 *
 *   else Hunspell itself (libhunspell), with the dictionaries installed in
 *   the usual places (/usr/share/hunspell, /usr/share/myspell...) and in
 *   $DICPATH. Words learned then last for the session only.
 *
 * As libdbus for the screen readers (atspi.c), both are loaded when first
 * needed: Raven links with neither, and has no checker where none is
 * installed. Only the few functions called are declared here.
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "spell.h"

#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/types.h>

#define RVN_UNIX_SPELL_DICTS 8

typedef struct EnchantBroker EnchantBroker;
typedef struct EnchantDict EnchantDict;
typedef void (*rvn_enchant_describe_fn)(const char* tag, const char* provider_name, const char* provider_desc,
	const char* provider_file, void* user_data);
typedef struct Hunhandle Hunhandle;

static struct {
	void* library;
	/* Enchant */
	EnchantBroker* broker;
	EnchantBroker* (*broker_init)(void);
	void (*broker_free)(EnchantBroker*);
	EnchantDict* (*broker_request_dict)(EnchantBroker*, const char*);
	void (*broker_free_dict)(EnchantBroker*, EnchantDict*);
	void (*broker_list_dicts)(EnchantBroker*, rvn_enchant_describe_fn, void*);
	int (*dict_check)(EnchantDict*, const char*, ssize_t);
	char** (*dict_suggest)(EnchantDict*, const char*, ssize_t, size_t*);
	void (*dict_free_string_list)(EnchantDict*, char**);
	void (*dict_add)(EnchantDict*, const char*, ssize_t);
	/* Hunspell */
	Hunhandle* (*hun_create)(const char*, const char*);
	void (*hun_destroy)(Hunhandle*);
	int (*hun_spell)(Hunhandle*, const char*);
	int (*hun_suggest)(Hunhandle*, char***, const char*);
	void (*hun_free_list)(Hunhandle*, char***, int);
	int (*hun_add)(Hunhandle*, const char*);
	char* (*hun_get_dic_encoding)(Hunhandle*);

	/** The languages, and for Hunspell where their files are (the .dic, the .aff beside it). */
	char** languages;
	char** paths;
	int language_count;
	int language_cap;
	char* default_language;
	/** The dictionaries opened, one per language used. */
	struct {
		char* language;
		void* dict;
		/** Hunspell: the dictionary is in UTF-8 (otherwise only words in ASCII are given to it). */
		int utf8;
	} dicts[RVN_UNIX_SPELL_DICTS];
	int dict_count;
} g_unix_spell;

#define RVN_UNIX_SPELL_LOAD(field, name) \
	(*(void**)&g_unix_spell.field = dlsym(g_unix_spell.library, name)) != NULL

static void rvn_unix_spell_add_language(const char* tag, const char* path) {
	int i;
	if (!tag || !tag[0]) return;
	for (i = 0; i < g_unix_spell.language_count; i++) {
		if (strcmp(g_unix_spell.languages[i], tag) == 0) return;
	}
	if (g_unix_spell.language_count == g_unix_spell.language_cap) {
		g_unix_spell.language_cap = g_unix_spell.language_cap ? g_unix_spell.language_cap * 2 : 16;
		g_unix_spell.languages = (char**)rvn_realloc(g_unix_spell.languages, sizeof(char*) * (size_t)g_unix_spell.language_cap);
		g_unix_spell.paths = (char**)rvn_realloc(g_unix_spell.paths, sizeof(char*) * (size_t)g_unix_spell.language_cap);
	}
	g_unix_spell.languages[g_unix_spell.language_count] = rvn_strdup(tag);
	g_unix_spell.paths[g_unix_spell.language_count] = path ? rvn_strdup(path) : NULL;
	g_unix_spell.language_count++;
}

static void rvn_unix_spell_describe(const char* tag, const char* provider_name, const char* provider_desc,
	const char* provider_file, void* user_data) {
	(void)provider_name;
	(void)provider_desc;
	(void)provider_file;
	(void)user_data;
	rvn_unix_spell_add_language(tag, NULL);
}

static int rvn_unix_spell_open_enchant(void) {
	g_unix_spell.library = dlopen("libenchant-2.so.2", RTLD_NOW | RTLD_LOCAL);
	if (!g_unix_spell.library) g_unix_spell.library = dlopen("libenchant-2.so", RTLD_NOW | RTLD_LOCAL);
	if (!g_unix_spell.library) return R_FALSE;
	if (!(RVN_UNIX_SPELL_LOAD(broker_init, "enchant_broker_init") && RVN_UNIX_SPELL_LOAD(broker_free, "enchant_broker_free") &&
		RVN_UNIX_SPELL_LOAD(broker_request_dict, "enchant_broker_request_dict") &&
		RVN_UNIX_SPELL_LOAD(broker_free_dict, "enchant_broker_free_dict") &&
		RVN_UNIX_SPELL_LOAD(broker_list_dicts, "enchant_broker_list_dicts") &&
		RVN_UNIX_SPELL_LOAD(dict_check, "enchant_dict_check") && RVN_UNIX_SPELL_LOAD(dict_suggest, "enchant_dict_suggest") &&
		RVN_UNIX_SPELL_LOAD(dict_free_string_list, "enchant_dict_free_string_list") &&
		RVN_UNIX_SPELL_LOAD(dict_add, "enchant_dict_add"))) {
		dlclose(g_unix_spell.library);
		g_unix_spell.library = NULL;
		return R_FALSE;
	}
	g_unix_spell.broker = g_unix_spell.broker_init();
	if (!g_unix_spell.broker) {
		dlclose(g_unix_spell.library);
		g_unix_spell.library = NULL;
		return R_FALSE;
	}
	g_unix_spell.broker_list_dicts(g_unix_spell.broker, rvn_unix_spell_describe, NULL);
	return R_TRUE;
}

/* The dictionaries of a folder: each name.dic with a name.aff beside it. */
static void rvn_unix_spell_scan(const char* folder) {
	DIR* dir = opendir(folder);
	struct dirent* entry;
	if (!dir) return;
	while ((entry = readdir(dir)) != NULL) {
		size_t length = strlen(entry->d_name);
		char path[1024];
		FILE* aff;
		if (length < 5 || strcmp(entry->d_name + length - 4, ".dic") != 0 || entry->d_name[0] == '.') continue;
		/* (hyph_*.dic are hyphenation patterns, th_*.dat thesauri) */
		if (strncmp(entry->d_name, "hyph_", 5) == 0) continue;
		if ((size_t)snprintf(path, sizeof(path), "%s/%.*s.aff", folder, (int)(length - 4), entry->d_name) >= sizeof(path)) continue;
		aff = fopen(path, "rb");
		if (!aff) continue;
		fclose(aff);
		path[strlen(path) - 4] = '\0';
		{
			char tag[128];
			snprintf(tag, sizeof(tag), "%.*s", (int)(length - 4), entry->d_name);
			rvn_unix_spell_add_language(tag, path);
		}
	}
	closedir(dir);
}

static int rvn_unix_spell_open_hunspell(void) {
	static const char* const libraries[] = {
		"libhunspell-1.7.so.0", "libhunspell-1.6.so.0", "libhunspell.so.0", "libhunspell.so"
	};
	static const char* const folders[] = {
		"/usr/share/hunspell", "/usr/share/myspell/dicts", "/usr/share/myspell",
		"/usr/local/share/hunspell", "/usr/local/share/myspell"
	};
	const char* dicpath = getenv("DICPATH");
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(libraries) && !g_unix_spell.library; i++) {
		g_unix_spell.library = dlopen(libraries[i], RTLD_NOW | RTLD_LOCAL);
	}
	if (!g_unix_spell.library) return R_FALSE;
	if (!(RVN_UNIX_SPELL_LOAD(hun_create, "Hunspell_create") && RVN_UNIX_SPELL_LOAD(hun_destroy, "Hunspell_destroy") &&
		RVN_UNIX_SPELL_LOAD(hun_spell, "Hunspell_spell") && RVN_UNIX_SPELL_LOAD(hun_suggest, "Hunspell_suggest") &&
		RVN_UNIX_SPELL_LOAD(hun_free_list, "Hunspell_free_list") && RVN_UNIX_SPELL_LOAD(hun_add, "Hunspell_add") &&
		RVN_UNIX_SPELL_LOAD(hun_get_dic_encoding, "Hunspell_get_dic_encoding"))) {
		dlclose(g_unix_spell.library);
		g_unix_spell.library = NULL;
		return R_FALSE;
	}
	/* $DICPATH first (folders separated by ':'), then the folders of the distributions */
	if (dicpath && dicpath[0]) {
		char* copy = rvn_strdup(dicpath);
		char* folder = copy;
		while (folder) {
			char* next = strchr(folder, ':');
			if (next) *next++ = '\0';
			if (folder[0]) rvn_unix_spell_scan(folder);
			folder = next;
		}
		rvn_free(copy);
	}
	for (i = 0; i < RVN_COUNT_OF(folders); i++) rvn_unix_spell_scan(folders[i]);
	return R_TRUE;
}

static void rvn_unix_spell_close(void) {
	int i;
	for (i = 0; i < g_unix_spell.dict_count; i++) {
		if (g_unix_spell.broker) g_unix_spell.broker_free_dict(g_unix_spell.broker, (EnchantDict*)g_unix_spell.dicts[i].dict);
		else g_unix_spell.hun_destroy((Hunhandle*)g_unix_spell.dicts[i].dict);
		rvn_free(g_unix_spell.dicts[i].language);
	}
	if (g_unix_spell.broker) g_unix_spell.broker_free(g_unix_spell.broker);
	for (i = 0; i < g_unix_spell.language_count; i++) {
		rvn_free(g_unix_spell.languages[i]);
		rvn_free(g_unix_spell.paths[i]);
	}
	rvn_free(g_unix_spell.languages);
	rvn_free(g_unix_spell.paths);
	rvn_free(g_unix_spell.default_language);
	if (g_unix_spell.library) dlclose(g_unix_spell.library);
	memset(&g_unix_spell, 0, sizeof(g_unix_spell));
}

static int rvn_unix_spell_open(void) {
	const char* lang;
	int i;
	if (!rvn_unix_spell_open_enchant() && !rvn_unix_spell_open_hunspell()) return 0;
	if (g_unix_spell.language_count == 0) {
		/* the library is there, but no dictionary */
		rvn_unix_spell_close();
		return 0;
	}
	/* the language of the environment ("fr_FR.UTF-8"), else English, else the first one */
	lang = getenv("LC_ALL");
	if (!lang || !lang[0]) lang = getenv("LC_MESSAGES");
	if (!lang || !lang[0]) lang = getenv("LANG");
	if (lang && lang[0] && strcmp(lang, "C") != 0 && strcmp(lang, "POSIX") != 0) {
		size_t length = strcspn(lang, ".@");
		for (i = 0; i < g_unix_spell.language_count && !g_unix_spell.default_language; i++) {
			if (strlen(g_unix_spell.languages[i]) == length && strncmp(g_unix_spell.languages[i], lang, length) == 0) {
				g_unix_spell.default_language = rvn_strdup(g_unix_spell.languages[i]);
			}
		}
	}
	for (i = 0; i < g_unix_spell.language_count && !g_unix_spell.default_language; i++) {
		if (strcmp(g_unix_spell.languages[i], "en_US") == 0) g_unix_spell.default_language = rvn_strdup("en_US");
	}
	if (!g_unix_spell.default_language) g_unix_spell.default_language = rvn_strdup(g_unix_spell.languages[0]);
	return 1;
}

static int rvn_unix_spell_language_count(void) {
	return g_unix_spell.language_count;
}

static const char* rvn_unix_spell_language_at(int index) {
	return index >= 0 && index < g_unix_spell.language_count ? g_unix_spell.languages[index] : NULL;
}

static const char* rvn_unix_spell_default_language(void) {
	return g_unix_spell.default_language;
}

/* The dictionary of a language, opened on first use; its index in dicts, or -1. */
static int rvn_unix_spell_dict(const char* language) {
	void* dict = NULL;
	int i, utf8 = R_TRUE;
	if (!language) language = g_unix_spell.default_language;
	if (!language) return -1;
	for (i = 0; i < g_unix_spell.dict_count; i++) {
		if (strcmp(g_unix_spell.dicts[i].language, language) == 0) return i;
	}
	if (g_unix_spell.broker) {
		dict = g_unix_spell.broker_request_dict(g_unix_spell.broker, language);
	} else {
		for (i = 0; i < g_unix_spell.language_count; i++) {
			char aff[1040], dic[1040];
			const char* encoding;
			if (strcmp(g_unix_spell.languages[i], language) != 0 || !g_unix_spell.paths[i]) continue;
			snprintf(aff, sizeof(aff), "%s.aff", g_unix_spell.paths[i]);
			snprintf(dic, sizeof(dic), "%s.dic", g_unix_spell.paths[i]);
			dict = g_unix_spell.hun_create(aff, dic);
			if (!dict) break;
			encoding = g_unix_spell.hun_get_dic_encoding((Hunhandle*)dict);
			utf8 = encoding && (rvn_stricmp(encoding, "UTF-8") == 0 || rvn_stricmp(encoding, "UTF8") == 0);
			break;
		}
	}
	if (!dict) return -1;
	if (g_unix_spell.dict_count == RVN_UNIX_SPELL_DICTS) {
		/* the oldest one goes */
		if (g_unix_spell.broker) g_unix_spell.broker_free_dict(g_unix_spell.broker, (EnchantDict*)g_unix_spell.dicts[0].dict);
		else g_unix_spell.hun_destroy((Hunhandle*)g_unix_spell.dicts[0].dict);
		rvn_free(g_unix_spell.dicts[0].language);
		memmove(&g_unix_spell.dicts[0], &g_unix_spell.dicts[1], sizeof(g_unix_spell.dicts[0]) * (RVN_UNIX_SPELL_DICTS - 1));
		g_unix_spell.dict_count--;
	}
	i = g_unix_spell.dict_count++;
	g_unix_spell.dicts[i].language = rvn_strdup(language);
	g_unix_spell.dicts[i].dict = dict;
	g_unix_spell.dicts[i].utf8 = utf8;
	return i;
}

static int rvn_unix_spell_ascii(const char* text, int length) {
	int i;
	for (i = 0; i < length; i++) {
		if ((unsigned char)text[i] >= 0x80) return R_FALSE;
	}
	return R_TRUE;
}

typedef struct rvn_unix_spell_job_t {
	int dict;
	const char* text;
	rvn_spell_range_fn add;
	void* user;
} rvn_unix_spell_job_t;

static void rvn_unix_spell_word(void* user, int start, int end) {
	rvn_unix_spell_job_t* job = (rvn_unix_spell_job_t*)user;
	void* dict = g_unix_spell.dicts[job->dict].dict;
	int wrong;
	if (g_unix_spell.broker) {
		wrong = g_unix_spell.dict_check((EnchantDict*)dict, job->text + start, (ssize_t)(end - start)) > 0;
	} else {
		char word[256];
		if (end - start >= (int)sizeof(word)) return;
		if (!g_unix_spell.dicts[job->dict].utf8 && !rvn_unix_spell_ascii(job->text + start, end - start)) return;
		memcpy(word, job->text + start, (size_t)(end - start));
		word[end - start] = '\0';
		wrong = g_unix_spell.hun_spell((Hunhandle*)dict, word) == 0;
	}
	if (wrong) job->add(job->user, start, end);
}

static int rvn_unix_spell_check(const char* language, const char* text, int length, rvn_spell_range_fn add, void* user) {
	rvn_unix_spell_job_t job;
	job.dict = rvn_unix_spell_dict(language);
	if (job.dict < 0) return 0;
	job.text = text;
	job.add = add;
	job.user = user;
	rvn_spell_words(text, length, rvn_unix_spell_word, &job);
	return 1;
}

static int rvn_unix_spell_suggest(const char* language, const char* word, rvn_spell_word_fn add, void* user) {
	int index = rvn_unix_spell_dict(language);
	void* dict;
	if (index < 0) return 0;
	dict = g_unix_spell.dicts[index].dict;
	if (g_unix_spell.broker) {
		size_t count = 0, i;
		char** list = g_unix_spell.dict_suggest((EnchantDict*)dict, word, (ssize_t)strlen(word), &count);
		if (!list) return 1;
		for (i = 0; i < count; i++) add(user, list[i]);
		g_unix_spell.dict_free_string_list((EnchantDict*)dict, list);
	} else {
		char** list = NULL;
		int count, i;
		int utf8 = g_unix_spell.dicts[index].utf8;
		if (!utf8 && !rvn_unix_spell_ascii(word, (int)strlen(word))) return 1;
		count = g_unix_spell.hun_suggest((Hunhandle*)dict, &list, word);
		for (i = 0; i < count; i++) {
			if (list[i] && (utf8 || rvn_unix_spell_ascii(list[i], (int)strlen(list[i])))) add(user, list[i]);
		}
		if (list) g_unix_spell.hun_free_list((Hunhandle*)dict, &list, count);
	}
	return 1;
}

static void rvn_unix_spell_learn(const char* language, const char* word) {
	int index = rvn_unix_spell_dict(language);
	if (index < 0) return;
	if (g_unix_spell.broker) g_unix_spell.dict_add((EnchantDict*)g_unix_spell.dicts[index].dict, word, (ssize_t)strlen(word));
	else g_unix_spell.hun_add((Hunhandle*)g_unix_spell.dicts[index].dict, word);
}

static const rvn_spell_engine_t g_unix_spell_engine = {
	"Enchant/Hunspell",
	rvn_unix_spell_open,
	rvn_unix_spell_close,
	rvn_unix_spell_language_count,
	rvn_unix_spell_language_at,
	rvn_unix_spell_default_language,
	rvn_unix_spell_check,
	rvn_unix_spell_suggest,
	rvn_unix_spell_learn
};

const rvn_spell_engine_t* rvn_spell_engine_system(void) {
	return &g_unix_spell_engine;
}
