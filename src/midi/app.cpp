#ifndef TEST_BUILD

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstdint>
#include "shell/command-line-parser.h"
#include "imaging/bitmap.h"
#include "imaging/bmp-format.h"
#include "imaging/bmp-format.h"
#include "midi/midi.h"

using namespace imaging;
using namespace shell;
using namespace midi;
using namespace std;

int getWidth(vector<NOTE>& notes)
{
	int res = 0;
	for (NOTE n : notes)
	{
		if (value(n.start + n.duration) > res)
		{
			res = value(n.start + n.duration);
		}
	}
	return res;
}

void draw_rectangle(Bitmap& bitmap,
	const Position& pos,
	const uint32_t& width,
	const uint32_t& height,
	const Color& color)
{
	for (uint32_t i = 0; i < width; i++)
	{
		for (uint32_t j = 0; j < height; j++)
		{
			bitmap[Position(pos.x + i, pos.y + j)] = color;
		}
	}
}

int getMinNoteNumber(vector<NOTE>& notes) {
	int res = 128;
	for (NOTE note : notes)
	{
		if (value(note.note_number) < res) {
			res = value(note.note_number);
		}
	}
	return res;
}

int getMaxNoteNumber(vector<NOTE>& notes) {
	int res = 0;
	for (NOTE note : notes)
	{
		if (value(note.note_number) > res) {
			res = value(note.note_number);
		}
	}
	return res;
}

int main(int argn, char* argv[])
{
	string file = ".\\tmp\\12-notes.mid";
	string outfile = ".\\tmp\\bitmaps\\frame%d.bmp";
	uint32_t frame_width = 0;
	uint32_t step = 1;
	uint32_t scale = 10;
	uint32_t height_of_note = 16;

	CommandLineParser parser;
	parser.add_argument(std::string("-w"), &frame_width);
	parser.add_argument(std::string("-d"), &step);
	parser.add_argument(std::string("-s"), &scale);
	parser.add_argument(std::string("-h"), &height_of_note);
	parser.process(std::vector<std::string>(argv + 1, argv + argn));
	vector<string> positionalArgs = parser.positional_arguments();

	if (positionalArgs.size() >= 1) {
		file = positionalArgs[0];
		if (positionalArgs.size() >= 2) {
			outfile = positionalArgs[1];
		}
	}

	ifstream in(file, ifstream::binary);
	vector<NOTE> notes = read_notes(in);

	uint32_t bitmapwidth = getWidth(notes) / scale;
	uint32_t bitmapheight = 127 * height_of_note;

	if (frame_width == 0) {
		frame_width = bitmapwidth;
	}

	uint16_t highest_note = getMaxNoteNumber(notes);
	uint16_t lowest_note = getMinNoteNumber(notes);

	Bitmap bitmap(bitmapwidth, bitmapheight);

	for (int i = 0; i < 128; i++) {
		for (NOTE n : notes) {
			if (n.note_number == NoteNumber(i)) {
				draw_rectangle(bitmap,
					Position(value(n.start) / scale,
					(127 - value(n.note_number))*height_of_note),
					value(n.duration) / scale,
					height_of_note, Color(1, 0, 0));
			}
		}
	}

	bitmap = *bitmap.slice(0, (127 - highest_note) * height_of_note, bitmapwidth, (highest_note - lowest_note + 1)*height_of_note).get();

	for (int i = 0; i <= bitmapwidth - frame_width; i += step)
	{
		std::cout << "generated frame " + std::to_string(i / step) + " of " + std::to_string((bitmapwidth - frame_width) / step) + " (" + std::to_string((int)ceil(((float)i / (bitmapwidth - frame_width)) * 100)) + "%)" << endl;
		Bitmap newBitmap = *bitmap.slice(i, 0, frame_width, (highest_note - lowest_note + 1)*height_of_note).get();
		string temp = outfile;
		string out = temp.replace(temp.find("%d"), std::string("%d").size(), to_string(i / step));
		save_as_bmp(out, newBitmap);
	}
}

#endif

