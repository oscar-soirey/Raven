// Raven SDK Installer - Windows helpers shared by the platform layer, the
// compiler detection and the shell integration: UTF-8 <-> UTF-16, the
// registry, error texts. Include only from Windows sources, after <windows.h>.
#pragma once
#if defined(_WIN32)

#include <string>
#include <vector>

namespace rinst::win32 {

std::wstring widen(const std::string& text);
std::string narrow(const std::wstring& text);
std::string last_error_text(unsigned long code = 0);

/** A string value (REG_SZ / REG_EXPAND_SZ, 64-bit view); "" when missing. */
std::string reg_get_string(HKEY root, const std::wstring& key, const std::wstring& name);
bool reg_key_exists(HKEY root, const std::wstring& key);
/** Creates the key (and its parents); name "" is the default value. */
bool reg_set_string(HKEY root, const std::wstring& key, const std::wstring& name, const std::wstring& value);
bool reg_set_dword(HKEY root, const std::wstring& key, const std::wstring& name, unsigned long value);
/** Deletes a key and everything under it. */
bool reg_delete_tree(HKEY root, const std::wstring& key);
std::vector<std::wstring> reg_subkeys(HKEY root, const std::wstring& key);

} // namespace rinst::win32

#endif
