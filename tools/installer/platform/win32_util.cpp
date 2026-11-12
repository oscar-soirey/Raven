// Raven SDK Installer - Windows helpers (see win32_util.hpp).
#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "win32_util.hpp"

namespace rinst::win32 {

std::wstring widen(const std::string& text) {
	if (text.empty()) return {};
	const int n = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
	std::wstring out(static_cast<std::size_t>(n), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), &out[0], n);
	return out;
}

std::string narrow(const std::wstring& text) {
	if (text.empty()) return {};
	const int n = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
	std::string out(static_cast<std::size_t>(n), '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), &out[0], n, nullptr, nullptr);
	return out;
}

std::string last_error_text(unsigned long code) {
	if (code == 0) code = GetLastError();
	wchar_t* buffer = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0,
		reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
	std::string text = buffer ? narrow(buffer) : "error " + std::to_string(code);
	LocalFree(buffer);
	while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ' || text.back() == '.')) text.pop_back();
	return text;
}

std::string reg_get_string(HKEY root, const std::wstring& key, const std::wstring& name) {
	DWORD size = 0;
	if (RegGetValueW(root, key.c_str(), name.empty() ? nullptr : name.c_str(), RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_SUBKEY_WOW6464KEY,
			nullptr, nullptr, &size) != ERROR_SUCCESS || size == 0)
		return {};
	std::wstring value(size / sizeof(wchar_t), L'\0');
	if (RegGetValueW(root, key.c_str(), name.empty() ? nullptr : name.c_str(), RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_SUBKEY_WOW6464KEY,
			nullptr, &value[0], &size) != ERROR_SUCCESS)
		return {};
	value.resize(wcsnlen(value.c_str(), value.size()));
	return narrow(value);
}

bool reg_key_exists(HKEY root, const std::wstring& key) {
	HKEY h = nullptr;
	if (RegOpenKeyExW(root, key.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &h) != ERROR_SUCCESS) return false;
	RegCloseKey(h);
	return true;
}

bool reg_set_string(HKEY root, const std::wstring& key, const std::wstring& name, const std::wstring& value) {
	HKEY h = nullptr;
	if (RegCreateKeyExW(root, key.c_str(), 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &h, nullptr) != ERROR_SUCCESS)
		return false;
	const LONG r = RegSetValueExW(h, name.empty() ? nullptr : name.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
		static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
	RegCloseKey(h);
	return r == ERROR_SUCCESS;
}

bool reg_set_dword(HKEY root, const std::wstring& key, const std::wstring& name, unsigned long value) {
	HKEY h = nullptr;
	if (RegCreateKeyExW(root, key.c_str(), 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &h, nullptr) != ERROR_SUCCESS)
		return false;
	const DWORD v = value;
	const LONG r = RegSetValueExW(h, name.empty() ? nullptr : name.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
	RegCloseKey(h);
	return r == ERROR_SUCCESS;
}

bool reg_delete_tree(HKEY root, const std::wstring& key) {
	const LONG r = RegDeleteTreeW(root, key.c_str());
	if (r == ERROR_FILE_NOT_FOUND) return true;
	if (r != ERROR_SUCCESS) return false;
	return RegDeleteKeyExW(root, key.c_str(), KEY_WOW64_64KEY, 0) == ERROR_SUCCESS || !reg_key_exists(root, key);
}

std::vector<std::wstring> reg_subkeys(HKEY root, const std::wstring& key) {
	std::vector<std::wstring> names;
	HKEY h = nullptr;
	if (RegOpenKeyExW(root, key.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &h) != ERROR_SUCCESS) return names;
	wchar_t name[256];
	for (DWORD i = 0;; i++) {
		DWORD length = 256;
		if (RegEnumKeyExW(h, i, name, &length, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
		names.emplace_back(name, length);
	}
	RegCloseKey(h);
	return names;
}

} // namespace rinst::win32

#endif
