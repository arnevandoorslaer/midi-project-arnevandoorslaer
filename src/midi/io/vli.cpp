#include "io/vli.h"

namespace io {

	bool leftmost_bit_set(uint8_t byte) {
		return (byte >> 7) == 1;
	}

	uint8_t lowest_7_bit(uint8_t byte) {
		return byte & 0b0111'1111;
	}

	uint64_t read_variable_length_integer(std::istream & in) {
		uint8_t byte = read<uint8_t>(in);
		uint64_t acc = 0;
		while (leftmost_bit_set(byte)) {
			acc = (acc << 7) | lowest_7_bit(byte);
			byte = read<uint8_t>(in);
		}

		acc = (acc << 7) | lowest_7_bit(byte);
		return acc;

	}
}
