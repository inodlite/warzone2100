/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*
 * FrontEnd.c
 *
 * front end title and options screens.
 * Alex Lee. Pumpkin Studios. Eidos PLC 98,
 */

#include "lib/framework/wzapp.h"

#if defined(WZ_OS_WIN)
#  include <shellapi.h> /* For ShellExecute  */
#endif

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/sound/mixer.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/slider.h"

#include <imgui/imgui.h>

#include "advvis.h"
#include "challenge.h"
#include "component.h"
#include "configuration.h"
#include "difficulty.h"
#include "display.h"
#include "droid.h"
#include "frend.h"
#include "frontend.h"
#include "group.h"
#include "hci.h"
#include "init.h"
#include "levels.h"
#include "intdisplay.h"
#include "keyedit.h"
#include "loadsave.h"
#include "main.h"
#include "modding.h"
#include "multiint.h"
#include "multilimit.h"
#include "multiplay.h"
#include "radar.h"
#include "seqdisp.h"
#include "texture.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"

#include <functional>

struct CAMPAIGN_FILE
{
	QString name;
	QString level;
	QString video;
	QString captions;
	QString package;
	QString loading;
};

// ////////////////////////////////////////////////////////////////////////////
// Global variables

tMode titleMode; // the global case
tMode lastTitleMode; // Since skirmish and multiplayer setup use the same functions, we use this to go back to the corresponding menu.

char			aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.

bool			bLimiterLoaded = false;

#define TUTORIAL_LEVEL "TUTORIAL3"

// ////////////////////////////////////////////////////////////////////////////
// Forward definitions

static void addSmallTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, unsigned int style);

static void doAudioOptionsMenu();
static void doGameOptionsMenu();
static void doGraphicsOptionsMenu();
static void doMouseOptionsMenu();
static void doVideoOptionsMenu();

// ////////////////////////////////////////////////////////////////////////////
// Helper functions

namespace ImGui {
	namespace Wz {
		const ImVec2 useFullWidth(-1, 0);

		ImGuiWindowFlags titleWindowFlags = ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

		bool ButtonFW(const char* label)
		{
			return ImGui::Button(label, useFullWidth);
		}

		void Image(const ImageDef *image, const ImVec2& size)
		{
			ImGui::Image(reinterpret_cast<ImTextureID>(pie_Texture(image->textureId)),
				     size,
				     ImVec2((image->XOffset + image->Tu) * image->invTextureSize,
					    (image->YOffset + image->Tv) * image->invTextureSize),
				     ImVec2((image->XOffset + image->Width + image->Tu) * image->invTextureSize,
					    (image->YOffset + image->Height + image->Tv) * image->invTextureSize));
		}

		void Image(const char* tex_name, const ImVec2& size)
		{
			ImageDef *image = iV_GetImage(QString(tex_name));
			Image(image, size);
		}

		void ImageFE(const int fe_img_id, const ImVec2& size)
		{
			ImageDef *image = &FrontImages->imageDefs[fe_img_id];
			Image(image, size);
		}

		bool ImageButton(const ImageDef *image, const ImVec2& size)
		{
			return ImGui::ImageButton(reinterpret_cast<ImTextureID>(pie_Texture(image->textureId)),
					   size,
					   ImVec2((image->XOffset + image->Tu) * image->invTextureSize,
						  (image->YOffset + image->Tv) * image->invTextureSize),
					   ImVec2((image->XOffset + image->Tu + image->Width) * image->invTextureSize,
						  (image->YOffset + image->Tv + image->Height) * image->invTextureSize));
		}

		bool ImageButton(const char* tex_name, const ImVec2& size)
		{
			ImageDef *image = iV_GetImage(QString(tex_name));
			return ImageButton(image, size);
		}

		bool ImageButtonFE(const int fe_img_id, const ImVec2& size)
		{
			ImageDef *image = &FrontImages->imageDefs[fe_img_id];
			return ImageButton(image, size);
		}

		bool ColorButtonFE(const char* label, const ImVec4& col)
		{
			return ImGui::ColorButton(label, col,
						  ImGuiColorEditFlags_NoAlpha |
						  ImGuiColorEditFlags_NoPicker |
						  ImGuiColorEditFlags_NoTooltip,
						  ImVec2(20,20));
		}

		void LogoForm()
		{
			int frmW = (titleMode == MULTIOPTION) ? FRONTEND_TOPFORM_WIDEW : FRONTEND_TOPFORMW;
			int frmH = (titleMode == MULTIOPTION) ? FRONTEND_TOPFORM_WIDEH : FRONTEND_TOPFORMH;

			ImGuiIO& io = ImGui::GetIO();

			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - (HIDDEN_FRONTEND_WIDTH * 0.5f) +
						       ((titleMode == MULTIOPTION) ? FRONTEND_TOPFORM_WIDEX : FRONTEND_TOPFORMX),
						       io.DisplaySize.y * 0.5f - (HIDDEN_FRONTEND_HEIGHT * 0.5f) +
						       ((titleMode == MULTIOPTION) ? FRONTEND_TOPFORM_WIDEY : FRONTEND_TOPFORMY)));
			ImGui::SetNextWindowSize(ImVec2(frmW, frmH));

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

			ImGui::Begin("TitleMenuTop", nullptr, ImGui::Wz::titleWindowFlags | ImGuiWindowFlags_NoScrollbar);
			{
				int imgW = iV_GetImageWidth(FrontImages, IMAGE_FE_LOGO);
				int imgH = iV_GetImageHeight(FrontImages, IMAGE_FE_LOGO);

				int dstW = frmW;
				int dstH = frmH;
				if (imgW * dstH < imgH * dstW) // Want to set aspect ratio dstW/dstH = imgW/imgH.
					dstW = imgW * dstH / imgH; // Too wide.
				else if (imgW * dstH > imgH * dstW)
					dstH = imgH * dstW / imgW; // Too high.

				ImGui::Dummy(ImVec2((frmW - dstW) * 0.5f, 0));
				ImGui::SameLine();
				ImGui::Wz::ImageFE(IMAGE_FE_LOGO, ImVec2(dstW, dstH));
			}

			ImGui::PopStyleVar(2);

			if (ImGui::IsItemHovered())
			{
				static std::string mods_str = getModList();

				ImGui::BeginTooltip();
				ImGui::Text("%s", version_getFormattedVersionString());
				if (!mods_str.empty())
					ImGui::Text("%s %s", _("Mod: "), mods_str.c_str());
				ImGui::EndTooltip();
			}

			ImGui::End();
		}

		void TitleForm(const char* form_title, const std::function< void() >& top_fn,
			       const std::function< void() >& main_fn, const std::function< void() >& bottom_fn)
		{
			ImGuiIO& io = ImGui::GetIO();

			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - (HIDDEN_FRONTEND_WIDTH * 0.5f) + FRONTEND_BOTFORMX,
						       io.DisplaySize.y * 0.5f - (HIDDEN_FRONTEND_HEIGHT * 0.5f) + FRONTEND_BOTFORMY));
			ImGui::SetNextWindowSize(ImVec2(FRONTEND_BOTFORMW, FRONTEND_BOTFORMH));
			ImGui::Begin("TitleMenuBottom", nullptr, ImGui::Wz::titleWindowFlags);

			ImVec2 windowSize = ImGui::GetWindowSize();

			// Use 3 columns with no borders and make middle one wide enough
			ImGui::Columns(3, "TitleMenuColumns", false);
			ImGui::SetColumnWidth(0, windowSize.x * 0.2f);
			ImGui::SetColumnWidth(1, windowSize.x * 0.6f);
			ImGui::SetColumnWidth(2, windowSize.x * 0.2f);

			// do top stuff
			if (top_fn)
				top_fn();

			ImGui::NextColumn();

			ImGui::PushFont(fontBig);

			ImGui::Text("%s", form_title);

			// do main stuff
			main_fn();

			ImGui::PopFont();

			ImGui::Columns();

			// do bottom stuff
			if (bottom_fn)
				bottom_fn();

			ImGui::End();

		}

		template<tMode MODE> void TitleFormSimpleReturnFn()
		{
		       if (ImGui::Wz::ImageButtonFE(IMAGE_RETURN, ImVec2(MULTIOP_RETW, MULTIOP_RETH)))
			       changeTitleMode(MODE);
		       if (ImGui::IsItemHovered())
		       {
			       ImGui::BeginTooltip();
			       ImGui::Text("%s", P_("menu", "Return"));
			       ImGui::EndTooltip();
		       }
		};
	}
}

// Returns true if escape key pressed.
//
bool CancelPressed(void)
{
	const bool cancel = keyPressed(KEY_ESC);
	if (cancel)
	{
		inputLoseFocus();	// clear the input buffer.
	}
	return cancel;
}

// Cycle through options, one at a time.
template <typename T>
static T stepCycleDir(bool forwards, T value, T min, T max)
{
	return forwards ?
	       value < max ? T(value + 1) : min :  // Cycle forwards. The T cast is to cycle through enums.
	       min < value ? T(value - 1) : max;  // Cycle backwards.
}

// Cycle through options, one at a time.
template <typename T>
static T stepCycle(T value, T min, T max)
{
	return stepCycleDir<T>(!mouseReleased(MOUSE_RMB), value, min , max);
}

// Cycle through options, which are powers of two, such as [128, 256, 512, 1024, 2048].
template <typename T>
static T pow2CycleDir(bool forwards, T value, T min, T max)
{
	return forwards ?
	       value < max ? std::max<T>(1, value) * 2 : min :  // Cycle forwards.
	       min < value ? (value / 2 > 1 ? value / 2 : 0) : max;  // Cycle backwards.
}

// Cycle through options, which are powers of two, such as [128, 256, 512, 1024, 2048].
template <typename T>
static T pow2Cycle(T value, T min, T max)
{
	return pow2CycleDir<T>(!mouseReleased(MOUSE_RMB), value, min , max);
}

// ////////////////////////////////////////////////////////////////////////////
// Title Screen
static bool startTitleMenu(void)
{
	intRemoveReticule();

	addBackdrop();

	addTopForm();
	addBottomForm();

	addTextButton(FRONTEND_SINGLEPLAYER, FRONTEND_POS2X, FRONTEND_POS2Y, _("Single Player"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MULTIPLAYER, FRONTEND_POS3X, FRONTEND_POS3Y, _("Multi Player"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS4X, FRONTEND_POS4Y, _("Tutorial"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_OPTIONS, FRONTEND_POS5X, FRONTEND_POS5Y, _("Options"), WBUT_TXTCENTRE);
	if (PHYSFS_exists("sequences/devastation.ogg"))
	{
		addTextButton(FRONTEND_PLAYINTRO, FRONTEND_POS6X, FRONTEND_POS6Y, _("View Intro"), WBUT_TXTCENTRE);
	}
	else
	{
		addTextButton(FRONTEND_PLAYINTRO, FRONTEND_POS6X, FRONTEND_POS6Y, _("View Intro"), WBUT_TXTCENTRE | WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_PLAYINTRO, _("Videos are missing, download them from http://wz2100.net"));
	}
	addTextButton(FRONTEND_QUIT, FRONTEND_POS7X, FRONTEND_POS7Y, _("Quit Game"), WBUT_TXTCENTRE);
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("MAIN MENU"));

	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS8X, FRONTEND_POS8Y, _("Official site: http://wz2100.net/"), 0);
	widgSetTip(psWScreen, FRONTEND_HYPERLINK, _("Come visit the forums and all Warzone 2100 news! Click this link."));
	addSmallTextButton(FRONTEND_DONATELINK, FRONTEND_POS8X + 360, FRONTEND_POS8Y, _("Donate: http://donations.wz2100.net/"), 0);
	widgSetTip(psWScreen, FRONTEND_DONATELINK, _("Help support the project with our server costs, Click this link."));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_UPGRDLINK, 7, 7, MULTIOP_BUTW, MULTIOP_BUTH, _("Check for a newer version"), IMAGE_GAMEVERSION, IMAGE_GAMEVERSION_HI, true);

	return true;
}

static void runUpgrdHyperlink(void)
{
	//FIXME: There is no decent way we can re-init the display to switch to window or fullscreen within game. refs: screenToggleMode().
	std::string link = "http://gamecheck.wz2100.net/";
	std::string version = version_getVersionString();
	for (char ch : version)
	{
		link += ch == ' '? '_' : ch;
	}

#if defined(WZ_OS_WIN)
	wchar_t  wszDest[250] = {'/0'};
	MultiByteToWideChar(CP_UTF8, 0, link.c_str(), -1, wszDest, 250);

	ShellExecuteW(NULL, L"open", wszDest, NULL, NULL, SW_SHOWNORMAL);
#elif defined (WZ_OS_MAC)
	char lbuf[250] = {'\0'};
	ssprintf(lbuf, "open %s &", link.c_str());
	system(lbuf);
#else
	// for linux
	char lbuf[250] = {'\0'};
	ssprintf(lbuf, "xdg-open %s &", link.c_str());
	int stupidWarning = system(lbuf);
	(void)stupidWarning;  // Why is system() a warn_unused_result function..?
#endif
}

static void runHyperlink(void)
{
#if defined(WZ_OS_WIN)
	ShellExecuteW(NULL, L"open", L"http://wz2100.net/", NULL, NULL, SW_SHOWNORMAL);
#elif defined (WZ_OS_MAC)
	// For the macs
	system("open http://wz2100.net");
#else
	// for linux
	int stupidWarning = system("xdg-open http://wz2100.net &");
	(void)stupidWarning;  // Why is system() a warn_unused_result function..?
#endif
}

static void rundonatelink(void)
{
#if defined(WZ_OS_WIN)
	ShellExecuteW(NULL, L"open", L"http://donations.wz2100.net/", NULL, NULL, SW_SHOWNORMAL);
#elif defined (WZ_OS_MAC)
	// For the macs
	system("open http://donations.wz2100.net");
#else
	// for linux
	int stupidWarning = system("xdg-open http://donations.wz2100.net &");
	(void)stupidWarning;  // Why is system() a warn_unused_result function..?
#endif
}

bool runTitleMenu(void)
{
 if (use_wzwidgets) {
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_QUIT:
		changeTitleMode(CREDITS);
		break;
	case FRONTEND_MULTIPLAYER:
		changeTitleMode(MULTI);
		break;
	case FRONTEND_SINGLEPLAYER:
		changeTitleMode(SINGLE);
		break;
	case FRONTEND_OPTIONS:
		changeTitleMode(OPTIONS);
		break;
	case FRONTEND_TUTORIAL:
		changeTitleMode(TUTORIAL);
		break;
	case FRONTEND_PLAYINTRO:
		changeTitleMode(SHOWINTRO);
		break;
	case FRONTEND_HYPERLINK:
		runHyperlink();
		break;
	case FRONTEND_UPGRDLINK:
		runUpgrdHyperlink();
		break;
	case FRONTEND_DONATELINK:
		rundonatelink();
		break;

	default:
		break;
	}

	widgDisplayScreen(psWScreen); // show the widgets currently running
 }
 else
 {
	bool wantsToQuit = false;
	static bool hasIntroData = PHYSFS_exists("sequences/devastation.ogg");

	// Top form
	ImGui::Wz::LogoForm();

	// Bottom form

	static auto top_fn = [] ()
	{
		if (ImGui::Wz::ImageButtonFE(IMAGE_GAMEVERSION,
					     ImVec2(MULTIOP_BUTW * (MULTIOP_RETH / float(MULTIOP_BUTH)), MULTIOP_RETH)))
			runUpgrdHyperlink();
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text(_("Check for a newer version"));
			ImGui::EndTooltip();
		}
	};

	static auto middle_fn = [&wantsToQuit] ()
	{
		if (ImGui::Wz::ButtonFW(_("Single Player")))
			changeTitleMode(SINGLE);
		if (ImGui::Wz::ButtonFW(_("Multi Player")))
			changeTitleMode(MULTI);

		if (ImGui::Wz::ButtonFW(_("Tutorial")))
			changeTitleMode(TUTORIAL);
		if (ImGui::Wz::ButtonFW(_("Options")))
			changeTitleMode(OPTIONS);

		if (ImGui::Wz::ButtonFW(_("View Intro")) && hasIntroData)
			changeTitleMode(SHOWINTRO);
		if (ImGui::IsItemHovered() && !hasIntroData)
		{
			ImGui::PopFont();
			ImGui::BeginTooltip();
			ImGui::Text(_("Videos are missing, download them from http://wz2100.net"));
			ImGui::EndTooltip();
			ImGui::PushFont(fontBig);
		}

		if (ImGui::Wz::ButtonFW(_("Quit Game")))
			wantsToQuit = true;
	};

	static auto bottom_fn = [] ()
	{
		ImGui::Dummy(ImVec2(1, ImGui::GetWindowHeight() * 0.025f));

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.88f, 0.3f, 1.0f));
		if (ImGui::SmallButton(_("Official site: http://wz2100.net/")))
			runHyperlink();
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text(_("Come visit the forums and all Warzone 2100 news! Click this link."));
			ImGui::EndTooltip();
		}

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.88f, 0.3f, 1.0f));
		if (ImGui::SmallButton(_("Donate: http://donations.wz2100.net/")))
			rundonatelink();
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text(_("Help support the project with our server costs, Click this link."));
			ImGui::EndTooltip();
		}
	};

	ImGui::Wz::TitleForm(_("MAIN MENU"), top_fn, middle_fn, bottom_fn);

	if (wantsToQuit)
		changeTitleMode(CREDITS);
  }

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Tutorial Menu
static bool startTutorialMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();


	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS3X, FRONTEND_POS3Y, _("Tutorial"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_FASTPLAY, FRONTEND_POS4X, FRONTEND_POS4Y, _("Fast Play"), WBUT_TXTCENTRE);
	addSideText(FRONTEND_SIDETEXT , FRONTEND_SIDEX, FRONTEND_SIDEY, _("TUTORIALS"));
	// TRANSLATORS: "Return", in this context, means "return to previous screen/menu"
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	return true;
}

bool runTutorialMenu(void)
{
 if (use_wzwidgets)
 {
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_TUTORIAL:
		NetPlay.players[0].allocated = true;
		game.skDiff[0] = UBYTE_MAX;
		sstrcpy(aLevelName, TUTORIAL_LEVEL);
		changeTitleMode(STARTGAME);
		break;

	case FRONTEND_FASTPLAY:
		NETinit(true);
		NetPlay.players[0].allocated = true;
		game.skDiff[0] = UBYTE_MAX;
		sstrcpy(aLevelName, "FASTPLAY");
		changeTitleMode(STARTGAME);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(TITLE);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running
 }
 else
 {
	// Top form
	ImGui::Wz::LogoForm();

	// Bottom form
	static auto middle_fn = [] ()
	{
		if (ImGui::Wz::ButtonFW(_("Tutorial")))
		{
			NetPlay.players[0].allocated = true;
			game.skDiff[0] = UBYTE_MAX;
			sstrcpy(aLevelName, TUTORIAL_LEVEL);
			changeTitleMode(STARTGAME);
		}
		if (ImGui::Wz::ButtonFW(_("Fast Play")))
		{
			NETinit(true);
			NetPlay.players[0].allocated = true;
			game.skDiff[0] = UBYTE_MAX;
			sstrcpy(aLevelName, "FASTPLAY");
			changeTitleMode(STARTGAME);
		}
	};

	ImGui::Wz::TitleForm(_("TUTORIALS"), ImGui::Wz::TitleFormSimpleReturnFn<TITLE>,
			     middle_fn, nullptr);
  }
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Single Player Menu
static void startSinglePlayerMenu(void)
{
	challengeActive = false;
	addBackdrop();
	addTopForm();
	addBottomForm();

	addTextButton(FRONTEND_NEWGAME,  FRONTEND_POS2X, FRONTEND_POS2Y, _("New Campaign") , WBUT_TXTCENTRE);
	addTextButton(FRONTEND_SKIRMISH, FRONTEND_POS3X, FRONTEND_POS3Y, _("Start Skirmish Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_CHALLENGES, FRONTEND_POS4X, FRONTEND_POS4Y, _("Challenges"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_LOADGAME_MISSION, FRONTEND_POS5X, FRONTEND_POS5Y, _("Load Campaign Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_LOADGAME_SKIRMISH, FRONTEND_POS6X, FRONTEND_POS6Y, _("Load Skirmish Game"), WBUT_TXTCENTRE);

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("SINGLE PLAYER"));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	// show this only when the video sequences are not installed
	if (!PHYSFS_exists("sequences/devastation.ogg"))
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS8X, FRONTEND_POS8Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
	}
}

static QList<CAMPAIGN_FILE> readCampaignFiles()
{
	QList<CAMPAIGN_FILE> result;
	char **files = PHYSFS_enumerateFiles("campaigns");
	for (char **i = files; *i != NULL; ++i)
	{
		CAMPAIGN_FILE c;
		QString filename("campaigns/");
		filename += *i;
		if (!filename.endsWith(".json"))
		{
			continue;
		}
		WzConfig ini(filename, WzConfig::ReadOnlyAndRequired);
		c.name = ini.value("name").toString();
		c.level = ini.value("level").toString();
		c.package = ini.value("package").toString();
		c.loading = ini.value("loading").toString();
		c.video = ini.value("video").toString();
		c.captions = ini.value("captions").toString();
		result += c;
	}
	PHYSFS_freeList(files);
	return result;
}

static void startCampaignSelector()
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	QList<CAMPAIGN_FILE> list = readCampaignFiles();
	for (int i = 0; i < list.size(); i++)
	{
		addTextButton(FRONTEND_CAMPAIGN_1 + i, FRONTEND_POS1X, FRONTEND_POS2Y + 40 * i, gettext(list[i].name.toUtf8().constData()), WBUT_TXTCENTRE);
	}
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("CAMPAIGNS"));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	// show this only when the video sequences are not installed
	if (!PHYSFS_exists("sequences/devastation.ogg"))
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS8X, FRONTEND_POS8Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
	}
}

static void frontEndNewGame(int which)
{
	QList<CAMPAIGN_FILE> list = readCampaignFiles();
	sstrcpy(aLevelName, list[which].level.toUtf8().constData());
	if (!list[which].video.isEmpty())
	{
		seq_ClearSeqList();
		seq_AddSeqToList(list[which].video.toUtf8().constData(), NULL, list[which].captions.toUtf8().constData(), false);
		seq_StartNextFullScreenVideo();
	}
	if (!list[which].package.isEmpty())
	{
		QString path;
		path += PHYSFS_getWriteDir();
		path += PHYSFS_getDirSeparator();
		path += "campaigns";
		path += PHYSFS_getDirSeparator();
		path += list[which].package;
		if (!PHYSFS_mount(path.toUtf8().constData(), NULL, PHYSFS_APPEND))
		{
			debug(LOG_ERROR, "Failed to load campaign mod \"%s\": %s",
			      path.toUtf8().constData(), PHYSFS_getLastError());
		}
	}
	if (!list[which].loading.isEmpty())
	{
		debug(LOG_WZ, "Adding campaign mod level \"%s\"", list[which].loading.toUtf8().constData());
		if (!loadLevFile(list[which].loading.toUtf8().constData(), mod_campaign, false, NULL))
		{
			debug(LOG_ERROR, "Failed to load %s", list[which].loading.toUtf8().constData());
			return;
		}
	}
	debug(LOG_WZ, "Loading campaign mod -- %s", aLevelName);
	changeTitleMode(STARTGAME);
}

static void loadOK(void)
{
	if (strlen(sRequestResult))
	{
		sstrcpy(saveGameName, sRequestResult);
		changeTitleMode(LOADSAVEGAME);
	}
}

void SPinit()
{
	uint8_t playercolor;

	// clear out the skDiff array
	memset(game.skDiff, 0x0, sizeof(game.skDiff));
	NetPlay.bComms = false;
	bMultiPlayer = false;
	bMultiMessages = false;
	game.type = CAMPAIGN;
	NET_InitPlayers();
	NetPlay.players[0].allocated = true;
	NetPlay.players[0].autoGame = false;
	game.skDiff[0] = UBYTE_MAX;
	game.maxPlayers = MAX_PLAYERS -1;	// Currently, 0 - 10 for a total of MAX_PLAYERS
	// make sure we have a valid color choice for our SP game. Valid values are 0, 4-7
	playercolor = war_GetSPcolor();

	if (playercolor >= 1 && playercolor <= 3)
	{
		playercolor = 0;	// default is green
	}
	setPlayerColour(0, playercolor);
	game.hash.setZero();	// must reset this to zero
}

bool runCampaignSelector()
{
if (use_wzwidgets)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.
	if (id == FRONTEND_QUIT)
	{
		changeTitleMode(SINGLE); // go back
	}
	else if (id >= FRONTEND_CAMPAIGN_1 && id <= FRONTEND_CAMPAIGN_6) // chose a campaign
	{
		SPinit();
		frontEndNewGame(id - FRONTEND_CAMPAIGN_1);
	}
	widgDisplayScreen(psWScreen); // show the widgets currently running
}
else
{
	static QList<CAMPAIGN_FILE> list = readCampaignFiles();
	static int campaignSelection = -1;

	// Top form
	ImGui::Wz::LogoForm();

	// Bottom form
	static auto middle_fn = [] ()
	{
		campaignSelection = -1;
		for (int i = 0; i < list.size(); i++)
		{
			if (ImGui::Wz::ButtonFW(gettext(list[i].name.toUtf8().constData())))
				campaignSelection = i;
		}
	};

	ImGui::Wz::TitleForm(_("CAMPAIGNS"), ImGui::Wz::TitleFormSimpleReturnFn<SINGLE>,
			     middle_fn, nullptr);

	if (campaignSelection >= 0)
	{
		SPinit();
		frontEndNewGame(campaignSelection);
	}

}
	return true;
}

bool runSinglePlayerMenu(void)
{
	if (bLoadSaveUp)
	{
		if (runLoadSave(false)) // check for file name.
		{
			loadOK();
		}
	}
	else if (challengesUp)
	{
		runChallenges();
	}
	else
	{
	  if (use_wzwidgets)
	  {
		WidgetTriggers const &triggers = widgRunScreen(psWScreen);
		unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

		switch (id)
		{
		case FRONTEND_NEWGAME:
			changeTitleMode(CAMPAIGNS);
			break;

		case FRONTEND_LOADGAME_MISSION:
			SPinit();
			addLoadSave(LOAD_FRONTEND_MISSION, _("Load Campaign Saved Game"));	// change mode when loadsave returns
			break;

		case FRONTEND_LOADGAME_SKIRMISH:
			SPinit();
			bMultiPlayer = true;
			addLoadSave(LOAD_FRONTEND_SKIRMISH, _("Load Skirmish Saved Game"));	// change mode when loadsave returns
			break;

		case FRONTEND_SKIRMISH:
			SPinit();
			ingame.bHostSetup = true;
			lastTitleMode = SINGLE;
			changeTitleMode(MULTIOPTION);
			break;

		case FRONTEND_QUIT:
			changeTitleMode(TITLE);
			break;

		case FRONTEND_CHALLENGES:
			SPinit();
			addChallenges();
			break;

		default:
			break;
		}

		if (CancelPressed())
		{
			changeTitleMode(TITLE);
		}
	  }
	  else
	  {
	       bool wantsToQuit = false;
	       // Top form
	       ImGui::Wz::LogoForm();

	       // Bottom form
	       static auto middle_fn = [] ()
	       {
		       if (ImGui::Wz::ButtonFW(_("New Campaign")))
			       changeTitleMode(CAMPAIGNS);
		       if (ImGui::Wz::ButtonFW(_("Start Skirmish Game")))
		       {
			       SPinit();
			       ingame.bHostSetup = true;
			       lastTitleMode = SINGLE;
			       changeTitleMode(MULTIOPTION);
		       }

		       if (ImGui::Wz::ButtonFW(_("Challenges")))
		       {
			       SPinit();
			       addChallenges();
		       }

		       if (ImGui::Wz::ButtonFW(_("Load Campaign Game")))
		       {
			       SPinit();
			       addLoadSave(LOAD_FRONTEND_MISSION, _("Load Campaign Saved Game"));	// change mode when loadsave returns
		       }
		       if (ImGui::Wz::ButtonFW(_("Load Skirmish Game")))
		       {
			       SPinit();
			       bMultiPlayer = true;
			       addLoadSave(LOAD_FRONTEND_SKIRMISH, _("Load Skirmish Saved Game"));	// change mode when loadsave returns
		       }
	       };

	       static auto bottom_fn = [] ()
	       {
		       static bool hasIntroData = PHYSFS_exists("sequences/devastation.ogg");

		       // show this only when the video sequences are not installed
		       if (!hasIntroData)
		       {
			       ImGui::Dummy(ImVec2(1, ImGui::GetWindowHeight() * 0.2f));

			       ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.88f, 0.3f, 1.0f));
			       ImGui::SmallButton(_("Campaign videos are missing! Get them from http://wz2100.net"));
			       ImGui::PopStyleColor();
		       }
	       };

	       ImGui::Wz::TitleForm(_("SINGLE PLAYER"), ImGui::Wz::TitleFormSimpleReturnFn<TITLE>,
				    middle_fn, bottom_fn);

	       if (wantsToQuit)
		       changeTitleMode(CREDITS);
	 }
	}

	if (!bLoadSaveUp && !challengesUp)						// if save/load screen is up
	{
	  if (use_wzwidgets)
		widgDisplayScreen(psWScreen);						// show the widgets currently running
	}
	if (bLoadSaveUp)								// if save/load screen is up
	{
		displayLoadSave();
	}
	else if (challengesUp)
	{
		displayChallenges();
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Multi Player Menu
static bool startMultiPlayerMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	addSideText(FRONTEND_SIDETEXT ,	FRONTEND_SIDEX, FRONTEND_SIDEY, _("MULTI PLAYER"));

	addTextButton(FRONTEND_HOST,     FRONTEND_POS2X, FRONTEND_POS2Y, _("Host Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_JOIN,     FRONTEND_POS3X, FRONTEND_POS3Y, _("Join Game"), WBUT_TXTCENTRE);

	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	// This isn't really a hyperlink for now... perhaps link to the wiki ?
	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS8X, FRONTEND_POS8Y, _("TCP port 2100 must be opened in your firewall or router to host games!"), 0);

	return true;
}

bool runMultiPlayerMenu(void)
{
if (use_wzwidgets)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_HOST:
		// don't pretend we are running a network game. Really do it!
		NetPlay.bComms = true; // use network = true
		NetPlay.isUPNP_CONFIGURED = false;
		NetPlay.isUPNP_ERROR = false;
		ingame.bHostSetup = true;
		bMultiPlayer = true;
		bMultiMessages = true;
		NETinit(true);
		NETdiscoverUPnPDevices();
		game.type = SKIRMISH;		// needed?
		lastTitleMode = MULTI;
		changeTitleMode(MULTIOPTION);
		break;
	case FRONTEND_JOIN:
		NETinit(true);
		ingame.bHostSetup = false;
		if (getLobbyError() != ERROR_INVALID)
		{
			setLobbyError(ERROR_NOERROR);
		}
		changeTitleMode(PROTOCOL);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;
	default:
		break;
	}

	widgDisplayScreen(psWScreen); // show the widgets currently running

	if (CancelPressed())
	{
		changeTitleMode(TITLE);
	}
}
else
{
	// Top form
	ImGui::Wz::LogoForm();

	// Bottom form
	static auto middle_fn = [] ()
	{
		if (ImGui::Wz::ButtonFW(_("Host Game")))
		{
			// don't pretend we are running a network game. Really do it!
			NetPlay.bComms = true; // use network = true
			NetPlay.isUPNP_CONFIGURED = false;
			NetPlay.isUPNP_ERROR = false;
			ingame.bHostSetup = true;
			bMultiPlayer = true;
			bMultiMessages = true;
			NETinit(true);
			NETdiscoverUPnPDevices();
			game.type = SKIRMISH;		// needed?
			lastTitleMode = MULTI;
			changeTitleMode(MULTIOPTION);
		}
		if (ImGui::Wz::ButtonFW(_("Join Game")))
		{
			NETinit(true);
			ingame.bHostSetup = false;
			if (getLobbyError() != ERROR_INVALID)
			{
				setLobbyError(ERROR_NOERROR);
			}
			changeTitleMode(PROTOCOL);
		}
	};

	ImGui::Wz::TitleForm(_("MULTI PLAYER"), ImGui::Wz::TitleFormSimpleReturnFn<TITLE>,
			     middle_fn, nullptr);
}
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Options Menu
static bool startOptionsMenu(void)
{
	sliderEnableDrag(true);

	addBackdrop();
	addTopForm();
	addBottomForm();

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("OPTIONS"));
	addTextButton(FRONTEND_GAMEOPTIONS,	FRONTEND_POS2X, FRONTEND_POS2Y, _("Game Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_GRAPHICSOPTIONS, FRONTEND_POS3X, FRONTEND_POS3Y, _("Graphics Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_VIDEOOPTIONS, FRONTEND_POS4X, FRONTEND_POS4Y, _("Video Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_AUDIOOPTIONS, FRONTEND_POS5X, FRONTEND_POS5Y, _("Audio Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MOUSEOPTIONS, FRONTEND_POS6X, FRONTEND_POS6Y, _("Mouse Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_KEYMAP,		FRONTEND_POS7X, FRONTEND_POS7Y, _("Key Mappings"), WBUT_TXTCENTRE);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	return true;
}

bool runOptionsMenu(void)
{
if (use_wzwidgets)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_GAMEOPTIONS:
		changeTitleMode(GAME);
		break;
	case FRONTEND_GRAPHICSOPTIONS:
		changeTitleMode(GRAPHICS_OPTIONS);
		break;
	case FRONTEND_AUDIOOPTIONS:
		changeTitleMode(AUDIO_OPTIONS);
		break;
	case FRONTEND_VIDEOOPTIONS:
		changeTitleMode(VIDEO_OPTIONS);
		break;
	case FRONTEND_MOUSEOPTIONS:
		changeTitleMode(MOUSE_OPTIONS);
		break;
	case FRONTEND_KEYMAP:
		changeTitleMode(KEYMAP);
		break;
	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;
	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(TITLE);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running
}
else
{
	// Top form
	ImGui::Wz::LogoForm();

	// Bottom form
	static auto middle_fn = [] ()
	{
	};

	static auto bottom_fn = [] ()
	{
		if (ImGui::BeginTabBar("OptsTabBar", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem(_("Game")))
			{
				doGameOptionsMenu();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(_("Graphics")))
			{
				doGraphicsOptionsMenu();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(_("Video")))
			{
				doVideoOptionsMenu();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(_("Audio")))
			{
				doAudioOptionsMenu();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(_("Mouse")))
			{
				doMouseOptionsMenu();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	};

	ImGui::Wz::TitleForm(_("OPTIONS"), ImGui::Wz::TitleFormSimpleReturnFn<TITLE>,
			     middle_fn, bottom_fn);
}
	return true;
}

static char const *graphicsOptionsFmvmodeString()
{
	switch (war_GetFMVmode())
	{
	case FMV_1X: return _("1×");
	case FMV_2X: return _("2×");
	case FMV_FULLSCREEN: return _("Fullscreen");
	default: return _("Unsupported");
	}
}

static char const *graphicsOptionsScanlinesString()
{
	switch (war_getScanlineMode())
	{
	case SCANLINES_OFF: return _("Off");
	case SCANLINES_50: return _("50%");
	case SCANLINES_BLACK: return _("Black");
	default: return _("Unsupported");
	}
}

static char const *graphicsOptionsSubtitlesString()
{
	return seq_GetSubtitles() ? _("On") : _("Off");
}

static char const *graphicsOptionsShadowsString()
{
	return getDrawShadows() ? _("On") : _("Off");
}

static char const *graphicsOptionsRadarString()
{
	return rotateRadar ? _("Rotating") : _("Fixed");
}

// ////////////////////////////////////////////////////////////////////////////
// Graphics Options
static bool startGraphicsOptionsMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	////////////
	//FMV mode.
	addTextButton(FRONTEND_FMVMODE,   FRONTEND_POS2X - 35, FRONTEND_POS2Y, _("Video Playback"), WBUT_SECONDARY);
	addTextButton(FRONTEND_FMVMODE_R, FRONTEND_POS2M - 55, FRONTEND_POS2Y, graphicsOptionsFmvmodeString(), WBUT_SECONDARY);

	// Scanlines
	addTextButton(FRONTEND_SCANLINES,   FRONTEND_POS3X - 35, FRONTEND_POS3Y, _("Scanlines"), WBUT_SECONDARY);
	addTextButton(FRONTEND_SCANLINES_R, FRONTEND_POS3M - 55, FRONTEND_POS3Y, graphicsOptionsScanlinesString(), WBUT_SECONDARY);

	////////////
	//subtitle mode.
	addTextButton(FRONTEND_SUBTITLES,   FRONTEND_POS4X - 35, FRONTEND_POS4Y, _("Subtitles"), WBUT_SECONDARY);
	addTextButton(FRONTEND_SUBTITLES_R, FRONTEND_POS4M - 55, FRONTEND_POS4Y, graphicsOptionsSubtitlesString(), WBUT_SECONDARY);

	////////////
	//shadows
	addTextButton(FRONTEND_SHADOWS,   FRONTEND_POS5X - 35, FRONTEND_POS5Y, _("Shadows"), WBUT_SECONDARY);
	addTextButton(FRONTEND_SHADOWS_R, FRONTEND_POS5M - 55, FRONTEND_POS5Y, graphicsOptionsShadowsString(), WBUT_SECONDARY);

	// Radar
	addTextButton(FRONTEND_RADAR,   FRONTEND_POS6X - 35, FRONTEND_POS6Y, _("Radar"), WBUT_SECONDARY);
	addTextButton(FRONTEND_RADAR_R, FRONTEND_POS6M - 55, FRONTEND_POS6Y, graphicsOptionsRadarString(), WBUT_SECONDARY);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("GRAPHICS OPTIONS"));

	////////////
	// quit.
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	return true;
}

bool runGraphicsOptionsMenu(void)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	case FRONTEND_SUBTITLES:
	case FRONTEND_SUBTITLES_R:
		seq_SetSubtitles(!seq_GetSubtitles());
		widgSetString(psWScreen, FRONTEND_SUBTITLES_R, graphicsOptionsSubtitlesString());
		break;

	case FRONTEND_SHADOWS:
	case FRONTEND_SHADOWS_R:
		setDrawShadows(!getDrawShadows());
		widgSetString(psWScreen, FRONTEND_SHADOWS_R, graphicsOptionsShadowsString());
		break;

	case FRONTEND_FMVMODE:
	case FRONTEND_FMVMODE_R:
		war_SetFMVmode(stepCycle(war_GetFMVmode(), FMV_FULLSCREEN, FMV_MODE(FMV_MAX - 1)));
		widgSetString(psWScreen, FRONTEND_FMVMODE_R, graphicsOptionsFmvmodeString());
		break;

	case FRONTEND_SCANLINES:
	case FRONTEND_SCANLINES_R:
		war_setScanlineMode(stepCycle(war_getScanlineMode(), SCANLINES_OFF, SCANLINES_BLACK));
		widgSetString(psWScreen, FRONTEND_SCANLINES_R, graphicsOptionsScanlinesString());
		break;

	case FRONTEND_RADAR_R:
		rotateRadar = !rotateRadar;
		widgSetString(psWScreen, FRONTEND_RADAR_R, graphicsOptionsRadarString());
		break;

	default:
		break;
	}


	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Audio Options Menu
static bool startAudioOptionsMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	// 2d audio
	addTextButton(FRONTEND_FX, FRONTEND_POS2X - 25, FRONTEND_POS2Y, _("Voice Volume"), 0);
	addFESlider(FRONTEND_FX_SL, FRONTEND_BOTFORM, FRONTEND_POS2M, FRONTEND_POS2Y + 5, AUDIO_VOL_MAX, sound_GetUIVolume() * 100.0);

	// 3d audio
	addTextButton(FRONTEND_3D_FX, FRONTEND_POS3X - 25, FRONTEND_POS3Y, _("FX Volume"), 0);
	addFESlider(FRONTEND_3D_FX_SL, FRONTEND_BOTFORM, FRONTEND_POS3M, FRONTEND_POS3Y + 5, AUDIO_VOL_MAX, sound_GetEffectsVolume() * 100.0);

	// cd audio
	addTextButton(FRONTEND_MUSIC, FRONTEND_POS4X - 25, FRONTEND_POS4Y, _("Music Volume"), 0);
	addFESlider(FRONTEND_MUSIC_SL, FRONTEND_BOTFORM, FRONTEND_POS4M, FRONTEND_POS4Y + 5, AUDIO_VOL_MAX, sound_GetMusicVolume() * 100.0);

	// quit.
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	//add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("AUDIO OPTIONS"));


	return true;
}

bool runAudioOptionsMenu(void)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_FX:
	case FRONTEND_3D_FX:
	case FRONTEND_MUSIC:
		break;

	case FRONTEND_FX_SL:
		sound_SetUIVolume((float)widgGetSliderPos(psWScreen, FRONTEND_FX_SL) / 100.0);
		break;

	case FRONTEND_3D_FX_SL:
		sound_SetEffectsVolume((float)widgGetSliderPos(psWScreen, FRONTEND_3D_FX_SL) / 100.0);
		break;

	case FRONTEND_MUSIC_SL:
		sound_SetMusicVolume((float)widgGetSliderPos(psWScreen, FRONTEND_MUSIC_SL) / 100.0);
		break;


	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}

static char const *videoOptionsWindowModeString()
{
	return war_getFullscreen()? _("Fullscreen") : _("Windowed");
}

static std::string videoOptionsAntialiasingString()
{
	if (war_getAntialiasing() == 0)
	{
		return _("Off");
	}
	else
	{
		return std::to_string(war_getAntialiasing()) + "×";
	}
}

static std::string videoOptionsResolutionString()
{
	char resolution[100];
	ssprintf(resolution, "[%d] %d × %d", war_GetScreen(), war_GetWidth(), war_GetHeight());
	return resolution;
}

static std::string videoOptionsTextureSizeString()
{
	char textureSize[20];
	ssprintf(textureSize, "%d", getTextureSize());
	return textureSize;
}

static char const *videoOptionsVsyncString()
{
	return war_GetVsync()? _("On") : _("Off");
}

// ////////////////////////////////////////////////////////////////////////////
// Video Options
static bool startVideoOptionsMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	// Add a note about changes taking effect on restart for certain options
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	W_LABEL *label = new W_LABEL(parent);
	label->setGeometry(FRONTEND_POS1X + 48, FRONTEND_POS1Y, FRONTEND_BUTWIDTH - FRONTEND_POS1X - 48, FRONTEND_BUTHEIGHT);
	label->setFontColour(WZCOL_TEXT_BRIGHT);
	label->setString(_("* Takes effect on game restart"));
	label->setTextAlignment(WLAB_ALIGNBOTTOMLEFT);

	// Fullscreen/windowed
	addTextButton(FRONTEND_WINDOWMODE, FRONTEND_POS2X - 35, FRONTEND_POS2Y, _("Graphics Mode*"), WBUT_SECONDARY);
	addTextButton(FRONTEND_WINDOWMODE_R, FRONTEND_POS2M - 55, FRONTEND_POS2Y, videoOptionsWindowModeString(), WBUT_SECONDARY);

	// Resolution
	addTextButton(FRONTEND_RESOLUTION, FRONTEND_POS3X - 35, FRONTEND_POS3Y, _("Resolution*"), WBUT_SECONDARY);
	addTextButton(FRONTEND_RESOLUTION_R, FRONTEND_POS3M - 55, FRONTEND_POS3Y, videoOptionsResolutionString(), WBUT_SECONDARY);

	// Texture size
	addTextButton(FRONTEND_TEXTURESZ, FRONTEND_POS4X - 35, FRONTEND_POS4Y, _("Texture size"), WBUT_SECONDARY);
	addTextButton(FRONTEND_TEXTURESZ_R, FRONTEND_POS4M - 55, FRONTEND_POS4Y, videoOptionsTextureSizeString(), WBUT_SECONDARY);

	// Vsync
	addTextButton(FRONTEND_VSYNC, FRONTEND_POS5X - 35, FRONTEND_POS5Y, _("Vertical sync"), WBUT_SECONDARY);
	addTextButton(FRONTEND_VSYNC_R, FRONTEND_POS5M - 55, FRONTEND_POS5Y, videoOptionsVsyncString(), WBUT_SECONDARY);

	// Antialiasing
	addTextButton(FRONTEND_FSAA, FRONTEND_POS5X - 35, FRONTEND_POS6Y, "Antialiasing*", WBUT_SECONDARY);
	addTextButton(FRONTEND_FSAA_R, FRONTEND_POS5M - 55, FRONTEND_POS6Y, videoOptionsAntialiasingString(), WBUT_SECONDARY);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("VIDEO OPTIONS"));

	// Quit/return
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	return true;
}

bool runVideoOptionsMenu(void)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_WINDOWMODE:
	case FRONTEND_WINDOWMODE_R:
		war_setFullscreen(!war_getFullscreen());
		widgSetString(psWScreen, FRONTEND_WINDOWMODE_R, videoOptionsWindowModeString());
		break;

	case FRONTEND_FSAA:
	case FRONTEND_FSAA_R:
		{
			war_setAntialiasing(pow2Cycle(war_getAntialiasing(), 0, pie_GetMaxAntialiasing()));
			widgSetString(psWScreen, FRONTEND_FSAA_R, videoOptionsAntialiasingString().c_str());
			break;
		}

	case FRONTEND_RESOLUTION:
	case FRONTEND_RESOLUTION_R:
		{
			auto compareKey = [](screeninfo const &x) { return std::make_tuple(x.screen, x.width, x.height); };
			auto compareLess = [&](screeninfo const &a, screeninfo const &b) { return compareKey(a) < compareKey(b); };
			auto compareEq = [&](screeninfo const &a, screeninfo const &b) { return compareKey(a) == compareKey(b); };

			// Get resolutions, sorted with duplicates removed.
			std::vector<screeninfo> modes = wzAvailableResolutions();
			std::sort(modes.begin(), modes.end(), compareLess);
			modes.erase(std::unique(modes.begin(), modes.end(), compareEq), modes.end());

			// We can't pick resolutions if there aren't any.
			if (modes.empty())
			{
				debug(LOG_ERROR, "No resolutions available to change.");
				break;
			}

			// Get currently configured resolution.
			screeninfo config;
			config.screen = war_GetScreen();
			config.width = war_GetWidth();
			config.height = war_GetHeight();
			config.refresh_rate = -1;  // Unused.

			// Find current resolution in list.
			auto current = std::lower_bound(modes.begin(), modes.end(), config, compareLess);
			if (current == modes.end() || !compareEq(*current, config))
			{
				--current;  // If current resolution doesn't exist, round down to next-highest one.
			}

			// Increment/decrement and loop if required.
			current = stepCycle(current, modes.begin(), modes.end() - 1);

			// Set the new width and height (takes effect on restart)
			war_SetScreen(current->screen);
			war_SetWidth(current->width);
			war_SetHeight(current->height);

			// Update the widget
			widgSetString(psWScreen, FRONTEND_RESOLUTION_R, videoOptionsResolutionString().c_str());
			break;
		}

	case FRONTEND_TEXTURESZ:
	case FRONTEND_TEXTURESZ_R:
		{
			int newTexSize = pow2Cycle(getTextureSize(), 128, 2048);

			// Set the new size
			setTextureSize(newTexSize);

			// Update the widget
			widgSetString(psWScreen, FRONTEND_TEXTURESZ_R, videoOptionsTextureSizeString().c_str());

			break;
		}

	case FRONTEND_VSYNC:
	case FRONTEND_VSYNC_R:
		wzSetSwapInterval(!war_GetVsync());
		war_SetVsync(wzGetSwapInterval());
		widgSetString(psWScreen, FRONTEND_VSYNC_R, videoOptionsVsyncString());
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);

	return true;
}


static char const *mouseOptionsMflipString()
{
	return getInvertMouseStatus() ? _("On") : _("Off");
}

static char const *mouseOptionsTrapString()
{
	return war_GetTrapCursor() ? _("On") : _("Off");
}

static char const *mouseOptionsMbuttonsString()
{
	return getRightClickOrders() ? _("On") : _("Off");
}

static char const *mouseOptionsMmrotateString()
{
	return getMiddleClickRotate() ? _("Middle Mouse") : _("Right Mouse");
}

static char const *mouseOptionsCursorModeString()
{
	return war_GetColouredCursor() ? _("On") : _("Off");
}

// ////////////////////////////////////////////////////////////////////////////
// Mouse Options
static bool startMouseOptionsMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	////////////
	// mouseflip
	addTextButton(FRONTEND_MFLIP,   FRONTEND_POS2X - 35, FRONTEND_POS2Y, _("Reverse Rotation"), WBUT_SECONDARY);
	addTextButton(FRONTEND_MFLIP_R, FRONTEND_POS2M - 25, FRONTEND_POS2Y, mouseOptionsMflipString(), WBUT_SECONDARY);

	// Cursor trapping
	addTextButton(FRONTEND_TRAP,   FRONTEND_POS3X - 35, FRONTEND_POS3Y, _("Trap Cursor"), WBUT_SECONDARY);
	addTextButton(FRONTEND_TRAP_R, FRONTEND_POS3M - 25, FRONTEND_POS3Y, mouseOptionsTrapString(), WBUT_SECONDARY);

	////////////
	// left-click orders
	addTextButton(FRONTEND_MBUTTONS,   FRONTEND_POS2X - 35, FRONTEND_POS4Y, _("Switch Mouse Buttons"), WBUT_SECONDARY);
	addTextButton(FRONTEND_MBUTTONS_R, FRONTEND_POS2M - 25, FRONTEND_POS4Y, mouseOptionsMbuttonsString(), WBUT_SECONDARY);

	////////////
	// middle-click rotate
	addTextButton(FRONTEND_MMROTATE,   FRONTEND_POS2X - 35, FRONTEND_POS5Y, _("Rotate Screen"), WBUT_SECONDARY);
	addTextButton(FRONTEND_MMROTATE_R, FRONTEND_POS2M - 25, FRONTEND_POS5Y, mouseOptionsMmrotateString(), WBUT_SECONDARY);

	// Hardware / software cursor toggle
	addTextButton(FRONTEND_CURSORMODE,   FRONTEND_POS4X - 35, FRONTEND_POS6Y, _("Colored Cursors"), WBUT_SECONDARY);
	addTextButton(FRONTEND_CURSORMODE_R, FRONTEND_POS4M - 25, FRONTEND_POS6Y, mouseOptionsCursorModeString(), WBUT_SECONDARY);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("MOUSE OPTIONS"));

	// Quit/return
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	return true;
}

bool runMouseOptionsMenu(void)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_MFLIP:
	case FRONTEND_MFLIP_R:
		setInvertMouseStatus(!getInvertMouseStatus());
		widgSetString(psWScreen, FRONTEND_MFLIP_R, mouseOptionsMflipString());
		break;
	case FRONTEND_TRAP:
	case FRONTEND_TRAP_R:
		war_SetTrapCursor(!war_GetTrapCursor());
		widgSetString(psWScreen, FRONTEND_TRAP_R, mouseOptionsTrapString());
		break;

	case FRONTEND_MBUTTONS:
	case FRONTEND_MBUTTONS_R:
		setRightClickOrders(!getRightClickOrders());
		widgSetString(psWScreen, FRONTEND_MBUTTONS_R, mouseOptionsMbuttonsString());
		break;

	case FRONTEND_MMROTATE:
	case FRONTEND_MMROTATE_R:
		setMiddleClickRotate(!getMiddleClickRotate());
		widgSetString(psWScreen, FRONTEND_MMROTATE_R, mouseOptionsMmrotateString());
		break;

	case FRONTEND_CURSORMODE:
	case FRONTEND_CURSORMODE_R:
		war_SetColouredCursor(!war_GetColouredCursor());
		widgSetString(psWScreen, FRONTEND_CURSORMODE_R, mouseOptionsCursorModeString());
		wzSetCursor(CURSOR_DEFAULT);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);

	return true;
}

static char const *gameOptionsDifficultyString()
{
	switch (getDifficultyLevel())
	{
	case DL_EASY: return _("Easy");
	case DL_NORMAL: return _("Normal");
	case DL_HARD: return _("Hard");
	case DL_INSANE: return _("Insane");
	case DL_TOUGH: return _("Tough");
	case DL_KILLER: return _("Killer");
	}
	return _("Unsupported");
}

// ////////////////////////////////////////////////////////////////////////////
// Game Options Menu
static bool startGameOptionsMenu(void)
{
	UDWORD	w, h;
	int playercolor;

	addBackdrop();
	addTopForm();
	addBottomForm();

	// language
	addTextButton(FRONTEND_LANGUAGE,  FRONTEND_POS2X - 25, FRONTEND_POS2Y, _("Language"), WBUT_SECONDARY);
	addTextButton(FRONTEND_LANGUAGE_R,  FRONTEND_POS2M - 25, FRONTEND_POS2Y, getLanguageName(), WBUT_SECONDARY);

	// Difficulty
	addTextButton(FRONTEND_DIFFICULTY,   FRONTEND_POS3X - 25, FRONTEND_POS3Y, _("Campaign Difficulty"), WBUT_SECONDARY);
	addTextButton(FRONTEND_DIFFICULTY_R, FRONTEND_POS3M - 25, FRONTEND_POS3Y, gameOptionsDifficultyString(), WBUT_SECONDARY);

	// Scroll speed
	addTextButton(FRONTEND_SCROLLSPEED, FRONTEND_POS4X - 25, FRONTEND_POS4Y, _("Scroll Speed"), 0);
	addFESlider(FRONTEND_SCROLLSPEED_SL, FRONTEND_BOTFORM, FRONTEND_POS4M, FRONTEND_POS4Y + 5, 16, scroll_speed_accel / 100);

	// Colour stuff
	addTextButton(FRONTEND_COLOUR, FRONTEND_POS5X - 25, FRONTEND_POS5Y, _("Unit Colour:"), 0);

	w = iV_GetImageWidth(FrontImages, IMAGE_PLAYERN);
	h = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);

	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P0, FRONTEND_POS6M + (0 * (w + 6)), FRONTEND_POS6Y, w, h, NULL, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 0);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P4, FRONTEND_POS6M + (1 * (w + 6)), FRONTEND_POS6Y, w, h, NULL, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 4);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P5, FRONTEND_POS6M + (2 * (w + 6)), FRONTEND_POS6Y, w, h, NULL, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 5);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P6, FRONTEND_POS6M + (3 * (w + 6)), FRONTEND_POS6Y, w, h, NULL, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 6);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P7, FRONTEND_POS6M + (4 * (w + 6)), FRONTEND_POS6Y, w, h, NULL, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 7);

	// FIXME: if playercolor = 1-3, then we Assert in widgSetButtonState() since we don't define FE_P1 - FE_P3
	// I assume the reason is that in SP games, those are reserved for the AI?  Valid values are 0, 4-7.
	// This is a workaround, until we find what is setting that to 1-3.  See configuration.c:701
	playercolor = war_GetSPcolor();
	if (playercolor >= 1 && playercolor <= 3)
	{
		playercolor = 0;
	}
	widgSetButtonState(psWScreen, FE_P0 + playercolor, WBUT_LOCK);
	addTextButton(FRONTEND_COLOUR_CAM, FRONTEND_POS6X, FRONTEND_POS6Y, _("Campaign"), 0);

	playercolor = war_getMPcolour();
	for (int colour = -1; colour < MAX_PLAYERS_IN_GUI; ++colour)
	{
		int cellX = (colour + 1) % 7;
		int cellY = (colour + 1) / 7;
		addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_MP_PR + colour + 1, FRONTEND_POS7M + cellX * (w + 2), FRONTEND_POS7Y + cellY * (h + 2) - 5, w, h, NULL, IMAGE_PLAYERN, IMAGE_PLAYERX, true, colour >= 0 ? colour : MAX_PLAYERS + 1);
	}
	widgSetButtonState(psWScreen, FE_MP_PR + playercolor + 1, WBUT_LOCK);
	addTextButton(FRONTEND_COLOUR_MP, FRONTEND_POS7X, FRONTEND_POS7Y, _("Skirmish/Multiplayer"), 0);

	// Quit
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("GAME OPTIONS"));

	return true;
}

bool runGameOptionsMenu(void)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_LANGUAGE_R:
		setNextLanguage(mouseReleased(MOUSE_RMB));
		widgSetString(psWScreen, FRONTEND_LANGUAGE_R, getLanguageName());
		/* Hack to reset current menu text, which looks fancy. */
		widgSetString(psWScreen, FRONTEND_SIDETEXT, _("GAME OPTIONS"));
		widgGetFromID(psWScreen, FRONTEND_QUIT)->setTip(P_("menu", "Return"));
		widgSetString(psWScreen, FRONTEND_LANGUAGE, _("Language"));
		widgSetString(psWScreen, FRONTEND_COLOUR, _("Unit Colour:"));
		widgSetString(psWScreen, FRONTEND_COLOUR_CAM, _("Campaign"));
		widgSetString(psWScreen, FRONTEND_COLOUR_MP, _("Skirmish/Multiplayer"));
		widgSetString(psWScreen, FRONTEND_DIFFICULTY, _("Campaign Difficulty"));
		widgSetString(psWScreen, FRONTEND_SCROLLSPEED, _("Scroll Speed"));
		widgSetString(psWScreen, FRONTEND_DIFFICULTY_R, gameOptionsDifficultyString());
		break;

	case FRONTEND_SCROLLSPEED:
		break;

	case FRONTEND_DIFFICULTY:
	case FRONTEND_DIFFICULTY_R:
		setDifficultyLevel(stepCycle(getDifficultyLevel(), DL_EASY, DL_HARD));
		widgSetString(psWScreen, FRONTEND_DIFFICULTY_R, gameOptionsDifficultyString());
		break;

	case FRONTEND_SCROLLSPEED_SL:
		scroll_speed_accel = widgGetSliderPos(psWScreen, FRONTEND_SCROLLSPEED_SL) * 100; //0-1600
		if (scroll_speed_accel == 0)		// make sure you CAN scroll.
		{
			scroll_speed_accel = 100;
		}
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	case FE_P0:
		widgSetButtonState(psWScreen, FE_P0, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(0);
		break;
	case FE_P4:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(4);
		break;
	case FE_P5:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(5);
		break;
	case FE_P6:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(6);
		break;
	case FE_P7:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, WBUT_LOCK);
		war_SetSPcolor(7);
		break;

	default:
		break;
	}

	if (id >= FE_MP_PR && id <= FE_MP_PMAX)
	{
		int chosenColour = id - FE_MP_PR - 1;
		for (int colour = -1; colour < MAX_PLAYERS_IN_GUI; ++colour)
		{
			unsigned thisID = FE_MP_PR + colour + 1;
			widgSetButtonState(psWScreen, thisID, id == thisID ? WBUT_LOCK : 0);
		}
		war_setMPcolour(chosenColour);
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}

static void cycleResolution(bool forwards)
{
	auto compareKey = [](screeninfo const &x) { return std::make_tuple(x.screen, x.width, x.height); };
	auto compareLess = [&](screeninfo const &a, screeninfo const &b) { return compareKey(a) < compareKey(b); };
	auto compareEq = [&](screeninfo const &a, screeninfo const &b) { return compareKey(a) == compareKey(b); };

	// Get resolutions, sorted with duplicates removed.
	std::vector<screeninfo> modes = wzAvailableResolutions();
	std::sort(modes.begin(), modes.end(), compareLess);
	modes.erase(std::unique(modes.begin(), modes.end(), compareEq), modes.end());

	// We can't pick resolutions if there aren't any.
	if (modes.empty())
	{
		debug(LOG_ERROR, "No resolutions available to change.");
		return;
	}

	// Get currently configured resolution.
	screeninfo config;
	config.screen = war_GetScreen();
	config.width = war_GetWidth();
	config.height = war_GetHeight();
	config.refresh_rate = -1;  // Unused.

	// Find current resolution in list.
	auto current = std::lower_bound(modes.begin(), modes.end(), config, compareLess);
	if (current == modes.end() || !compareEq(*current, config))
	{
		--current;  // If current resolution doesn't exist, round down to next-highest one.
	}

	// Increment/decrement and loop if required.
	current = stepCycleDir(forwards, current, modes.begin(), modes.end() - 1);

	// Set the new width and height (takes effect on restart)
	war_SetScreen(current->screen);
	war_SetWidth(current->width);
	war_SetHeight(current->height);
}

static void doAudioOptionsMenu()
{
	ImVec2 windowSize = ImGui::GetWindowSize();

	ImGui::PushID("audopts");

	ImGui::Columns(2, "cols", false);
	ImGui::SetColumnWidth(0, windowSize.x * 0.35f);
	ImGui::SetColumnWidth(1, windowSize.x * 0.65f);

	ImGui::TextWrapped(_("Voice Volume"));

	ImGui::NextColumn();

	static const float volume_min = 0;
	static const float volume_max = 100.0f;

	float cur_vol = sound_GetUIVolume() * 100.0f;
	if (ImGui::SliderScalar("##vv", ImGuiDataType_Float, &cur_vol,
			    &volume_min, &volume_max, ""))
		sound_SetUIVolume(cur_vol / 100.0f);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("FX Volume"));

	ImGui::NextColumn();

	cur_vol = sound_GetEffectsVolume() * 100.0f;
	if (ImGui::SliderScalar("##fxv", ImGuiDataType_Float, &cur_vol,
			    &volume_min, &volume_max, ""))
		sound_SetEffectsVolume(cur_vol / 100.0f);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Music Volume"));

	ImGui::NextColumn();

	cur_vol = sound_GetMusicVolume() * 100.0f;
	if (ImGui::SliderScalar("##mv", ImGuiDataType_Float, &cur_vol,
			    &volume_min, &volume_max, ""))
		sound_SetMusicVolume(cur_vol / 100.0f);

	ImGui::Columns();

	ImGui::PopID();
}

static void doGameOptionsMenu()
{
	ImVec2 windowSize = ImGui::GetWindowSize();

	ImGui::PushID("gameopts");

	ImGui::Columns(2, "cols", false);
	ImGui::SetColumnWidth(0, windowSize.x * 0.35f);
	ImGui::SetColumnWidth(1, windowSize.x * 0.65f);

	ImGui::TextWrapped(_("Language"));

	ImGui::NextColumn();

	ImGui::PushID("lng");

	if (ImGui::ArrowButton("##left", ImGuiDir_Left))
		setNextLanguage(true);
	ImGui::SameLine();
	if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		setNextLanguage(false);
	ImGui::SameLine();
	ImGui::TextWrapped("%s", getLanguageName());

	ImGui::PopID();

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Campaign Difficulty"));

	ImGui::NextColumn();

	int dl_idx = static_cast<int>(getDifficultyLevel());

	static std::array<char*, 3> str_array;
	str_array[0] = _("Easy");
	str_array[1] = _("Normal");
	str_array[2] = _("Hard");

	if (ImGui::Combo("##dl", &dl_idx,
			 &str_array.front(), str_array.size()))
	{
		setDifficultyLevel(static_cast<DIFFICULTY_LEVEL>(dl_idx));
	}

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Scroll Speed"));

	ImGui::NextColumn();

	static UDWORD scroll_speed_accel_min = 100;
	static UDWORD scroll_speed_accel_max = 1600;

	ImGui::SliderScalar("##sa", ImGuiDataType_U32, &scroll_speed_accel,
			    &scroll_speed_accel_min, &scroll_speed_accel_max, "");

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Unit Colour:"));

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Campaign"));
	ImGui::SameLine();

	ImGui::PushID("spcol");

	static std::array<ImVec4, MAX_PLAYERS_IN_GUI> tcarr;
	for (int colour = 0; colour < MAX_PLAYERS_IN_GUI; ++colour)
	{
		tcarr[colour] = pal_PIELIGHTtoImVec4(pal_GetTeamColour(colour));
	}

	// FIXME: if playercolor = 1-3, then we Assert in widgSetButtonState() since we don't define FE_P1 - FE_P3
	// I assume the reason is that in SP games, those are reserved for the AI?  Valid values are 0, 4-7.
	// This is a workaround, until we find what is setting that to 1-3.  See configuration.c:701
	int8_t playercolor = war_GetSPcolor();
	if ((playercolor >= 1 && playercolor <= 3) || playercolor >= MAX_PLAYERS_IN_GUI)
	{
		playercolor = 0;
	}
	ImVec4 tc = tcarr[playercolor];

	if (ImGui::Wz::ColorButtonFE("##cur", tc))
		ImGui::OpenPopup("sppalette");

	if (ImGui::BeginPopup("sppalette"))
	{
		if (ImGui::Wz::ColorButtonFE("##tc0", tcarr[0]))
			war_SetSPcolor(0);
		for (int spcnt = 4; spcnt < 8; ++spcnt)
		{
			ImGui::SameLine();
			ImGui::PushID(spcnt);
			if (ImGui::Wz::ColorButtonFE("##tc", tcarr[spcnt]))
				war_SetSPcolor(spcnt);
			ImGui::PopID();
		}
		ImGui::EndPopup();
	}

	ImGui::PopID();

	ImGui::NextColumn();
	ImGui::NextColumn();

	ImGui::TextWrapped(_("Skirmish/Multiplayer"));
	ImGui::SameLine();

	ImGui::PushID("mpcol");

	ImVec4 tcmix;
	const int scale = 4000;
	int f = realTime % scale;

	tcmix.x = 0.5f + iSinR(65536 * f / scale + 65536 * 0 / 3, 127) / 255.0f;
	tcmix.y = 0.5f + iSinR(65536 * f / scale + 65536 * 1 / 3, 127) / 255.0f;
	tcmix.z = 0.5f + iSinR(65536 * f / scale + 65536 * 2 / 3, 127) / 255.0f;
	tcmix.w = 1.0f;

	playercolor = war_getMPcolour();
	if (playercolor < 0)
		tc = tcmix;
	else
		tc = tcarr[playercolor];

	if (ImGui::Wz::ColorButtonFE("##cur", tc))
		ImGui::OpenPopup("mppalette");

	if (ImGui::BeginPopup("mppalette"))
	{
		if (ImGui::Wz::ColorButtonFE("##tcdyn", tcmix))
			war_setMPcolour(-1);
		for (int mpcnt = 0; mpcnt < MAX_PLAYERS_IN_GUI; ++mpcnt)
		{
			ImGui::SameLine();
			ImGui::PushID(mpcnt);
			if (ImGui::Wz::ColorButtonFE("##tc", tcarr[mpcnt]))
				war_setMPcolour(mpcnt);
			ImGui::PopID();
		}
		ImGui::EndPopup();
	}

	ImGui::PopID();

	ImGui::Columns();

	ImGui::PopID();
}

static void doGraphicsOptionsMenu()
{
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::PushID("gfxopts");

	ImGui::Columns(2, "cols", false);
	ImGui::SetColumnWidth(0, windowSize.x * 0.35f);
	ImGui::SetColumnWidth(1, windowSize.x * 0.65f);

	ImGui::TextWrapped(_("Video Playback"));

	ImGui::NextColumn();

	int fmv_idx = static_cast<int>(war_GetFMVmode());

	static std::array<char*, 3> str_array_fmv;
	str_array_fmv[0] = _("Fullscreen");
	str_array_fmv[1] = _("1×");
	str_array_fmv[2] = _("2×");

	if (ImGui::Combo("##fmv", &fmv_idx,
			 &str_array_fmv.front(), str_array_fmv.size()))
	{
		war_SetFMVmode(static_cast<FMV_MODE>(fmv_idx));
	}

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Scanlines"));

	ImGui::NextColumn();

	int scn_idx = static_cast<int>(war_getScanlineMode());

	static std::array<char*, 3> str_array_scn;
	str_array_scn[0] = _("Off");
	str_array_scn[1] = _("50%");
	str_array_scn[2] = _("Black");

	if (ImGui::Combo("##scn", &scn_idx,
			 &str_array_scn.front(), str_array_scn.size()))
	{
		war_setScanlineMode(static_cast<SCANLINE_MODE>(scn_idx));
	}

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Subtitles"));

	ImGui::NextColumn();

	bool bool_val = seq_GetSubtitles();
	if (ImGui::Checkbox("##sbt", &bool_val))
		seq_SetSubtitles(bool_val);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Shadows"));

	ImGui::NextColumn();

	bool_val = getDrawShadows();
	if (ImGui::Checkbox("##shd", &bool_val))
		setDrawShadows(bool_val);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Radar"));

	ImGui::NextColumn();

	int radiobtn_val = rotateRadar ? 1 : 0;
	ImGui::RadioButton(_("Fixed"), &radiobtn_val, 0); ImGui::SameLine();
	ImGui::RadioButton(_("Rotating"), &radiobtn_val, 1);
	rotateRadar = radiobtn_val == 1;

	ImGui::Columns();

	ImGui::PopID();
}

static void doMouseOptionsMenu()
{
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::PushID("mouseopts");

	ImGui::Columns(2, "cols", false);
	ImGui::SetColumnWidth(0, windowSize.x * 0.35f);
	ImGui::SetColumnWidth(1, windowSize.x * 0.65f);

	ImGui::TextWrapped(_("Reverse Rotation"));

	ImGui::NextColumn();

	bool bool_val = getInvertMouseStatus();
	if (ImGui::Checkbox("##invert", &bool_val))
		setInvertMouseStatus(bool_val);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Trap Cursor"));

	ImGui::NextColumn();

	bool_val = war_GetTrapCursor();
	if (ImGui::Checkbox("##trap", &bool_val))
		war_SetTrapCursor(bool_val);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Switch Mouse Buttons"));

	ImGui::NextColumn();

	bool_val = getRightClickOrders();
	if (ImGui::Checkbox("##rclick", &bool_val))
		setRightClickOrders(bool_val);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Rotate Screen"));

	ImGui::NextColumn();

	int radiobtn_val = getMiddleClickRotate() ? 1 : 0;
	if (ImGui::RadioButton(_("Right Mouse"), &radiobtn_val, 0))
		setMiddleClickRotate(false);
	ImGui::SameLine();
	if (ImGui::RadioButton(_("Middle Mouse"), &radiobtn_val, 1))
		setMiddleClickRotate(true);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Colored Cursors"));

	ImGui::NextColumn();

	bool_val = war_GetColouredCursor();
	if (ImGui::Checkbox("##rotate", &bool_val))
	{
		war_SetColouredCursor(bool_val);
		wzSetCursor(CURSOR_DEFAULT);
	}

	ImGui::Columns();

	ImGui::PopID();
}

static void doVideoOptionsMenu()
{
	ImVec2 windowSize = ImGui::GetWindowSize();

	ImGui::PushID("vidopts");

	ImGui::TextDisabled(_("* Takes effect on game restart"));

	ImGui::Columns(2, "cols", false);
	ImGui::SetColumnWidth(0, windowSize.x * 0.35f);
	ImGui::SetColumnWidth(1, windowSize.x * 0.65f);

	ImGui::TextWrapped(_("Graphics Mode*"));

	ImGui::NextColumn();

	int radiobtn_val = war_getFullscreen() ? 1 : 0;
	if (ImGui::RadioButton(_("Windowed"), &radiobtn_val, 0))
		war_setFullscreen(false);
	ImGui::SameLine();
	if (ImGui::RadioButton(_("Fullscreen"), &radiobtn_val, 1))
		war_setFullscreen(true);

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Resolution*"));

	ImGui::NextColumn();

	ImGui::PushID("res");

	if (ImGui::ArrowButton("##left", ImGuiDir_Left))
		cycleResolution(false);
	ImGui::SameLine();
	if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		cycleResolution(true);
	ImGui::SameLine();
	ImGui::Text("%s", videoOptionsResolutionString().c_str());

	ImGui::PopID();

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Texture size"));

	ImGui::NextColumn();

	ImGui::PushID("tsize");

	if (ImGui::ArrowButton("##left", ImGuiDir_Left))
		setTextureSize(pow2CycleDir(false, getTextureSize(), 128, 2048));
	ImGui::SameLine();
	if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		setTextureSize(pow2CycleDir(true, getTextureSize(), 128, 2048));
	ImGui::SameLine();
	ImGui::Text("%s", videoOptionsTextureSizeString().c_str());

	ImGui::PopID();

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Vertical sync"));

	ImGui::NextColumn();

	bool bool_val = war_GetVsync();
	if (ImGui::Checkbox("##vsync", &bool_val))
	{
		wzSetSwapInterval(bool_val);
		war_SetVsync(wzGetSwapInterval());
	}

	ImGui::NextColumn();

	ImGui::TextWrapped(_("Antialiasing*"));

	ImGui::NextColumn();

	ImGui::PushID("aa");

	if (ImGui::ArrowButton("##left", ImGuiDir_Left))
		war_setAntialiasing(pow2CycleDir(false, war_getAntialiasing(),
					      0, pie_GetMaxAntialiasing()));
	ImGui::SameLine();
	if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		war_setAntialiasing(pow2CycleDir(true, war_getAntialiasing(),
					      0, pie_GetMaxAntialiasing()));
	ImGui::SameLine();
	ImGui::Text("%s", videoOptionsAntialiasingString().c_str());

	ImGui::PopID();

	ImGui::Columns();

	ImGui::PopID();
}

// ////////////////////////////////////////////////////////////////////////////
// drawing functions

// show a background piccy (currently used for version and mods labels)
static void displayTitleBitmap(WZ_DECL_UNUSED WIDGET *psWidget, WZ_DECL_UNUSED UDWORD xOffset, WZ_DECL_UNUSED UDWORD yOffset)
{
	char modListText[MAX_STR_LENGTH] = "";

	iV_SetTextColour(WZCOL_GREY);
	iV_DrawTextRotated(version_getFormattedVersionString(), pie_GetVideoBufferWidth() - 9, pie_GetVideoBufferHeight() - 14, 270.f, font_regular);

	if (!getModList().empty())
	{
		sstrcat(modListText, _("Mod: "));
		sstrcat(modListText, getModList().c_str());
		iV_DrawText(modListText, 9, 14, font_regular);
	}

	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawTextRotated(version_getFormattedVersionString(), pie_GetVideoBufferWidth() - 10, pie_GetVideoBufferHeight() - 15, 270.f, font_regular);

	if (!getModList().empty())
	{
		iV_DrawText(modListText, 10, 15, font_regular);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// show warzone logo
static void displayLogo(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	iV_DrawImage2("image_fe_logo.png", xOffset + psWidget->x(), yOffset + psWidget->y(), psWidget->width(), psWidget->height());
}


// ////////////////////////////////////////////////////////////////////////////
// show, well have a guess..
static void displayBigSlider(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_SLIDER *Slider = (W_SLIDER *)psWidget;
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	iV_DrawImage(IntImages, IMAGE_SLIDER_BIG, x + STAT_SLD_OX, y + STAT_SLD_OY);			// draw bdrop

	int sx = (Slider->width() - 3 - Slider->barSize) * Slider->pos / Slider->numStops;  // determine pos.
	iV_DrawImage(IntImages, IMAGE_SLIDER_BIGBUT, x + 3 + sx, y + 3);								//draw amount
}

// ////////////////////////////////////////////////////////////////////////////
// show text written on its side.
static void displayTextAt270(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD		fx, fy;
	W_LABEL		*psLab;

	psLab = (W_LABEL *)psWidget;

	fx = xOffset + psWidget->x();
	fy = yOffset + psWidget->y() + iV_GetTextWidth(psLab->aText.toUtf8().constData(), font_large);

	iV_SetTextColour(WZCOL_GREY);
	iV_DrawTextRotated(psLab->aText.toUtf8().constData(), fx + 2, fy + 2, 270.f, font_large);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawTextRotated(psLab->aText.toUtf8().constData(), fx, fy, 270.f, font_large);
}

// ////////////////////////////////////////////////////////////////////////////
// show a text option.
void displayTextOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD			fx, fy, fw;
	W_BUTTON		*psBut = (W_BUTTON *)psWidget;
	bool			hilight = false;
	bool			greyOut = psWidget->UserData; // if option is unavailable.

	if (widgGetMouseOver(psWScreen) == psBut->id)					// if mouse is over text then hilight.
	{
		hilight = true;
	}

	fw = iV_GetTextWidth(psBut->pText.toUtf8().constData(), psBut->FontID);
	fy = yOffset + psWidget->y() + (psWidget->height() - iV_GetTextLineSize(psBut->FontID)) / 2 - iV_GetTextAboveBase(psBut->FontID);

	if (psWidget->style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = xOffset + psWidget->x() + ((psWidget->width() - fw) / 2);
	}
	else
	{
		fx = xOffset + psWidget->x();
	}

	if (greyOut)														// unavailable
	{
		iV_SetTextColour(WZCOL_TEXT_DARK);
	}
	else															// available
	{
		if (hilight)													// hilight
		{
			iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		}
		else if (psWidget->id == FRONTEND_HYPERLINK || psWidget->id == FRONTEND_DONATELINK)				// special case for our hyperlink
		{
			iV_SetTextColour(WZCOL_YELLOW);
		}
		else														// dont highlight
		{
			iV_SetTextColour(WZCOL_TEXT_MEDIUM);
		}
	}

	iV_DrawText(psBut->pText.toUtf8().constData(), fx, fy, psBut->FontID);

	return;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// common widgets.

void addBackdrop(void)
{
	W_FORMINIT sFormInit;                              // Backdrop
	sFormInit.formID = 0;
	sFormInit.id = FRONTEND_BACKDROP;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)((pie_GetVideoBufferWidth() - HIDDEN_FRONTEND_WIDTH) / 2);
	sFormInit.y = (SWORD)((pie_GetVideoBufferHeight() - HIDDEN_FRONTEND_HEIGHT) / 2);
	sFormInit.width = HIDDEN_FRONTEND_WIDTH - 1;
	sFormInit.height = HIDDEN_FRONTEND_HEIGHT - 1;
	sFormInit.pDisplay = displayTitleBitmap;
	widgAddForm(psWScreen, &sFormInit);
}

// ////////////////////////////////////////////////////////////////////////////
void addTopForm(void)
{
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *topForm = new IntFormAnimated(parent, false);
	topForm->id = FRONTEND_TOPFORM;
	if (titleMode == MULTIOPTION)
	{
		topForm->setGeometry(FRONTEND_TOPFORM_WIDEX, FRONTEND_TOPFORM_WIDEY, FRONTEND_TOPFORM_WIDEW, FRONTEND_TOPFORM_WIDEH);
	}
	else
	{
		topForm->setGeometry(FRONTEND_TOPFORMX, FRONTEND_TOPFORMY, FRONTEND_TOPFORMW, FRONTEND_TOPFORMH);
	}

	W_FORMINIT sFormInit;
	sFormInit.formID = FRONTEND_TOPFORM;
	sFormInit.id	= FRONTEND_LOGO;
	int imgW = iV_GetImageWidth(FrontImages, IMAGE_FE_LOGO);
	int imgH = iV_GetImageHeight(FrontImages, IMAGE_FE_LOGO);
	int dstW = topForm->width();
	int dstH = topForm->height();
	if (imgW * dstH < imgH * dstW) // Want to set aspect ratio dstW/dstH = imgW/imgH.
	{
		dstW = imgW * dstH / imgH; // Too wide.
	}
	else if (imgW * dstH > imgH * dstW)
	{
		dstH = imgH * dstW / imgW; // Too high.
	}
	sFormInit.x = (topForm->width()  - dstW) / 2;
	sFormInit.y = (topForm->height() - dstH) / 2;
	sFormInit.width  = dstW;
	sFormInit.height = dstH;
	sFormInit.pDisplay = displayLogo;
	widgAddForm(psWScreen, &sFormInit);
}

// ////////////////////////////////////////////////////////////////////////////
void addBottomForm(void)
{
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *botForm = new IntFormAnimated(parent);
	botForm->id = FRONTEND_BOTFORM;
	botForm->setGeometry(FRONTEND_BOTFORMX, FRONTEND_BOTFORMY, FRONTEND_BOTFORMW, FRONTEND_BOTFORMH);
}

// ////////////////////////////////////////////////////////////////////////////
void addText(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, UDWORD formID)
{
	WIDGET *parent = widgGetFromID(psWScreen, formID);

	W_LABEL *label = new W_LABEL(parent);
	label->id = id;
	label->setGeometry(PosX, PosY, MULTIOP_READY_WIDTH, FRONTEND_BUTHEIGHT);
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setFont(font_small, WZCOL_TEXT_BRIGHT);
	label->setString(txt);
}

// ////////////////////////////////////////////////////////////////////////////
void addSideText(UDWORD id,  UDWORD PosX, UDWORD PosY, const char *txt)
{
	W_LABINIT sLabInit;

	sLabInit.formID = FRONTEND_BACKDROP;
	sLabInit.id = id;
	sLabInit.x = (short) PosX;
	sLabInit.y = (short) PosY;
	sLabInit.width = 30;
	sLabInit.height = FRONTEND_BOTFORMH;

	sLabInit.FontID = font_large;

	sLabInit.pDisplay = displayTextAt270;
	sLabInit.pText = txt;
	widgAddLabel(psWScreen, &sLabInit);
}

// ////////////////////////////////////////////////////////////////////////////
void addTextButton(UDWORD id,  UDWORD PosX, UDWORD PosY, const std::string &txt, unsigned int style)
{
	W_BUTINIT sButInit;

	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.id = id;
	sButInit.x = (short)PosX;
	sButInit.y = (short)PosY;

	// Align
	if (!(style & WBUT_TXTCENTRE))
	{
		sButInit.width = (short)(iV_GetTextWidth(txt.c_str(), font_large) + 10);
		sButInit.x += 35;
	}
	else
	{
		sButInit.style |= WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
	}

	// Enable right clicks
	if (style & WBUT_SECONDARY)
	{
		sButInit.style |= WBUT_SECONDARY;
	}

	sButInit.UserData = (style & WBUT_DISABLE); // store disable state

	sButInit.height = FRONTEND_BUTHEIGHT;
	sButInit.pDisplay = displayTextOption;
	sButInit.FontID = font_large;
	sButInit.pText = txt.c_str();
	widgAddButton(psWScreen, &sButInit);

	// Disable button
	if (style & WBUT_DISABLE)
	{
		widgSetButtonState(psWScreen, id, WBUT_DISABLE);
	}
}

void addSmallTextButton(UDWORD id,  UDWORD PosX, UDWORD PosY, const char *txt, unsigned int style)
{
	W_BUTINIT sButInit;

	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.id = id;
	sButInit.x = (short)PosX;
	sButInit.y = (short)PosY;

	// Align
	if (!(style & WBUT_TXTCENTRE))
	{
		sButInit.width = (short)(iV_GetTextWidth(txt, font_small) + 10);
		sButInit.x += 35;
	}
	else
	{
		sButInit.style |= WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
	}

	// Enable right clicks
	if (style & WBUT_SECONDARY)
	{
		sButInit.style |= WBUT_SECONDARY;
	}

	sButInit.UserData = (style & WBUT_DISABLE); // store disable state

	sButInit.height = FRONTEND_BUTHEIGHT;
	sButInit.pDisplay = displayTextOption;
	sButInit.FontID = font_small;
	sButInit.pText = txt;
	widgAddButton(psWScreen, &sButInit);

	// Disable button
	if (style & WBUT_DISABLE)
	{
		widgSetButtonState(psWScreen, id, WBUT_DISABLE);
	}
}

// ////////////////////////////////////////////////////////////////////////////
void addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos)
{
	W_SLDINIT sSldInit;
	sSldInit.formID		= parent;
	sSldInit.id			= id;
	sSldInit.x			= (short)x;
	sSldInit.y			= (short)y;
	sSldInit.width		= iV_GetImageWidth(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.height		= iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.numStops	= (UBYTE) stops;
	sSldInit.barSize	= iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.pos		= (UBYTE) pos;
	sSldInit.pDisplay	= displayBigSlider;
	sSldInit.pCallback  = intUpdateQuantitySlider;
	widgAddSlider(psWScreen, &sSldInit);
}

// ////////////////////////////////////////////////////////////////////////////
// Change Mode
void changeTitleMode(tMode mode)
{
	tMode oldMode;

	widgDelete(psWScreen, FRONTEND_BACKDROP);		// delete backdrop.

	oldMode = titleMode;							// store old mode
	titleMode = mode;								// set new mode

	switch (mode)
	{
	case CAMPAIGNS:
		startCampaignSelector();
		break;
	case SINGLE:
		startSinglePlayerMenu();
		break;
	case GAME:
		startGameOptionsMenu();
		break;
	case GRAPHICS_OPTIONS:
		startGraphicsOptionsMenu();
		break;
	case AUDIO_OPTIONS:
		startAudioOptionsMenu();
		break;
	case VIDEO_OPTIONS:
		startVideoOptionsMenu();
		break;
	case MOUSE_OPTIONS:
		startMouseOptionsMenu();
		break;
	case TUTORIAL:
		startTutorialMenu();
		break;
	case OPTIONS:
		startOptionsMenu();
		break;
	case TITLE:
		startTitleMenu();
		break;
	case CREDITS:
		startCreditsScreen();
		break;
	case MULTI:
		startMultiPlayerMenu();		// goto multiplayer menu
		break;
	case PROTOCOL:
		startConnectionScreen();
		break;
	case MULTIOPTION:
		startMultiOptions(oldMode == MULTILIMIT);
		break;
	case GAMEFIND:
		startGameFind();
		break;
	case MULTILIMIT:
		startLimitScreen();
		break;
	case KEYMAP:
		startKeyMapEditor(true);
		break;
	case STARTGAME:
	case QUIT:
	case LOADSAVEGAME:
		bLimiterLoaded = false;
	case SHOWINTRO:
		break;
	default:
		debug(LOG_FATAL, "Unknown title mode requested");
		abort();
		break;
	}

	return;
}
