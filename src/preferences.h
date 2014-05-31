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

/// @file preferences.h
/// @brief Preferences dialogue
/// @see preferences.cpp
/// @ingroup configuration_ui

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <wx/dialog.h>

class wxButton;
class wxTreebook;
namespace agi { class OptionValue; }

class Preferences final : public wxDialog {
public:
	typedef std::function<void ()> Thunk;
private:
	wxTreebook *book;
	wxButton *applyButton;

	std::map<std::string, std::unique_ptr<agi::OptionValue>> pending_changes;
	std::vector<Thunk> pending_callbacks;
	std::vector<std::string> option_names;

	void OnOK(wxCommandEvent &);
	void OnCancel(wxCommandEvent &);
	void OnApply(wxCommandEvent &);
	void OnResetDefault(wxCommandEvent&);

public:
	Preferences(wxWindow *parent);

	/// Add an option to be set when the OK or Apply button is clicked
	/// @param new_value Clone of the option with the new value to copy over
	void SetOption(std::unique_ptr<agi::OptionValue> new_value);

	/// All a function to call when the OK or Apply button is clicked
	/// @param callback Function to call
	void AddPendingChange(Thunk const& callback);

	/// Add an option which can be changed via the dialog
	/// @param name Name of the option
	///
	/// This is used for resetting options to the defaults. We don't want to
	/// simply revert to the default config file as a bunch of things other than
	/// user options are stored in it. Perhaps that should change in the future.
	void AddChangeableOption(std::string const& name);
};
