// Copyright (c) 2005-2007, Rodrigo Braz Monteiro
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

/// @file audio_player.cpp
/// @brief Baseclass for audio players
/// @ingroup audio_output
///

#include "config.h"

#include "include/aegisub/audio_player.h"

#include "audio_controller.h"
#include "factory_manager.h"
#include "options.h"

#include <boost/range/iterator_range.hpp>

AudioPlayer::AudioPlayer(AudioProvider *provider)
: provider(provider)
{
}

std::unique_ptr<AudioPlayer> CreateAlsaPlayer(AudioProvider *providers);
std::unique_ptr<AudioPlayer> CreateDirectSoundPlayer(AudioProvider *providers);
std::unique_ptr<AudioPlayer> CreateDirectSound2Player(AudioProvider *providers);
std::unique_ptr<AudioPlayer> CreateOpenALPlayer(AudioProvider *providers);
std::unique_ptr<AudioPlayer> CreatePortAudioPlayer(AudioProvider *providers);
std::unique_ptr<AudioPlayer> CreatePulseAudioPlayer(AudioProvider *providers);
std::unique_ptr<AudioPlayer> CreateOSSPlayer(AudioProvider *providers);

namespace {
	struct factory {
		const char *name;
		std::unique_ptr<AudioPlayer> (*create)(AudioProvider *);
		bool hidden;
	};

	const factory factories[] = {
#ifdef WITH_ALSA
		{"ALSA", CreateAlsaPlayer, false},
#endif
#ifdef WITH_DIRECTSOUND
		{"DirectSound-old", CreateDirectSoundPlayer, false},
		{"DirectSound", CreateDirectSound2Player, false},
#endif
#ifdef WITH_OPENAL
		{"OpenAL", CreateOpenALPlayer, false},
#endif
#ifdef WITH_PORTAUDIO
		{"PortAudio", CreatePortAudioPlayer, false},
#endif
#ifdef WITH_LIBPULSE
		{"PulseAudio", CreatePulseAudioPlayer, false},
#endif
#ifdef WITH_OSS
		{"OSS", CreateOSSPlayer, false},
#endif
	};
}

std::vector<std::string> AudioPlayerFactory::GetClasses() {
	return ::GetClasses(boost::make_iterator_range(std::begin(factories), std::end(factories)));
}

std::unique_ptr<AudioPlayer> AudioPlayerFactory::GetAudioPlayer(AudioProvider *provider) {
	if (std::distance(std::begin(factories), std::end(factories)) == 0)
		throw agi::NoAudioPlayersError("No audio players are available.", nullptr);

	auto preferred = OPT_GET("Audio/Player")->GetString();
	auto sorted = GetSorted(boost::make_iterator_range(std::begin(factories), std::end(factories)), preferred);

	std::string error;
	for (auto factory : sorted) {
		try {
			return factory->create(provider);
		}
		catch (agi::AudioPlayerOpenError const& err) {
			error += std::string(factory->name) + " factory: " + err.GetChainedMessage() + "\n";
		}
	}
	throw agi::AudioPlayerOpenError(error, nullptr);
}
