#pragma pack(push,1)
#ifndef MIDI_H
#define MIDI_H

#include <cstdint>
#include <istream>
#include <ostream>
#include "primitives.h"

namespace midi {
	struct CHUNK_HEADER {
		char id[4];
		uint32_t size;
	};

	void read_chunk_header(std::istream&, CHUNK_HEADER*);

	std::string header_id(CHUNK_HEADER);

	struct MTHD {
		CHUNK_HEADER header;
		uint16_t type;
		uint16_t ntracks;
		uint16_t division;
	};

	void read_mthd(std::istream&, MTHD*);

	bool is_sysex_event(uint8_t);
	bool is_meta_event(uint8_t);
	bool is_midi_event(uint8_t);
	bool is_running_status(uint8_t);

	uint8_t extract_midi_event_type(uint8_t);
	Channel extract_midi_event_channel(uint8_t);

	bool is_note_off(uint8_t);
	bool is_note_on(uint8_t);
	bool is_polyphonic_key_pressure(uint8_t);
	bool is_control_change(uint8_t);
	bool is_program_change(uint8_t);
	bool is_channel_pressure(uint8_t);
	bool is_pitch_wheel_change(uint8_t);

	struct EventReceiver {
		virtual void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) = 0;
		virtual void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) = 0;
		virtual void program_change(Duration dt, Channel channel, Instrument program) = 0;
		virtual void channel_pressure(Duration dt, Channel channel, uint8_t pressure) = 0;
		virtual void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) = 0;
		virtual void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
		virtual void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
	};

	void read_mtrk(std::istream&, EventReceiver&);

	struct NOTE {
		NoteNumber note_number;
		Time start;
		Duration duration;
		uint8_t velocity;
		Instrument instrument;

		NOTE(NoteNumber noteNumber, Time start, Duration duration, uint8_t velocity, Instrument instrument) :
			note_number(noteNumber), start(start), duration(duration), velocity(velocity), instrument(instrument) {

		}

		bool operator ==(const NOTE other) const{
			return this->note_number == other.note_number &&
				this->start == other.start &&
				this->duration == other.duration &&
				this->velocity == other.velocity &&
				this->instrument == other.instrument;
		}
		bool operator !=(const NOTE other) const{
			return this->note_number != other.note_number &&
				this->start != other.start &&
				this->duration != other.duration &&
				this->velocity != other.velocity &&
				this->instrument != other.instrument;
		}

	};


	inline std::ostream& operator <<(std::ostream& out, const NOTE& note) {
		return out << "Note(number=" <<
			note.note_number << ",start=" <<
			note.start << ",duration=" <<
			note.duration << ",instrument=" <<
			note.instrument << ")";
	}
}

#endif
#pragma pack(pop)