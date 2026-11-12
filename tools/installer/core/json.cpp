// Raven SDK Installer - JSON value (see json.hpp).
#include "json.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace rinst {

namespace {
const Json& null_value() {
	static const Json value;
	return value;
}
const std::string& empty_string() {
	static const std::string value;
	return value;
}

void append_utf8(std::string& out, std::uint32_t cp) {
	if (cp < 0x80) out += static_cast<char>(cp);
	else if (cp < 0x800) {
		out += static_cast<char>(0xC0 | (cp >> 6));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		out += static_cast<char>(0xE0 | (cp >> 12));
		out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	} else {
		out += static_cast<char>(0xF0 | (cp >> 18));
		out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
}

void dump_string(std::string& out, const std::string& text) {
	out += '"';
	for (unsigned char c : text) {
		switch (c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			default:
				if (c < 0x20) {
					char buffer[8];
					std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
					out += buffer;
				} else {
					out += static_cast<char>(c);
				}
		}
	}
	out += '"';
}

class Parser {
public:
	explicit Parser(const std::string& text) : s_(text) {}

	bool parse(Json& out, std::string* error) {
		skip();
		if (!value(out, 0)) {
			if (error) *error = error_ + " (at offset " + std::to_string(pos_) + ")";
			return false;
		}
		skip();
		if (pos_ != s_.size()) {
			if (error) *error = "unexpected text after the value (at offset " + std::to_string(pos_) + ")";
			return false;
		}
		return true;
	}

private:
	void skip() {
		while (pos_ < s_.size() && (s_[pos_] == ' ' || s_[pos_] == '\t' || s_[pos_] == '\n' || s_[pos_] == '\r')) pos_++;
	}
	bool fail(const char* why) {
		error_ = why;
		return false;
	}
	bool literal(const char* word) {
		std::size_t i = 0;
		for (; word[i]; i++)
			if (pos_ + i >= s_.size() || s_[pos_ + i] != word[i]) return false;
		pos_ += i;
		return true;
	}
	bool hex4(std::uint32_t& value) {
		if (pos_ + 4 > s_.size()) return fail("truncated \\u escape");
		value = 0;
		for (int i = 0; i < 4; i++) {
			const char c = s_[pos_++];
			value <<= 4;
			if (c >= '0' && c <= '9') value |= static_cast<std::uint32_t>(c - '0');
			else if (c >= 'a' && c <= 'f') value |= static_cast<std::uint32_t>(c - 'a' + 10);
			else if (c >= 'A' && c <= 'F') value |= static_cast<std::uint32_t>(c - 'A' + 10);
			else return fail("bad \\u escape");
		}
		return true;
	}
	bool string(std::string& out) {
		pos_++; // "
		while (pos_ < s_.size()) {
			const char c = s_[pos_++];
			if (c == '"') return true;
			if (c != '\\') {
				out += c;
				continue;
			}
			if (pos_ >= s_.size()) break;
			const char e = s_[pos_++];
			switch (e) {
				case '"': out += '"'; break;
				case '\\': out += '\\'; break;
				case '/': out += '/'; break;
				case 'b': out += '\b'; break;
				case 'f': out += '\f'; break;
				case 'n': out += '\n'; break;
				case 'r': out += '\r'; break;
				case 't': out += '\t'; break;
				case 'u': {
					std::uint32_t cp = 0;
					if (!hex4(cp)) return false;
					if (cp >= 0xD800 && cp < 0xDC00 && pos_ + 6 <= s_.size() && s_[pos_] == '\\' && s_[pos_ + 1] == 'u') {
						pos_ += 2;
						std::uint32_t low = 0;
						if (!hex4(low)) return false;
						if (low >= 0xDC00 && low < 0xE000) cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
					}
					append_utf8(out, cp);
					break;
				}
				default: return fail("bad escape");
			}
		}
		return fail("unterminated string");
	}
	bool value(Json& out, int depth) {
		if (depth > 64) return fail("nested too deeply");
		if (pos_ >= s_.size()) return fail("unexpected end");
		const char c = s_[pos_];
		if (c == '{') {
			pos_++;
			out = Json::object();
			skip();
			if (pos_ < s_.size() && s_[pos_] == '}') {
				pos_++;
				return true;
			}
			for (;;) {
				skip();
				if (pos_ >= s_.size() || s_[pos_] != '"') return fail("expected a key");
				std::string key;
				if (!string(key)) return false;
				skip();
				if (pos_ >= s_.size() || s_[pos_] != ':') return fail("expected ':'");
				pos_++;
				skip();
				Json item;
				if (!value(item, depth + 1)) return false;
				out.set(key, std::move(item));
				skip();
				if (pos_ < s_.size() && s_[pos_] == ',') {
					pos_++;
					continue;
				}
				if (pos_ < s_.size() && s_[pos_] == '}') {
					pos_++;
					return true;
				}
				return fail("expected ',' or '}'");
			}
		}
		if (c == '[') {
			pos_++;
			out = Json::array();
			skip();
			if (pos_ < s_.size() && s_[pos_] == ']') {
				pos_++;
				return true;
			}
			for (;;) {
				skip();
				Json item;
				if (!value(item, depth + 1)) return false;
				out.push(std::move(item));
				skip();
				if (pos_ < s_.size() && s_[pos_] == ',') {
					pos_++;
					continue;
				}
				if (pos_ < s_.size() && s_[pos_] == ']') {
					pos_++;
					return true;
				}
				return fail("expected ',' or ']'");
			}
		}
		if (c == '"') {
			std::string text;
			if (!string(text)) return false;
			out = Json(std::move(text));
			return true;
		}
		if (literal("true")) {
			out = Json(true);
			return true;
		}
		if (literal("false")) {
			out = Json(false);
			return true;
		}
		if (literal("null")) {
			out = Json();
			return true;
		}
		if (c == '-' || (c >= '0' && c <= '9')) {
			const char* begin = s_.c_str() + pos_;
			char* end = nullptr;
			const double number = std::strtod(begin, &end);
			if (end == begin) return fail("bad number");
			pos_ += static_cast<std::size_t>(end - begin);
			out = Json(number);
			return true;
		}
		return fail("unexpected character");
	}

	const std::string& s_;
	std::size_t pos_ = 0;
	std::string error_;
};

} // namespace

Json Json::array() {
	Json value;
	value.type_ = Type::Array;
	return value;
}

Json Json::object() {
	Json value;
	value.type_ = Type::Object;
	return value;
}

std::int64_t Json::as_int(std::int64_t fallback) const {
	return type_ == Type::Number ? static_cast<std::int64_t>(std::llround(number_)) : fallback;
}

std::uint64_t Json::as_uint(std::uint64_t fallback) const {
	return type_ == Type::Number && number_ >= 0 ? static_cast<std::uint64_t>(std::llround(number_)) : fallback;
}

const std::string& Json::as_string() const {
	return type_ == Type::String ? string_ : empty_string();
}

std::size_t Json::size() const {
	if (type_ == Type::Array) return array_.size();
	if (type_ == Type::Object) return object_.size();
	return 0;
}

const Json& Json::operator[](std::size_t index) const {
	return type_ == Type::Array && index < array_.size() ? array_[index] : null_value();
}

Json& Json::push(Json value) {
	if (type_ != Type::Array) *this = array();
	array_.push_back(std::move(value));
	return array_.back();
}

const Json& Json::operator[](const std::string& key) const {
	if (type_ == Type::Object)
		for (const auto& item : object_)
			if (item.first == key) return item.second;
	return null_value();
}

bool Json::has(const std::string& key) const {
	if (type_ == Type::Object)
		for (const auto& item : object_)
			if (item.first == key) return true;
	return false;
}

Json& Json::set(const std::string& key, Json value) {
	if (type_ != Type::Object) *this = object();
	for (auto& item : object_)
		if (item.first == key) {
			item.second = std::move(value);
			return item.second;
		}
	object_.emplace_back(key, std::move(value));
	return object_.back().second;
}

bool Json::erase(const std::string& key) {
	for (auto it = object_.begin(); it != object_.end(); ++it)
		if (it->first == key) {
			object_.erase(it);
			return true;
		}
	return false;
}

void Json::dump_to(std::string& out, int indent, int depth) const {
	auto newline = [&](int level) {
		if (indent < 0) return;
		out += '\n';
		out.append(static_cast<std::size_t>(indent * level), ' ');
	};
	switch (type_) {
		case Type::Null: out += "null"; break;
		case Type::Bool: out += bool_ ? "true" : "false"; break;
		case Type::Number: {
			char buffer[40];
			if (std::isfinite(number_) && number_ == std::floor(number_) && std::fabs(number_) < 9.007199254740992e15)
				std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(number_));
			else if (std::isfinite(number_))
				std::snprintf(buffer, sizeof(buffer), "%.17g", number_);
			else
				std::snprintf(buffer, sizeof(buffer), "null");
			out += buffer;
			break;
		}
		case Type::String: dump_string(out, string_); break;
		case Type::Array:
			if (array_.empty()) {
				out += "[]";
				break;
			}
			out += '[';
			for (std::size_t i = 0; i < array_.size(); i++) {
				if (i) out += ',';
				newline(depth + 1);
				array_[i].dump_to(out, indent, depth + 1);
			}
			newline(depth);
			out += ']';
			break;
		case Type::Object:
			if (object_.empty()) {
				out += "{}";
				break;
			}
			out += '{';
			for (std::size_t i = 0; i < object_.size(); i++) {
				if (i) out += ',';
				newline(depth + 1);
				dump_string(out, object_[i].first);
				out += indent < 0 ? ":" : ": ";
				object_[i].second.dump_to(out, indent, depth + 1);
			}
			newline(depth);
			out += '}';
			break;
	}
}

std::string Json::dump(int indent) const {
	std::string out;
	dump_to(out, indent, 0);
	return out;
}

bool Json::parse(const std::string& text, Json& out, std::string* error) {
	Parser parser(text);
	return parser.parse(out, error);
}

} // namespace rinst
