// Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Aegisub Project http://www.aegisub.org/

#include "subtitle_format_ass.h"

#include "ass_attachment.h"
#include "ass_dialogue.h"
#include "ass_info.h"
#include "ass_file.h"
#include "ass_style.h"
#include "ass_parser.h"
#include "string_codec.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "version.h"

#include <libaegisub/fs.h>

DEFINE_SIMPLE_EXCEPTION(AssParseError, SubtitleFormatParseError, "subtitle_io/parse/ass")

AssSubtitleFormat::AssSubtitleFormat()
: SubtitleFormat("Advanced Substation Alpha")
{
}

std::vector<std::string> AssSubtitleFormat::GetReadWildcards() const {
	return {"ass", "ssa"};
}

std::vector<std::string> AssSubtitleFormat::GetWriteWildcards() const {
	return {"ass", "ssa"};
}

void AssSubtitleFormat::ReadFile(AssFile *target, agi::fs::path const& filename, agi::vfr::Framerate const& fps, std::string const& encoding) const {
	TextFileReader file(filename, encoding);
	int version = !agi::fs::HasExtension(filename, "ssa");
	AssParser parser(target, version);

	while (file.HasMoreLines()) {
		std::string line = file.ReadLineFromFile();
		try {
			parser.AddLine(line);
		}
		catch (const char *err) {
			throw AssParseError("Error processing line: " + line + ": " + err, nullptr);
		}
	}
}

#ifdef _WIN32
#define LINEBREAK "\r\n"
#else
#define LINEBREAK "\n"
#endif

namespace {
std::string format(AssEntryGroup group, bool ssa) {
	if (group == AssEntryGroup::DIALOGUE) {
		if (ssa)
			return "Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text" LINEBREAK;
		else
			return "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text" LINEBREAK;
	}

	if (group == AssEntryGroup::STYLE) {
		if (ssa)
			return "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding" LINEBREAK;
		else
			return "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding" LINEBREAK;
	}

	return "";
}

struct Writer {
	TextFileWriter file;
	bool ssa;
	AssEntryGroup group = AssEntryGroup::INFO;

	Writer(agi::fs::path const& filename, std::string const& encoding)
	: file(filename, encoding)
	, ssa(agi::fs::HasExtension(filename, "ssa"))
	{
		file.WriteLineToFile("[Script Info]");
		file.WriteLineToFile(std::string("; Script generated by Aegisub ") + GetAegisubLongVersionString());
		file.WriteLineToFile("; http://www.aegisub.org/");
	}

	template<typename T>
	void Write(T const& list) {
		for (auto const& line : list) {
			if (line.Group() != group) {
				// Add a blank line between each group
				file.WriteLineToFile("");

				file.WriteLineToFile(line.GroupHeader(ssa));
				file.WriteLineToFile(format(line.Group(), ssa), false);

				group = line.Group();
			}

			file.WriteLineToFile(ssa ? line.GetSSAText() : line.GetEntryData());
		}
	}

	void WriteExtradata(AegisubExtradataMap const& extradata) {
		if (extradata.size() == 0)
			return;

		group = AssEntryGroup::EXTRADATA;
		file.WriteLineToFile("");
		file.WriteLineToFile("[Aegisub Extradata]");
		for (auto const& edi : extradata) {
			std::string line = "Data: ";
			line += std::to_string(edi.first);
			line += ",";
			line += inline_string_encode(edi.second.first);
			line += ",";
			line += inline_string_encode(edi.second.second);
			file.WriteLineToFile(line);
		}
	}
};
}

void AssSubtitleFormat::WriteFile(const AssFile *src, agi::fs::path const& filename, agi::vfr::Framerate const& fps, std::string const& encoding) const {
	Writer writer(filename, encoding);

	writer.Write(src->Info);
	writer.Write(src->Styles);
	writer.Write(src->Attachments);
	writer.Write(src->Events);
	writer.WriteExtradata(src->Extradata);
}
