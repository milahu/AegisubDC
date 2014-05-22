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
#include "options.h"
#include "string_codec.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "version.h"

#include <libaegisub/ass/uuencode.h>
#include <libaegisub/fs.h>

DEFINE_SIMPLE_EXCEPTION(AssParseError, SubtitleFormatParseError, "subtitle_io/parse/ass")

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
const char *format(AssEntryGroup group) {
	if (group == AssEntryGroup::DIALOGUE)
		return "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text" LINEBREAK;
	if (group == AssEntryGroup::STYLE)
		return "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding" LINEBREAK;
	return nullptr;
}

struct Writer {
	TextFileWriter file;
	AssEntryGroup group = AssEntryGroup::INFO;

	Writer(agi::fs::path const& filename, std::string const& encoding)
	: file(filename, encoding)
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

				file.WriteLineToFile(line.GroupHeader());
				if (const char *str = format(line.Group()))
					file.WriteLineToFile(str, false);

				group = line.Group();
			}

			file.WriteLineToFile(line.GetEntryData());
		}
	}

	void Write(ProjectProperties const& properties) {
		file.WriteLineToFile("");
		file.WriteLineToFile("[Aegisub Project Garbage]");

		WriteIfNotEmpty("Automation Scripts: ", properties.automation_scripts);
		WriteIfNotEmpty("Export Filters: ", properties.export_filters);
		WriteIfNotEmpty("Export Encoding: ", properties.export_encoding);
		WriteIfNotEmpty("Last Style Storage: ", properties.style_storage);
		WriteIfNotEmpty("Audio File: ", properties.audio_file);
		WriteIfNotEmpty("Video File: ", properties.video_file);
		WriteIfNotEmpty("Timecodes File: ", properties.timecodes_file);
		WriteIfNotEmpty("Keyframes File: ", properties.keyframes_file);

		WriteIfNotZero("Video AR Mode: ", properties.ar_mode);
		WriteIfNotZero("Video AR Value: ", properties.ar_value);

		if (OPT_GET("App/Save UI State")->GetBool()) {
			WriteIfNotZero("Video Zoom Percent: ", properties.video_zoom);
			WriteIfNotZero("Scroll Position: ", properties.scroll_position);
			WriteIfNotZero("Active Line: ", properties.active_row);
			WriteIfNotZero("Video Position: ", properties.video_position);
		}
	}

	void WriteIfNotEmpty(const char *key, std::string const& value) {
		if (!value.empty())
			file.WriteLineToFile(key + value);
	}

	template<typename Number>
	void WriteIfNotZero(const char *key, Number n) {
		if (n != Number{})
			file.WriteLineToFile(key + std::to_string(n));
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
			std::string encoded_data = inline_string_encode(edi.second.second);
			if (4*edi.second.second.size() < 3*encoded_data.size()) {
				// the inline_string encoding grew the data by more than uuencoding would
				// so base64 encode it instead
				line += "u"; // marker for uuencoding
				line += agi::ass::UUEncode(edi.second.second, false);
			} else {
				line += "e"; // marker for inline_string encoding (escaping)
				line += encoded_data;
			}
			file.WriteLineToFile(line);
		}
	}
};
}

void AssSubtitleFormat::WriteFile(const AssFile *src, agi::fs::path const& filename, agi::vfr::Framerate const& fps, std::string const& encoding) const {
	Writer writer(filename, encoding);
	writer.Write(src->Info);
	writer.Write(src->Properties);
	writer.Write(src->Styles);
	writer.Write(src->Attachments);
	writer.Write(src->Events);
	writer.WriteExtradata(src->Extradata);
}
