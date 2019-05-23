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

	bool NOTE::operator ==(const NOTE other) const {
		return this->note_number == other.note_number &&
			this->start == other.start &&
			this->duration == other.duration &&
			this->velocity == other.velocity &&
			this->instrument == other.instrument;
	}
	bool NOTE::operator !=(const NOTE other) const {
		return this->note_number != other.note_number &&
			this->start != other.start &&
			this->duration != other.duration &&
			this->velocity != other.velocity &&
			this->instrument != other.instrument;
	}

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



	void ChannelNoteCollector::note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
	{
		if (velocity == 0) {
			this->note_off(dt, channel, note, velocity);
		}
		else {
			this->current += dt;
			if (this->channel == channel) {
				if (this->velocities[value(note)] != 128) {
					this->note_off(Duration(0), channel, note, velocity);
				}	
				this->start[value(note)] = this->current;
				this->velocities[value(note)] = velocity;
			}
		}


	}

	void ChannelNoteCollector::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
	{
		this->current += dt;
		if (this->channel == channel) {
			NOTE n = NOTE(note, this->start[value(note)], 
				Duration(this->current - this->start[value(note)]), 
				this->velocities[value(note)], 
				this->instrument);
			this->note_receiver(n);
			this->velocities[value(note)] = -1;
		}
	}

	void ChannelNoteCollector::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure)
	{
		this->current += dt;
	}

	void ChannelNoteCollector::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value)
	{
		this->current += dt;
	}

	void ChannelNoteCollector::program_change(Duration dt, Channel channel, Instrument program)
	{
		this->current += dt;

		if (this->channel == channel) {
			this->instrument = program;
		}
	}

	void ChannelNoteCollector::channel_pressure(Duration dt, Channel channel, uint8_t pressure)
	{
		this->current += dt;

	}

	void ChannelNoteCollector::pitch_wheel_change(Duration dt, Channel channel, uint16_t value)
	{
		this->current += dt;

	}

	void ChannelNoteCollector::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
	{
		this->current += dt;
	}

	void ChannelNoteCollector::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
	{
		this->current += dt;

	}

	void EventMulticaster::note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->note_on(dt, channel, note, velocity);
		}
	}

	void EventMulticaster::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->note_off(dt, channel, note, velocity);
		}
	}

	void EventMulticaster::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->polyphonic_key_pressure(dt, channel, note, pressure);
		}
	}

	void EventMulticaster::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->control_change(dt, channel, controller, value);
		}
	}

	void EventMulticaster::program_change(Duration dt, Channel channel, Instrument program)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->program_change(dt, channel, program);
		}
	}

	void EventMulticaster::channel_pressure(Duration dt, Channel channel, uint8_t pressure)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->channel_pressure(dt, channel, pressure);
		}
	}

	void EventMulticaster::pitch_wheel_change(Duration dt, Channel channel, uint16_t value)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->pitch_wheel_change(dt, channel, value);
		}
	}

	void EventMulticaster::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->meta(dt, type, std::move(data),data_size);
		}
	}

	void EventMulticaster::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
	{
		for (std::shared_ptr<EventReceiver> receiver : this->receivers) {
			receiver->sysex(dt, std::move(data), data_size);
		}
	}

}