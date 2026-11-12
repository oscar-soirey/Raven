// Raven SDK Installer - the files embedded in the program (the XML of the
// interface, the logo). resources.cpp is written by cmake/EmbedResources.cmake
// when one of them changes.
#pragma once

#include <cstddef>

namespace rinst::resources {

struct Resource {
	const char* path; // "ui/main.xml"
	const unsigned char* data;
	std::size_t size;
};

extern const Resource kResources[];
extern const std::size_t kResourceCount;

} // namespace rinst::resources
