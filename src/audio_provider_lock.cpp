// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
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

/// @file audio_provider_lock.cpp
/// @brief An audio provider adapter for un-threadsafe audio providers
/// @ingroup audio_input

#include "include/aegisub/audio_provider.h"

#include <libaegisub/make_unique.h>

#include <memory>
#include <mutex>

namespace {
class LockAudioProvider final : public AudioProviderWrapper {
	mutable std::mutex mutex;

	void FillBuffer(void *buf, int64_t start, int64_t count) const override {
		std::unique_lock<std::mutex> lock(mutex);
		source->GetAudio(buf, start, count);
	}

public:
	LockAudioProvider(std::unique_ptr<AudioProvider> src)
	: AudioProviderWrapper(std::move(src))
	{
	}
};
}

std::unique_ptr<AudioProvider> CreateLockAudioProvider(std::unique_ptr<AudioProvider> src) {
	return agi::make_unique<LockAudioProvider>(std::move(src));
}
