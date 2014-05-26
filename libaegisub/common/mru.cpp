// Copyright (c) 2010, Amar Takhar <verm@aegisub.org>
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

#include "libaegisub/mru.h"

#include "libaegisub/cajun/writer.h"

#include "libaegisub/io.h"
#include "libaegisub/json.h"
#include "libaegisub/log.h"
#include "libaegisub/option.h"
#include "libaegisub/option_value.h"

namespace agi {
MRUManager::MRUManager(agi::fs::path const& config, std::pair<const char *, size_t> default_config, agi::Options *options)
: config_name(config)
, options(options)
, option_names({
	{"Audio", "Limits/MRU"},
	{"Find", "Limits/Find Replace"},
	{"Keyframes", "Limits/MRU"},
	{"Replace", "Limits/Find Replace"},
	{"Subtitle", "Limits/MRU"},
	{"Timecodes", "Limits/MRU"},
	{"Video", "Limits/MRU"},
})
{
	LOG_D("agi/mru") << "Loading MRU List";

	json::Object root(json_util::file(config, default_config));
	for (auto const& it : root)
		Load(it.first, it.second);
}

MRUManager::MRUListMap &MRUManager::Find(std::string const& key) {
	auto index = mru.find(key);
	if (index == mru.end())
		throw MRUErrorInvalidKey("Invalid key value");
	return index->second;
}

void MRUManager::Add(std::string const& key, agi::fs::path const& entry) {
	MRUListMap &map = Find(key);
	auto it = find(begin(map), end(map), entry);
	if (it == begin(map) && it != end(map))
		return;
	if (it != end(map))
		rotate(begin(map), it, it);
	else {
		map.insert(begin(map), entry);
		Prune(key, map);
	}

	Flush();
}

void MRUManager::Remove(std::string const& key, agi::fs::path const& entry) {
	auto& map = Find(key);
	map.erase(remove(begin(map), end(map), entry), end(map));
	Flush();
}

const MRUManager::MRUListMap* MRUManager::Get(std::string const& key) {
	return &Find(key);
}

agi::fs::path const& MRUManager::GetEntry(std::string const& key, const size_t entry) {
	const MRUManager::MRUListMap *const map = Get(key);

	if (entry >= map->size())
		throw MRUErrorIndexOutOfRange("Requested element index is out of range.");

	return *next(map->begin(), entry);
}

void MRUManager::Flush() {
	json::Object out;

	for (auto const& mru_map : mru) {
		json::Array &array = out[mru_map.first];
		transform(begin(mru_map.second), end(mru_map.second),
			back_inserter(array), [](agi::fs::path const& p) { return p.string(); });
	}

	json::Writer::Write(out, io::Save(config_name).Get());
}

void MRUManager::Prune(std::string const& key, MRUListMap& map) const {
	size_t limit = 16u;
	if (options) {
		auto it = option_names.find(key);
		if (it != option_names.end())
			limit = (size_t)options->Get(it->second)->GetInt();
	}
	map.resize(std::min(limit, map.size()));
}

void MRUManager::Load(std::string const& key, const json::Array& array) {
	try {
		transform(begin(array), end(array),
			back_inserter(mru[key]), [](std::string const& s) { return s; });
	}
	catch (json::Exception const&) {
		// Out of date MRU file; just discard the data and skip it
	}
	Prune(key, mru[key]);
}

}
