// Raven SDK Installer - a small JSON value: the index of the payload, the
// installation manifest, the description of a project template.
//
// Objects keep the order of their keys (manifests stay readable and diffable).
// Numbers are doubles: every integer the installer stores (sizes, offsets,
// CRCs) is far below 2^53.
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace rinst {

class Json {
public:
	enum class Type { Null, Bool, Number, String, Array, Object };

	Json() = default;
	Json(std::nullptr_t) {}
	Json(bool value) : type_(Type::Bool), bool_(value) {}
	Json(int value) : type_(Type::Number), number_(value) {}
	Json(unsigned value) : type_(Type::Number), number_(value) {}
	Json(long long value) : type_(Type::Number), number_(static_cast<double>(value)) {}
	Json(unsigned long long value) : type_(Type::Number), number_(static_cast<double>(value)) {}
	Json(long value) : type_(Type::Number), number_(static_cast<double>(value)) {}
	Json(unsigned long value) : type_(Type::Number), number_(static_cast<double>(value)) {}
	Json(double value) : type_(Type::Number), number_(value) {}
	Json(const char* value) : type_(Type::String), string_(value ? value : "") {}
	Json(std::string value) : type_(Type::String), string_(std::move(value)) {}

	static Json array();
	static Json object();

	Type type() const { return type_; }
	bool is_null() const { return type_ == Type::Null; }
	bool is_bool() const { return type_ == Type::Bool; }
	bool is_number() const { return type_ == Type::Number; }
	bool is_string() const { return type_ == Type::String; }
	bool is_array() const { return type_ == Type::Array; }
	bool is_object() const { return type_ == Type::Object; }

	bool as_bool(bool fallback = false) const { return type_ == Type::Bool ? bool_ : fallback; }
	double as_number(double fallback = 0) const { return type_ == Type::Number ? number_ : fallback; }
	std::int64_t as_int(std::int64_t fallback = 0) const;
	std::uint64_t as_uint(std::uint64_t fallback = 0) const;
	/** The text, or "" when this is not a string. */
	const std::string& as_string() const;
	std::string string_or(const std::string& fallback) const { return type_ == Type::String ? string_ : fallback; }

	/** Arrays: number of elements; objects: number of keys. */
	std::size_t size() const;
	const Json& operator[](std::size_t index) const;
	Json& push(Json value);
	const std::vector<Json>& elements() const { return array_; }

	/** The value of a key, or a null value when it is missing (or this is not an object). */
	const Json& operator[](const std::string& key) const;
	const Json& operator[](const char* key) const { return (*this)[std::string(key)]; }
	bool has(const std::string& key) const;
	/** Sets a key (replaces it when present, appends it otherwise); returns the value stored. */
	Json& set(const std::string& key, Json value);
	bool erase(const std::string& key);
	const std::vector<std::pair<std::string, Json>>& items() const { return object_; }

	/** indent < 0: on one line. */
	std::string dump(int indent = -1) const;
	static bool parse(const std::string& text, Json& out, std::string* error = nullptr);

private:
	void dump_to(std::string& out, int indent, int depth) const;

	Type type_ = Type::Null;
	bool bool_ = false;
	double number_ = 0;
	std::string string_;
	std::vector<Json> array_;
	std::vector<std::pair<std::string, Json>> object_;
};

} // namespace rinst
