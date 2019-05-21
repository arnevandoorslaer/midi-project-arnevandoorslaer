#include "midi.h"
#include "../io/read.h"
#include "../io/endianness.h"
#include "../io/vli.h"

namespace midi {
	void read_chunk_header(std::istream& in, CHUNK_HEADER* header) {
		io::read_to(in, header);
		io::switch_endianness(&header->size);
	}

	void read_mthd(std::istream& in, MTHD* mthd) {
		io::read_to(in, mthd);
		io::switch_endianness(&mthd->header.size);
		io::switch_endianness(&mthd->type);
		io::switch_endianness(&mthd->ntracks);
		io::switch_endianness(&mthd->division);
	}

	std::string header_id(CHUNK_HEADER header) {
		std::string res = "";
		for (char c : header.id) {
			res += c;
		}
		return res;
	}

	bool is_sysex_event(uint8_t byte) {
		return (byte == 0xF0) | (byte == 0xF7);
	};
	bool is_meta_event(uint8_t byte) {
		return byte == 0xFF;
	};
	bool is_midi_event(uint8_t byte) {
		return (byte >= 0x80) & (byte < 0xF0);
	};
	bool is_running_status(uint8_t byte) {
		return byte < 0x80;
	};

	uint8_t extract_midi_event_type(uint8_t status) {
		return status >> 4;
	};
	Channel extract_midi_event_channel(uint8_t status) {
		return Channel(status & 0x0F);
	};

	bool is_note_off(uint8_t status) {
		return status == 0x08;
	};
	bool is_note_on(uint8_t status) {
		return status == 0x09;
	};
	bool is_polyphonic_key_pressure(uint8_t status) {
		return status == 0x0A;
	};
	bool is_control_change(uint8_t status) {
		return status == 0x0B;
	};
	bool is_program_change(uint8_t status) {
		return status == 0x0C;
	};
	bool is_channel_pressure(uint8_t status) {
		return status == 0x0D;
	};
	bool is_pitch_wheel_change(uint8_t status) {
		return status == 0x0E;
	};

	void read_mtrk(std::istream& in, EventReceiver& receiver) {
		CHUNK_HEADER header;
		read_chunk_header(in, &header);
		bool has_next = true;

		uint8_t previousID;
		uint8_t firstBytes;

		while (has_next) {
			Duration duration(io::read_variable_length_integer(in));
			uint8_t identifier = io::read<uint8_t>(in);

			if (is_running_status(identifier)) {
				firstBytes = identifier;
				identifier = previousID;
			}
			else {
				firstBytes = io::read<uint8_t>(in);
			}
			if (is_meta_event(identifier)) {
				uint8_t type = firstBytes;
				uint64_t length(io::read_variable_length_integer(in));
				std::unique_ptr<uint8_t[]> data = io::read_array<uint8_t>(in, length);
				receiver.meta(duration, type, std::move(data), length);
				if (type == 0x2F) {
					// end of track
					has_next = false;
				}
			}
			else if (is_sysex_event(identifier)) {
				in.putback(firstBytes);
				uint64_t length(io::read_variable_length_integer(in));
				std::unique_ptr<uint8_t[]> data = io::read_array<uint8_t>(in, length);
				receiver.sysex(duration, std::move(data), length);
			}
			else if (is_midi_event(identifier)) {
				uint8_t midiEventType = extract_midi_event_type(identifier);
				Channel channel = extract_midi_event_channel(identifier);
				if (is_note_off(midiEventType)) {
					uint8_t note = firstBytes;
					uint8_t velocity = io::read<uint8_t>(in);
					receiver.note_off(duration, channel, NoteNumber(note), velocity);
				}
				else if (is_note_on(midiEventType)) {
					uint8_t note = firstBytes;
					uint8_t velocity = io::read<uint8_t>(in);
					receiver.note_on(duration, channel, NoteNumber(note), velocity);
				}
				else if (is_polyphonic_key_pressure(midiEventType)) {
					uint8_t note = firstBytes;
					uint8_t pressure = io::read<uint8_t>(in);
					receiver.polyphonic_key_pressure(duration, channel, NoteNumber(note), pressure);
				}
				else if (is_control_change(midiEventType)) {
					uint8_t controller = firstBytes;
					uint8_t value = io::read<uint8_t>(in);
					receiver.control_change(duration, channel, controller, value);
				}
				else if (is_program_change(midiEventType)) {
					uint8_t program = firstBytes;
					receiver.program_change(duration, channel, Instrument(program));
				}
				else if (is_channel_pressure(midiEventType)) {
					uint8_t pressure = firstBytes;
					receiver.channel_pressure(duration, channel, pressure);
				}
				else if (is_pitch_wheel_change(midiEventType)) {
					uint8_t lower = firstBytes;
					uint8_t upper = io::read<uint8_t>(in);
					uint16_t lower_upper = lower | (upper << 7);
					receiver.pitch_wheel_change(duration, channel, lower_upper);
				}
				previousID = identifier;
			}

		}

	}

}