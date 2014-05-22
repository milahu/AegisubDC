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

#include "libaegisub/log.h"

#include "libaegisub/cajun/elements.h"
#include "libaegisub/cajun/writer.h"
#include "libaegisub/dispatch.h"
#include "libaegisub/time.h"
#include "libaegisub/util.h"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/range/algorithm/remove_if.hpp>

namespace agi {
	namespace log {

/// Global log sink.
LogSink *log;

/// Short Severity ID
/// Keep this ordered the same as Severity
const char *Severity_ID = "EAWID";

LogSink::LogSink() : queue(dispatch::Create()) { }

LogSink::~LogSink() {
	// The destructor for emitters may try to log messages, so disable all the
	// emitters before destructing any
	decltype(emitters) emitters_temp;
	queue->Sync([&]{ swap(emitters_temp, emitters); });
}

void LogSink::Log(SinkMessage const& sm) {
	queue->Async([=]{
		messages.push_back(sm);
		for (auto& em : emitters) em->log(&messages.back());
	});
}

void LogSink::Subscribe(std::unique_ptr<Emitter> em) {
	LOG_D("agi/log/emitter/subscribe") << "Subscribe: " << this;
	auto tmp = em.release();
	queue->Sync([=] { emitters.emplace_back(tmp); });
}

void LogSink::Unsubscribe(Emitter *em) {
	queue->Sync([=] {
		emitters.erase(
			boost::remove_if(emitters, [=](std::unique_ptr<Emitter> const& e) { return e.get() == em; }),
			emitters.end());
	});
	LOG_D("agi/log/emitter/unsubscribe") << "Un-Subscribe: " << this;
}

decltype(LogSink::messages) LogSink::GetMessages() const {
	decltype(messages) ret;
	queue->Sync([&] { ret = messages; });
	return ret;
}

Message::Message(const char *section, Severity severity, const char *file, const char *func, int line)
: msg(buffer, sizeof buffer)
{
	sm.section = section;
	sm.severity = severity;
	sm.file = file;
	sm.func = func;
	sm.line = line;
	sm.tv = util::time_log();
}

Message::~Message() {
	sm.message = std::string(buffer, (std::string::size_type)msg.tellp());
	agi::log::log->Log(sm);
}

JsonEmitter::JsonEmitter(fs::path const& directory)
: fp(new boost::filesystem::ofstream(unique_path(directory/util::strftime("%Y-%m-%d-%H-%M-%S-%%%%%%%%.json"))))
{
	WriteTime("open");
}

JsonEmitter::~JsonEmitter() {
	WriteTime("close");
}

void JsonEmitter::log(SinkMessage *sm) {
	json::Object entry;
	entry["sec"]      = (int64_t)sm->tv.tv_sec;
	entry["usec"]     = (int64_t)sm->tv.tv_usec;
	entry["severity"] = sm->severity;
	entry["section"]  = sm->section;
	entry["file"]     = sm->file;
	entry["func"]     = sm->func;
	entry["line"]     = sm->line;
	entry["message"]  = sm->message;
	json::Writer::Write(entry, *fp);
	fp->flush();
}

void JsonEmitter::WriteTime(const char *key) {
	auto time = util::time_log();
	json::Object obj;
	json::Array &timeval = obj[key];
	timeval.push_back((int64_t)time.tv_sec);
	timeval.push_back((int64_t)time.tv_usec);
	json::Writer::Write(obj, *fp);
}

	} // namespace log
} // namespace agi
