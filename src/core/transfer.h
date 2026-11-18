/**
 * Raven - core/transfer.h
 * Data given to the other applications (RDataItem): through the clipboard
 * and the drags out of the windows. What the platforms share to convert the
 * formats named by MIME types to their own.
 */

#ifndef RVN_TRANSFER_H
#define RVN_TRANSFER_H

#include "base.h"

/** R_TRUE when `mime` is `want`, ignoring case and the parameters ("text/plain;charset=utf-8" is "text/plain"). */
int rvn_mime_is(const char* mime, const char* want);
/** The first item of the format `mime`, or NULL. */
const RDataItem* rvn_data_find(const RDataItem* items, int count, const char* mime);
/**
 * A copy of `items` in one block (rvn_free): their MIME types and their
 * bytes, each followed by a '\0' that `size` does not count. Items without
 * a MIME type are left out: `*copied` receives how many were kept.
 */
RDataItem* rvn_data_copy(const RDataItem* items, int count, int* copied);
/** `size` bytes and a '\0' after them (rvn_free). */
char* rvn_bytes_dup(const void* data, size_t size);

/**
 * The local paths of the file:// URLs of a text/uri-list (lines starting
 * with '#' and the other schemes are skipped). Returns how many; `*paths`
 * is one block (rvn_free): the array, then the strings. Windows paths have
 * their drive and backslashes.
 */
int rvn_uri_list_paths(const char* list, size_t size, char*** paths);
/** Appends the file:// URL of a local path to a text/uri-list, with its CRLF. */
void rvn_uri_list_add(rvn_text_t* list, const char* path);

#endif /* RVN_TRANSFER_H */
