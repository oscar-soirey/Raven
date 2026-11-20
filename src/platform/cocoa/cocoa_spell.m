/**
 * Raven - platform/cocoa/cocoa_spell.m
 * The spell checker of macOS (platform/spell.h): NSSpellChecker, the one of
 * TextEdit and of every NSTextView, with the languages and the dictionary
 * of the user chosen in the settings of the system. It needs no window and
 * no running NSApplication, so it works in headless runs too.
 *
 * NSString counts in UTF-16 code units, Raven in bytes of UTF-8: the ranges
 * found are converted on the way back.
 *
 * Manual reference counting, as the rest of the Cocoa backend.
 */

#import <AppKit/AppKit.h>

#include "../spell.h"

static struct {
	NSInteger tag;
	char** languages;
	int language_count;
	char* default_language;
} g_cocoa_spell;

static NSString* rvn_cocoa_spell_string(const char* text, int length) {
	return [[[NSString alloc] initWithBytes:text length:(NSUInteger)length encoding:NSUTF8StringEncoding] autorelease];
}

static NSString* rvn_cocoa_spell_language(const char* language) {
	return language ? [NSString stringWithUTF8String:language] : nil;
}

static int rvn_cocoa_spell_open(void) {
	@autoreleasepool {
		NSSpellChecker* checker = [NSSpellChecker sharedSpellChecker];
		NSArray<NSString*>* languages;
		NSUInteger i;
		if (!checker) return 0;
		languages = [checker availableLanguages];
		g_cocoa_spell.tag = [NSSpellChecker uniqueSpellDocumentTag];
		g_cocoa_spell.languages = (char**)rvn_alloc(sizeof(char*) * ([languages count] + 1));
		g_cocoa_spell.language_count = 0;
		for (i = 0; i < [languages count]; i++) {
			const char* name = [[languages objectAtIndex:i] UTF8String];
			if (name && name[0]) g_cocoa_spell.languages[g_cocoa_spell.language_count++] = rvn_strdup(name);
		}
		if ([checker language]) g_cocoa_spell.default_language = rvn_strdup([[checker language] UTF8String]);
		return 1;
	}
}

static void rvn_cocoa_spell_close(void) {
	int i;
	@autoreleasepool {
		[[NSSpellChecker sharedSpellChecker] closeSpellDocumentWithTag:g_cocoa_spell.tag];
	}
	for (i = 0; i < g_cocoa_spell.language_count; i++) rvn_free(g_cocoa_spell.languages[i]);
	rvn_free(g_cocoa_spell.languages);
	rvn_free(g_cocoa_spell.default_language);
	memset(&g_cocoa_spell, 0, sizeof(g_cocoa_spell));
}

static int rvn_cocoa_spell_language_count(void) {
	return g_cocoa_spell.language_count;
}

static const char* rvn_cocoa_spell_language_at(int index) {
	return index >= 0 && index < g_cocoa_spell.language_count ? g_cocoa_spell.languages[index] : NULL;
}

static const char* rvn_cocoa_spell_default_language(void) {
	return g_cocoa_spell.default_language;
}

/* Walks the UTF-8 text forward to a position counted in UTF-16 units; the positions asked for only grow. */
static int rvn_cocoa_spell_byte(const char* text, int length, NSUInteger target, NSUInteger* unit, int* byte) {
	while (*unit < target && *byte < length) {
		unsigned char c = (unsigned char)text[*byte];
		int size = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
		*unit += size == 4 ? 2 : 1;
		*byte = *byte + size > length ? length : *byte + size;
	}
	return *byte;
}

static int rvn_cocoa_spell_check(const char* language, const char* text, int length, rvn_spell_range_fn add, void* user) {
	@autoreleasepool {
		NSSpellChecker* checker = [NSSpellChecker sharedSpellChecker];
		NSString* string = rvn_cocoa_spell_string(text, length);
		NSString* lang = rvn_cocoa_spell_language(language);
		NSUInteger total, start = 0, unit = 0;
		int byte = 0;
		if (!string) return 0;
		total = [string length];
		while (start < total) {
			NSRange found = [checker checkSpellingOfString:string startingAt:(NSInteger)start language:lang wrap:NO
				inSpellDocumentWithTag:g_cocoa_spell.tag wordCount:NULL];
			int from, to;
			if (found.location == NSNotFound || found.length == 0 || found.location < start) break;
			from = rvn_cocoa_spell_byte(text, length, found.location, &unit, &byte);
			to = rvn_cocoa_spell_byte(text, length, found.location + found.length, &unit, &byte);
			if (to > from) add(user, from, to);
			start = found.location + found.length;
		}
		return 1;
	}
}

static int rvn_cocoa_spell_suggest(const char* language, const char* word, rvn_spell_word_fn add, void* user) {
	@autoreleasepool {
		NSString* string = [NSString stringWithUTF8String:word];
		NSArray<NSString*>* guesses;
		NSUInteger i;
		if (!string) return 0;
		guesses = [[NSSpellChecker sharedSpellChecker] guessesForWordRange:NSMakeRange(0, [string length]) inString:string
			language:rvn_cocoa_spell_language(language) inSpellDocumentWithTag:g_cocoa_spell.tag];
		for (i = 0; i < [guesses count]; i++) {
			const char* guess = [[guesses objectAtIndex:i] UTF8String];
			if (guess) add(user, guess);
		}
		return 1;
	}
}

static void rvn_cocoa_spell_learn(const char* language, const char* word) {
	(void)language;
	@autoreleasepool {
		NSString* string = [NSString stringWithUTF8String:word];
		/* (the dictionary of the user is shared by all the languages) */
		if (string) [[NSSpellChecker sharedSpellChecker] learnWord:string];
	}
}

static const rvn_spell_engine_t g_cocoa_spell_engine = {
	"NSSpellChecker",
	rvn_cocoa_spell_open,
	rvn_cocoa_spell_close,
	rvn_cocoa_spell_language_count,
	rvn_cocoa_spell_language_at,
	rvn_cocoa_spell_default_language,
	rvn_cocoa_spell_check,
	rvn_cocoa_spell_suggest,
	rvn_cocoa_spell_learn
};

const rvn_spell_engine_t* rvn_spell_engine_system(void) {
	return &g_cocoa_spell_engine;
}
