/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#include "stdafx.h"

#include "common/config-manager.h"

#include "gui/chooser.h"
#include "gui/newgui.h"
#include "gui/ListWidget.h"

#include "scumm/dialogs.h"
#include "scumm/sound.h"
#include "scumm/scumm.h"
#include "scumm/imuse.h"
#include "scumm/player_v2.h"
#include "scumm/verbs.h"
#include "sound/mididrv.h"
#include "sound/mixer.h"

#ifndef DISABLE_HELP
#include "scumm/help.h"
#endif

#ifdef _WIN32_WCE
#include "gapi_keys.h"
extern bool _get_key_mapping;
extern void force_keyboard(bool);
extern void save_key_mapping();
extern void load_key_mapping();
#endif

namespace Scumm {

struct ResString {
	int num;
	char string[80];
};

#ifdef __PALM_OS__
static ResString *string_map_table_v7;
static ResString *string_map_table_v6;
static ResString *string_map_table_v5;
#else
static ResString string_map_table_v7[] = {
	{96, "game name and version"}, //that's how it's supposed to be
	{77, "Select a game to LOAD"},
	{76, "Name your SAVE game"},
	{70, "save"}, //boot8
	{71, "load"}, //boot9
	{72, "play"}, //boot10
	{73, "cancel"}, //boot11
	{74, "quit"}, //boot12
	{75, "ok"}, //boot13
	{85, "game paused"}, // boot3					

	/* this is the almost complete string map for v7
	{63, "how may I serve you?"},
	{64, "the dig v1.0"}, //(game name/version)
	{67, "text display only"},
	{68, "c:\\dig"}, //boot007 (save path ?)
	{69, "the dig"}, //boot21 (game name)
	{70, "save"}, //boot8
	{71, "load"}, //boot9
	{72, "play"}, //boot10
	{73, "cancel"}, //boot11
	{74, "quit"}, //boot12
	{75, "ok"}, //boot13
	{76, "name your save game"}, //boot19
	{77, "select a game to load"}, //boot20
	{78, "you must enter a name"},//boot14
	{79, "saving '%s'"}, //boot17
	{80, "loading '%s'"}, //boot18
	{81, "the game was NOT saved"}, //boot15
	{82, "the game was NOT loaded"}, //boot16
	{83, "how may I serve you?"},
	{84, "how may I serve you?"},
	{85, "game paused"}, // boot3
	{86, "Are you sure you want to restart"},
	{87, "Are you sure you want to quit?"}, //boot05
	{89, "how may I serve you?"},
	{90, "music"}, //boot22
	{91, "voice"}, //boot23
	{92, "sfx"}, //boot24
	{93, "disabled"}, //boot25
	{94, "text speed"}, //boot26
	{95, "text display"}, //boot27
	{96, "the dig v1.0"},*/
	
};

static ResString string_map_table_v6[] = {
	{117, "How may I serve you?"}, 
	{109, "Select a game to LOAD"}, 
	{108, "Name your SAVE game"}, 
	{96, "Save"}, 
	{97, "Load"}, 
	{98, "Play"}, 
	{99, "Cancel"}, 
	{100, "Quit"}, 
	{101, "OK"}, 
	{93, "Game paused"}, 
};

static ResString string_map_table_v5[] = {
	{28, "How may I serve you?"}, 
	{20, "Select a game to LOAD"},
	{19, "Name your SAVE game"},
	{7, "Save"},
	{8, "Load"},
	{9, "Play"},
	{10, "Cancel"},
	{11, "Quit"},
	{12, "OK"},
	{4, "Game paused"}
};
#endif

#pragma mark -


const Common::String ScummDialog::queryResString(int stringno) {
	byte *result;

	if (stringno == 0)
		return String();

	if (_scumm->_version >= 7)
		result = _scumm->getStringAddressVar(string_map_table_v7[stringno - 1].num);
	else if (_scumm->_version == 6)
		result = _scumm->getStringAddressVar(string_map_table_v6[stringno - 1].num);
	else if (_scumm->_version == 5)
		result = _scumm->getStringAddress(string_map_table_v5[stringno - 1].num);
	else
		// TODO: For V8 games, maybe grab the strings from the language file?
		return string_map_table_v5[stringno - 1].string;

	if (result && *result == '/') {
		byte tmp[256];
		_scumm->translateText(result, tmp);
		
		// FIXME: AARGH! We shouldn't just strcpy into the data we got from getStringAddress
		strcpy((char *)result, (char *)tmp);
	}

	if (!result || *result == '\0') {	// Gracelessly degrade to english :)
		return string_map_table_v5[stringno - 1].string;
	}

	// Convert to a proper string (take care of FF codes)
	byte chr;
	String tmp;
	while ((chr = *result++)) {		
		if (chr == 0xFF) {
			result += 3;
		} else if (chr != '@') {
			tmp += chr;
		}
	}
	return tmp;
}

#pragma mark -

enum {
	kSaveCmd = 'SAVE',
	kLoadCmd = 'LOAD',
	kPlayCmd = 'PLAY',
	kOptionsCmd = 'OPTN',
	kHelpCmd = 'HELP',
	kAboutCmd = 'ABOU',
	kQuitCmd = 'QUIT'
};

class SaveLoadChooser : public ChooserDialog {
	typedef Common::String String;
	typedef Common::StringList StringList;
protected:
	bool _saveMode;

public:
	SaveLoadChooser(const String &title, const StringList& list, const String &buttonLabel, bool saveMode);
	
	virtual void handleCommand(CommandSender *sender, uint32 cmd, uint32 data);
	const String &getResultString() const;
};

SaveLoadChooser::SaveLoadChooser(const String &title, const StringList& list, const String &buttonLabel, bool saveMode)
	: ChooserDialog(title, list, buttonLabel, 182), _saveMode(saveMode) {

	_list->setEditable(saveMode);
	_list->setNumberingMode(saveMode ? kListNumberingOne : kListNumberingZero);
}

const Common::String &SaveLoadChooser::getResultString() const {
	return _list->getSelectedString();
}

void SaveLoadChooser::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	int selItem = _list->getSelected();
	switch (cmd) {
	case kListItemActivatedCmd:
	case kListItemDoubleClickedCmd:
		if (selItem >= 0) {
			if (_saveMode || !getResultString().isEmpty()) {
				setResult(selItem);
				close();
			}
		}
		break;
	case kListSelectionChangedCmd:
		if (_saveMode) {
			_list->startEditMode();
		}
		// Disable button if nothing is selected, or (in load mode) if an empty
		// list item is selected. We allow choosing an empty item in save mode
		// because we then just assign a default name.
		_chooseButton->setEnabled(selItem >= 0 && (_saveMode || !getResultString().isEmpty()));
		_chooseButton->draw();
		break;
	default:
		ChooserDialog::handleCommand(sender, cmd, data);
	}
}

Common::StringList generateSavegameList(ScummEngine *scumm, bool saveMode) {
	// Get savegame names
	Common::StringList l;
	char name[32];
	uint i = saveMode ? 1 : 0;
	bool avail_saves[81];

	SaveFileManager *mgr = OSystem::instance()->get_savefile_manager();

	scumm->listSavegames(avail_saves, ARRAYSIZE(avail_saves), mgr);
	for (; i < ARRAYSIZE(avail_saves); i++) {
		if (avail_saves[i])
			scumm->getSavegameName(i, name, mgr);
		else
			name[0] = 0;
		l.push_back(name);
	}

	delete mgr;

	return l;
}

enum {
	kRowHeight = 18,
	kBigButtonWidth = 90,
	kMainMenuWidth 	= (kBigButtonWidth + 2 * 8),
	kMainMenuHeight 	= 7 * kRowHeight + 3 * 5 + 7 + 5
};

#define addBigButton(label, cmd, hotkey) \
	new ButtonWidget(this, x, y, kBigButtonWidth, 16, label, cmd, hotkey); y += kRowHeight

MainMenuDialog::MainMenuDialog(ScummEngine *scumm)
	: ScummDialog(scumm, (320 - kMainMenuWidth) / 2, (200 - kMainMenuHeight)/2, kMainMenuWidth, kMainMenuHeight) {
	int y = 7;

	const int x = (_w - kBigButtonWidth) / 2;
	addBigButton("Resume", kPlayCmd, 'P');
	y += 5;

	addBigButton("Load", kLoadCmd, 'L');
	addBigButton("Save", kSaveCmd, 'S');
	y += 5;

	addBigButton("Options", kOptionsCmd, 'O');
#ifndef DISABLE_HELP
	addBigButton("Help", kHelpCmd, 'H');
#endif
	addBigButton("About", kAboutCmd, 'A');
	y += 5;

	addBigButton("Quit", kQuitCmd, 'Q');

	//
	// Create the sub dialog(s)
	//
	_aboutDialog = new AboutDialog();
#ifndef DISABLE_HELP
	_helpDialog = new HelpDialog(scumm);
#endif
}

MainMenuDialog::~MainMenuDialog() {
	delete _aboutDialog;
#ifndef DISABLE_HELP
	delete _helpDialog;
#endif
}

void MainMenuDialog::open() {
#ifdef _WIN32_WCE
	force_keyboard(true);
#endif

	ScummDialog::open();
}

void MainMenuDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kSaveCmd:
		save();
		break;
	case kLoadCmd:
		load();
		break;
	case kPlayCmd:
		close();
		break;
	case kOptionsCmd:
		_scumm->optionsDialog();
		break;
	case kAboutCmd:
		_aboutDialog->runModal();
		break;
#ifndef DISABLE_HELP
	case kHelpCmd:
		_helpDialog->runModal();
		break;
#endif
	case kQuitCmd:
		_scumm->_quit = true;
		close();
		break;
	default:
		ScummDialog::handleCommand(sender, cmd, data);
	}
}

void MainMenuDialog::close() {
	ScummDialog::close();

#ifdef _WIN32_WCE
	force_keyboard(false);
#endif
}

void MainMenuDialog::save() {
	int idx;
	SaveLoadChooser dialog("Save game:", generateSavegameList(_scumm, true), "Save", true);
	idx = dialog.runModal();
	if (idx >= 0) {
		const String &result = dialog.getResultString();
		char buffer[20];
		const char *str;
		if (result.isEmpty()) {
			// If the user was lazy and entered no save name, come up with a default name.
			sprintf(buffer, "Save %d", idx + 1);
			str = buffer;
		} else
			str = result.c_str();
		_scumm->requestSave(idx + 1, str);
		close();
	}
}

void MainMenuDialog::load() {
	int idx;
	SaveLoadChooser dialog("Load game:", generateSavegameList(_scumm, false), "Load", false);
	idx = dialog.runModal();
	if (idx >= 0) {
		_scumm->requestLoad(idx);
		close();
	}
}

#pragma mark -

enum {
	kMasterVolumeChanged	= 'mavc',
	kMusicVolumeChanged		= 'muvc',
	kSfxVolumeChanged		= 'sfvc',
	kOKCmd					= 'ok  ',
	kCancelCmd				= 'cncl'
};

enum {
	kKeysCmd = 'KEYS'
};

#ifndef _WIN32_WCE
OptionsDialog::OptionsDialog(ScummEngine *scumm)
	: ScummDialog(scumm, 40, 30, 240, 124) {
#else
OptionsDialog::OptionsDialog(ScummEngine *scumm)
	: ScummDialog(scumm, 40, 30, 240, 124 + kButtonHeight + 4) {
#endif
	//
	// Add the buttons
	//
#ifdef _WIN32_WCE
	addButton(_w - kButtonWidth - 8, _h - 24 - kButtonHeight - 4, "OK", kOKCmd, 'O');
	addButton(_w - 2 * kButtonWidth - 12, _h - 24 - kButtonHeight - 4, "Cancel", kCancelCmd, 'C');

	addButton(kButtonWidth+12, _h - 24, "Keys", kKeysCmd, 'K');
#else
	addButton(_w - kButtonWidth-8, _h - 24, "OK", kOKCmd, 'O');
	addButton(_w - 2 * kButtonWidth-12, _h - 24, "Cancel", kCancelCmd, 'C');
#endif

	//
	// Sound controllers
	//
	int yoffset = 8;

	_masterVolumeSlider = new SliderWidget(this, 5, yoffset, 185, 12, "Master volume: ", 100, kMasterVolumeChanged);
	_masterVolumeLabel = new StaticTextWidget(this, 200, yoffset + 2, 24, 16, "100%", kTextAlignLeft);
	_masterVolumeSlider->setMinValue(0); _masterVolumeSlider->setMaxValue(255);
	_masterVolumeLabel->setFlags(WIDGET_CLEARBG);
	yoffset += 16;

	_musicVolumeSlider = new SliderWidget(this, 5, yoffset, 185, 12, "Music volume: ", 100, kMusicVolumeChanged);
	_musicVolumeLabel = new StaticTextWidget(this, 200, yoffset+2, 24, 16, "100%", kTextAlignLeft);
	_musicVolumeSlider->setMinValue(0); _musicVolumeSlider->setMaxValue(255);
	_musicVolumeLabel->setFlags(WIDGET_CLEARBG);
	yoffset += 16;

	_sfxVolumeSlider = new SliderWidget(this, 5, yoffset, 185, 12, "SFX volume: ", 100, kSfxVolumeChanged);
	_sfxVolumeLabel  = new StaticTextWidget(this, 200, yoffset + 2, 24, 16, "100%", kTextAlignLeft);
	_sfxVolumeSlider->setMinValue(0); _sfxVolumeSlider->setMaxValue(255);
	_sfxVolumeLabel->setFlags(WIDGET_CLEARBG);
	yoffset += 16;

	//
	// Some misc options
	//
	subtitlesCheckbox = new CheckboxWidget(this, 15, 62, 200, 16, "Show subtitles", 0, 'S');

	//
	// Create the sub dialog(s)
	//
#ifdef _WIN32_WCE
	_keysDialog = new KeysDialog(scumm);
#endif
}

OptionsDialog::~OptionsDialog() {
#ifdef _WIN32_WCE
	delete _keysDialog;
#endif
}

void OptionsDialog::open() {
	ScummDialog::open();

	// display current sound settings
	_soundVolumeMaster = _scumm->_sound->_sound_volume_master;
	_soundVolumeMusic = _scumm->_sound->_sound_volume_music;
	_soundVolumeSfx = _scumm->_sound->_sound_volume_sfx;

	_masterVolumeSlider->setValue(_soundVolumeMaster);
	_musicVolumeSlider->setValue(_soundVolumeMusic);
	_sfxVolumeSlider->setValue(_soundVolumeSfx);

	_masterVolumeLabel->setValue(_soundVolumeMaster);
	_musicVolumeLabel->setValue(_soundVolumeMusic);
	_sfxVolumeLabel->setValue(_soundVolumeSfx);

	// update checkboxes, too
	subtitlesCheckbox->setState(_scumm->_noSubtitles == false);
}

void OptionsDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kKeysCmd:
#ifdef _WIN32_WCE
		_keysDialog->runModal();
#endif
		break;
	case kMasterVolumeChanged:
		_soundVolumeMaster = _masterVolumeSlider->getValue();
		_masterVolumeLabel->setValue(_soundVolumeMaster);
		_masterVolumeLabel->draw();
		break;
	case kMusicVolumeChanged:
		_soundVolumeMusic = _musicVolumeSlider->getValue();
		_musicVolumeLabel->setValue(_soundVolumeMusic);
		_musicVolumeLabel->draw();
		break;
	case kSfxVolumeChanged:
		_soundVolumeSfx = _sfxVolumeSlider->getValue();
		_sfxVolumeLabel->setValue(_soundVolumeSfx);
		_sfxVolumeLabel->draw();
		break;
	case kOKCmd: {
		// Update the sound settings 
		_scumm->_sound->_sound_volume_master = _soundVolumeMaster;	// Master
		_scumm->_sound->_sound_volume_music = _soundVolumeMusic;	// Music
		_scumm->_sound->_sound_volume_sfx = _soundVolumeSfx;	// SFX
		
		if (_scumm->_imuse) {
			_scumm->_imuse->set_music_volume(_soundVolumeMusic);
		}
		if (_scumm->_musicEngine) {
			_scumm->_musicEngine->setMasterVolume(_soundVolumeMaster);
		}

		_scumm->_mixer->setVolume(_soundVolumeSfx * _soundVolumeMaster / 255);
		_scumm->_mixer->setMusicVolume(_soundVolumeMusic);
		
		ConfMan.set("master_volume", _soundVolumeMaster);
		ConfMan.set("music_volume", _soundVolumeMusic);
		ConfMan.set("sfx_volume", _soundVolumeSfx);

		// Subtitles?
		_scumm->_noSubtitles = !subtitlesCheckbox->getState();
		ConfMan.set("nosubtitles", _scumm->_noSubtitles);
		
		// Finally flush the modified config
		ConfMan.flushToDisk();
		}
	case kCancelCmd:
		close();
		break;
	default:
		ScummDialog::handleCommand(sender, cmd, data);
	}
}

#ifndef DISABLE_HELP

#pragma mark -

enum {
	kNextCmd = 'NEXT',
	kPrevCmd = 'PREV'
};

HelpDialog::HelpDialog(ScummEngine *scumm)
	: ScummDialog(scumm, 5, 5, 310, 190) {

	_page = 1;
	_numPages = ScummHelp::numPages(scumm->_gameId);

	_prevButton = addButton(10, 170, "Previous", kPrevCmd, 'P');
	_nextButton = addButton(90, 170, "Next", kNextCmd, 'N');
	addButton(210, 170, "Close", kCloseCmd, 'C');
	_prevButton->clearFlags(WIDGET_ENABLED);

	_title = new StaticTextWidget(this, 10, 5, 290, 16, "", kTextAlignCenter);
	for (int i = 0; i < HELP_NUM_LINES; i++) {
		_key[i] = new StaticTextWidget(this, 10, 18 + (10 * i), 80, 16, "", kTextAlignLeft);
		_dsc[i] = new StaticTextWidget(this, 90, 18 + (10 * i), 210, 16, "", kTextAlignLeft);
	}

	displayKeyBindings();
}

void HelpDialog::displayKeyBindings() {

	String titleStr, *keyStr, *dscStr;

	ScummHelp::updateStrings(_scumm->_gameId, _scumm->_version, _page, titleStr, keyStr, dscStr);

	_title->setLabel(titleStr);
	for (int i = 0; i < HELP_NUM_LINES; i++) {
		_key[i]->setLabel(keyStr[i]);
		_dsc[i]->setLabel(dscStr[i]);
	}

	delete [] keyStr;
	delete [] dscStr;
}

void HelpDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {

	switch (cmd) {
	case kNextCmd:
		_page++;
		if (_page >= _numPages) {
			_nextButton->clearFlags(WIDGET_ENABLED);
		}
		if (_page >= 2) {
			_prevButton->setFlags(WIDGET_ENABLED);
		}
		displayKeyBindings();
		draw();
		break;
	case kPrevCmd:
		_page--;
		if (_page <= _numPages) {
			_nextButton->setFlags(WIDGET_ENABLED);
		}
		if (_page <= 1) {
			_prevButton->clearFlags(WIDGET_ENABLED);
		}
		displayKeyBindings();
		draw();
		break;
	default:
		ScummDialog::handleCommand(sender, cmd, data);
	}
}

#endif

#pragma mark -

InfoDialog::InfoDialog(ScummEngine *scumm, int res)
: ScummDialog(scumm, 0, 80, 0, 16) { // dummy x and w
	setInfoText(queryResString (res));
}

InfoDialog::InfoDialog(ScummEngine *scumm, const String& message)
: ScummDialog(scumm, 0, 80, 0, 16) { // dummy x and w
	setInfoText(message);
}

void InfoDialog::setInfoText(const String& message) {
	int width = g_gui.getStringWidth(message) + 16;

	_x = (_scumm->_screenWidth - width) >> 1;
	_w = width;

	new StaticTextWidget(this, 4, 4, _w - 8, _h, message, kTextAlignCenter);
}

#pragma mark -

PauseDialog::PauseDialog(ScummEngine *scumm)
	: InfoDialog(scumm, 10) {
}

ConfirmExitDialog::ConfirmExitDialog(ScummEngine *scumm)
	: InfoDialog(scumm, "Do you really want to quit (y/n)?") {
}

void ConfirmExitDialog::handleKeyDown(uint16 ascii, int keycode, int modifiers) {
	if (tolower(ascii) == 'n') { // Close exit dialog if n key is pressed
		setResult(0);
		close();
	} else if (tolower(ascii) == 'y') { // Quit if y key is pressed
		setResult(1);
		close();
	} else
		ScummDialog::handleKeyDown(ascii, keycode, modifiers);
}

#ifdef _WIN32_WCE

#pragma mark -

enum {
	kMapCmd					= 'map '
};


KeysDialog::KeysDialog(ScummEngine *scumm)
	: ScummDialog(scumm, 30, 20, 260, 160) {
	addButton(160, 20, "Map", kMapCmd, 'M');	// Map
	addButton(160, 40, "OK", kOKCmd, 'O');						// OK
	addButton(160, 60, "Cancel", kCancelCmd, 'C');				// Cancel

	_actionsList = new ListWidget(this, 10, 20, 140, 90);
	_actionsList->setNumberingMode(kListNumberingZero);

	_actionTitle = new StaticTextWidget(this, 10, 120, 240, 16, "Choose an action to map", kTextAlignCenter);
	_keyMapping = new StaticTextWidget(this, 10, 140, 240, 16, "", kTextAlignCenter);

	_actionTitle->setFlags(WIDGET_CLEARBG);
	_keyMapping->setFlags(WIDGET_CLEARBG);

	// Get actions names
	Common::StringList l;

	for (int i = 1; i < TOTAL_ACTIONS; i++) 
		l.push_back(getActionName(i));

	_actionsList->setList(l);

	_actionSelected = -1;
	_get_key_mapping = false;
}

void KeysDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch(cmd) {

	case kListSelectionChangedCmd:
		if (_actionsList->getSelected() >= 0) {
				char selection[100];

				sprintf(selection, "Associated key : %s", getGAPIKeyName((unsigned int)getAction(_actionsList->getSelected() + 1)->action_key));				
				_keyMapping->setLabel(selection);
				_keyMapping->draw();
		}
		break;
	case kMapCmd:
		if (_actionsList->getSelected() < 0) {
				_actionTitle->setLabel("Please select an action");
		}
		else {
				char selection[100];

				_actionSelected = _actionsList->getSelected() + 1;
				sprintf(selection, "Associated key : %s", getGAPIKeyName((unsigned int)getAction(_actionSelected)->action_key));				
				_actionTitle->setLabel("Press the key to associate");
				_keyMapping->setLabel(selection);
				_keyMapping->draw();
				_get_key_mapping = true;
				_actionsList->setEnabled(false);
		}
		_actionTitle->draw();
		break;
	case kOKCmd:
		save_key_mapping();
		close();
		break;
	case kCancelCmd:
		load_key_mapping();
		close();
		break;
	default:
		ScummDialog::handleCommand(sender, cmd, data);
	}
}

void KeysDialog::handleKeyDown(uint16 ascii, int keycode, int modifiers) {
	if (modifiers == 0xff  && _get_key_mapping) {
		// GAPI key was selected
		char selection[100];

		clearActionKey(ascii & 0xff);
		getAction(_actionSelected)->action_key = (ascii & 0xff);
		sprintf(selection, "Associated key : %s", getGAPIKeyName((unsigned int)getAction(_actionSelected)->action_key));				
		_actionTitle->setLabel("Choose an action to map");
		_keyMapping->setLabel(selection);
		_keyMapping->draw();
		_actionSelected = -1;
		_actionsList->setEnabled(true);
		_get_key_mapping = false;
	}
}

#endif

} // End of namespace Scumm

#ifdef __PALM_OS__
#include "scumm_globals.h"

_GINIT(Dialogs)
_GSETPTR(Scumm::string_map_table_v7, GBVARS_STRINGMAPTABLEV7_INDEX, Scumm::ResString, GBVARS_SCUMM)
_GSETPTR(Scumm::string_map_table_v6, GBVARS_STRINGMAPTABLEV6_INDEX, Scumm::ResString, GBVARS_SCUMM)
_GSETPTR(Scumm::string_map_table_v5, GBVARS_STRINGMAPTABLEV5_INDEX, Scumm::ResString, GBVARS_SCUMM)
_GEND

_GRELEASE(Dialogs)
_GRELEASEPTR(GBVARS_STRINGMAPTABLEV7_INDEX, GBVARS_SCUMM)
_GRELEASEPTR(GBVARS_STRINGMAPTABLEV6_INDEX, GBVARS_SCUMM)
_GRELEASEPTR(GBVARS_STRINGMAPTABLEV5_INDEX, GBVARS_SCUMM)
_GEND

#endif
