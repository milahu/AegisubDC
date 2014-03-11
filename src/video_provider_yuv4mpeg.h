// Copyright (c) 2009, Karl Blomster
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

/// @file video_provider_yuv4mpeg.h
/// @see video_provider_yuv4mpeg.cpp
/// @ingroup video_input
///

#include "include/aegisub/video_provider.h"

#include <cstdio>
#include <vector>

/// the maximum allowed header length, in bytes
#define YUV4MPEG_HEADER_MAXLEN 128

/// @class YUV4MPEGVideoProvider
/// @brief Implements reading of YUV4MPEG uncompressed video files
class YUV4MPEGVideoProvider : public VideoProvider {
	/// Pixel formats
	enum Y4M_PixelFormat {
		Y4M_PIXFMT_NONE		= -1,	/// not set/unknown

		/// 4:2:0 sampling variants.
		/// afaict the only difference between these three
		/// is the chroma sample location, and nobody cares about that.
		Y4M_PIXFMT_420JPEG,		/// 4:2:0, H/V centered, for JPEG/MPEG-1
		Y4M_PIXFMT_420MPEG2,	/// 4:2:0, H cosited, for MPEG-2
		Y4M_PIXFMT_420PALDV,	/// 4:2:0, alternating Cb/Cr, for PAL-DV

		Y4M_PIXFMT_411,			/// 4:1:1, H cosited
		Y4M_PIXFMT_422,			/// 4:2:2, H cosited
		Y4M_PIXFMT_444,			/// 4:4:4, i.e. no chroma subsampling
		Y4M_PIXFMT_444ALPHA,	/// 4:4:4 plus alpha channel

		Y4M_PIXFMT_MONO		/// luma only (grayscale)
	};


	/// Interlacing mode for an entire stream
	enum Y4M_InterlacingMode {
		Y4M_ILACE_NOTSET = -1,	/// undefined
		Y4M_ILACE_PROGRESSIVE,	/// progressive (no interlacing)

		Y4M_ILACE_TFF,			/// interlaced, top field first
		Y4M_ILACE_BFF,			/// interlaced, bottom field first

		Y4M_ILACE_MIXED,		/// mixed interlaced/progressive, possibly with RFF flags
		Y4M_ILACE_UNKNOWN		/// unknown interlacing mode (not the same as undefined)
	};


	/// Frame information flags
	enum Y4M_FrameFlags {
		Y4M_FFLAG_NOTSET	= -1,		/// undefined
		Y4M_FFLAG_NONE		= 0x0000,	/// no flags set

		/// field order/repeat field flags
		Y4M_FFLAG_R_TFF		= 0x0001,	/// top field first
		Y4M_FFLAG_R_TFF_R	= 0x0002,	/// top field first, and repeat that field
		Y4M_FFLAG_R_BFF		= 0x0004,	/// bottom field first
		Y4M_FFLAG_R_BFF_R	= 0x0008,	/// bottom field first, and repeat that field
		Y4M_FFLAG_R_P		= 0x0010,	/// progressive
		Y4M_FFLAG_R_P_R		= 0x0020,	/// progressive, and repeat frame once
		Y4M_FFLAG_R_P_RR	= 0x0040,	/// progressive, and repeat frame twice

		/// temporal sampling flags
		Y4M_FFLAG_T_P		= 0x0080,	/// progressive (fields sampled at the same time)
		Y4M_FFLAG_T_I		= 0x0100,	/// interlaced (fields sampled at different times)

		/// chroma subsampling flags
		Y4M_FFLAG_C_P		= 0x0200,	/// progressive (whole frame subsampled)
		Y4M_FFLAG_C_I		= 0x0400,	/// interlaced (fields subsampled independently)
		Y4M_FFLAG_C_UNKNOWN = 0x0800	/// unknown (only allowed for non-4:2:0 sampling)
	};

	FILE *sf = nullptr;		/// source file
	bool inited = false;	/// initialization state

	int w = 0, h = 0;	/// frame width/height
	int num_frames = -1; /// length of file in frames
	int frame_sz;	/// size of each frame in bytes
	int luma_sz;	/// size of the luma plane of each frame, in bytes
	int chroma_sz;	/// size of one of the two chroma planes of each frame, in bytes

	Y4M_PixelFormat pixfmt = Y4M_PIXFMT_NONE;		/// colorspace/pixel format
	Y4M_InterlacingMode imode = Y4M_ILACE_NOTSET;	/// interlacing mode (for the entire stream)
	struct {
		int num;	/// numerator
		int den;	/// denominator
	} fps_rat;		/// framerate

	agi::vfr::Framerate fps;

	/// a list of byte positions detailing where in the file
	/// each frame header can be found
	std::vector<int64_t> seek_table;

	void CheckFileFormat();
	void ParseFileHeader(const std::vector<std::string>& tags);
	Y4M_FrameFlags ParseFrameHeader(const std::vector<std::string>& tags);
	std::vector<std::string> ReadHeader(int64_t startpos);
	int IndexFile();

public:
	YUV4MPEGVideoProvider(agi::fs::path const& filename, std::string const&);
	~YUV4MPEGVideoProvider();

	std::shared_ptr<VideoFrame> GetFrame(int n) override;

	int GetFrameCount() const override             { return num_frames; }
	int GetWidth() const override                  { return w; }
	int GetHeight() const override                 { return h; }
	double GetDAR() const override                 { return 0; }
	agi::vfr::Framerate GetFPS() const override    { return fps; }
	std::vector<int> GetKeyFrames() const override { return {}; }
	std::string GetColorSpace() const override     { return "TV.601"; }
	std::string GetDecoderName() const override    { return "YU4MPEG"; }
	bool WantsCaching() const override             { return true; }
};
