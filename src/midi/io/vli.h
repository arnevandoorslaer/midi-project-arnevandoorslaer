#ifndef VLI_H
#define VLI_H

#include "read.h"
#include <istream>
#include <cstdint>
#include "read.h"

namespace io {
	uint64_t read_variable_length_integer(std::istream & in);
}
#endif