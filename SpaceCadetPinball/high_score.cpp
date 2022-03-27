#include "pch.h"
#include "high_score.h"

#include "memory.h"
#include "options.h"

#include "livearea.h"

#include <psp2/ime_dialog.h>
#include <psp2/apputil.h>
#include <psp2/types.h>
#include <psp2/sysmodule.h> 
#include <codecvt>
#include <cstring>
#ifdef NETDEBUG
#include <debugnet.h>
#endif
#include "winmain.h"

#define DISPLAY_WIDTH			960
#define DISPLAY_HEIGHT			544
#define DISPLAY_STRIDE_IN_PIXELS	1024

static SceWChar16 _IME_Buffer[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static bool _HasInit = false;
static bool _JustGotWord = false;
static bool _AutoShowKeyboard = false;

void high_score::vita_done_input()
{
#ifdef NETDEBUG
	debugNetPrintf(DEBUG, "Stopping text input.\n");
#endif
	_JustGotWord = true;
}

int high_score::dlg_enter_name;
int high_score::dlg_score;
int high_score::dlg_position;
char high_score::default_name[32]{};
high_score_struct* high_score::dlg_hst;
bool high_score::ShowDialog = false;

int high_score::read(high_score_struct* table)
{
	char Buffer[20];

	int checkSum = 0;
	clear_table(table);
	for (auto position = 0; position < 5; ++position)
	{
		auto tablePtr = &table[position];

		snprintf(Buffer, sizeof Buffer, "%d", position);
		strcat(Buffer, ".Name");
		auto name = options::get_string(Buffer, "");
		strncpy(tablePtr->Name, name.c_str(), sizeof tablePtr->Name);

		snprintf(Buffer, sizeof Buffer, "%d", position);
		strcat(Buffer, ".Score");
		tablePtr->Score = options::get_int(Buffer, tablePtr->Score);

		for (int i = static_cast<int>(strlen(tablePtr->Name)); --i >= 0; checkSum += tablePtr->Name[i])
		{
		}
		checkSum += tablePtr->Score;
	}

	auto verification = options::get_int("Verification", 7);
	if (checkSum != verification)
		clear_table(table);
	return 0;
}

int high_score::write(high_score_struct* table)
{
	char Buffer[20];

	int checkSum = 0;
	for (auto position = 0; position < 5; ++position)
	{
		auto tablePtr = &table[position];

		snprintf(Buffer, sizeof Buffer, "%d", position);
		strcat(Buffer, ".Name");
		options::set_string(Buffer, tablePtr->Name);

		snprintf(Buffer, sizeof Buffer, "%d", position);
		strcat(Buffer, ".Score");
		options::set_int(Buffer, tablePtr->Score);

		for (int i = static_cast<int>(strlen(tablePtr->Name)); --i >= 0; checkSum += tablePtr->Name[i])
		{
		}
		checkSum += tablePtr->Score;
	}

	options::set_int("Verification", checkSum);
	return 0;
}

void high_score::clear_table(high_score_struct* table)
{
	for (int index = 5; index; --index)
	{
		table->Score = -999;
		table->Name[0] = 0;
		++table;
	}
}

int high_score::get_score_position(high_score_struct* table, int score)
{
	if (score <= 0)
		return -1;

	for (int position = 0; position < 5; position++)
	{
		if (table[position].Score < score)
			return position;
	}
	return -1;
}

int high_score::place_new_score_into(high_score_struct* table, int score, LPSTR scoreStr, int position)
{
	if (position >= 0)
	{
		if (position <= 4)
		{
			high_score_struct* tablePtr = table + 4;
			int index = 5 - position;
			do
			{
				--index;
				memcpy(tablePtr, &tablePtr[-1], sizeof(high_score_struct));
				--tablePtr;
			}
			while (index);
		}
		high_score_struct* posTable = &table[position];
		posTable->Score = score;
		if (strlen(scoreStr) >= 31)
			scoreStr[31] = 0;
		strncpy(posTable->Name, scoreStr, sizeof posTable->Name);
		posTable->Name[31] = 0;
	}
	return position;
}

void high_score::show_high_score_dialog(high_score_struct* table)
{
	dlg_enter_name = 0;
	dlg_score = 0;
	dlg_hst = table;
	ShowDialog = true;
}

void high_score::show_and_set_high_score_dialog(high_score_struct* table, int score, int pos, LPCSTR defaultName)
{
	dlg_position = pos;
	dlg_score = score;
	dlg_hst = table;
	dlg_enter_name = 1;
	strncpy(default_name, defaultName, sizeof default_name);
	ShowDialog = true;
	winmain::SetImguiEnabled(true);
	_AutoShowKeyboard = true;
}

void high_score::RenderHighScoreDialog()
{
	if (ShowDialog == true)
	{
		ShowDialog = false;
		if (dlg_position == -1)
		{
			dlg_enter_name = 0;
			return;
		}
		ImGui::OpenPopup("High Scores");
	}

	bool unused_open = true;
	if (ImGui::BeginPopupModal("High Scores", &unused_open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::BeginTable("table1", 3, 0))
		{
			char buf[36];
			ImGui::TableSetupColumn("Rank");
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Score");
			ImGui::TableHeadersRow();

			high_score_struct* tablePtr = dlg_hst;
			for (int row = 0; row < 5; row++)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				snprintf(buf, sizeof buf, "%d", row);
				ImGui::TextUnformatted(buf);

				auto score = tablePtr->Score;
				ImGui::TableNextColumn();
				if (dlg_enter_name == 1 && dlg_position == row)
				{
					score = dlg_score;
					ImGui::PushItemWidth(400);
					ImGui::InputText("###name_input", default_name, IM_ARRAYSIZE(default_name), ImGuiInputTextFlags_AutoSelectAll);

					auto &io = ImGui::GetIO();
					auto active = ImGui::GetActiveID();
					auto last = ImGui::GetID("###name_input");
					
				#ifdef NETDEBUG
					debugNetPrintf(DEBUG, "Active ID: %u, Last: %u; Equal: %d\n", active, last, active == last);
				#endif
					
					if (_AutoShowKeyboard)
					{
					#ifdef NETDEBUG
						debugNetPrintf(DEBUG, "Try to show keyboard automatically!\n");
					#endif
						ImGui::SetKeyboardFocusHere();
						
						io = ImGui::GetIO();
						active = ImGui::GetActiveID();
						
					#ifdef NETDEBUG
						debugNetPrintf(DEBUG, "Active ID: %u, Last: %u; Equal: %d\n", active, last, active == last);
						debugNetPrintf(DEBUG, "Do i have your attention now? %d\n", io.WantCaptureKeyboard);
					#endif
					}
					
					if (io.WantCaptureKeyboard && active != 0 && active == last)
					{
						if(_JustGotWord)
						{
							_JustGotWord = false;
							//SDL_StopTextInput();
							ImGui::SetFocusID(ImGui::GetID("Rank"), ImGui::GetCurrentWindow());
							ImGui::SetActiveID(ImGui::GetID("Rank"), ImGui::GetCurrentWindow());
							_HasInit = false;
						}
						else
						{
						#ifdef NETDEBUG
							debugNetPrintf(DEBUG, "Active ID: %u, Last: %u; Equal: %d\n", active, last, active == last);
						#endif
						
							if(_HasInit == false)
							{
							#ifdef NETDEBUG
								debugNetPrintf(DEBUG, "Starting text input.\n");
							#endif
								vita_start_text_input("Enter your name", default_name, SCE_IME_DIALOG_MAX_TEXT_LENGTH);
								_HasInit = true;
								_AutoShowKeyboard = false;
							}
						}
					}
				}
				else
				{
					ImGui::TextUnformatted(tablePtr->Name);
				}

				ImGui::TableNextColumn();
				score::string_format(score, buf);
				ImGui::TextUnformatted(buf);

				tablePtr++;
			}
			ImGui::EndTable();
		}
		ImGui::Separator();

		if (ImGui::Button("Ok"))
		{
			if (dlg_enter_name)
			{
				default_name[31] = 0;
				place_new_score_into(dlg_hst, dlg_score, default_name, dlg_position);
				high_score::write(dlg_hst);
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();

		ImGui::SameLine();
		if (ImGui::Button("Clear"))
			ImGui::OpenPopup("Confirm");
		if (ImGui::BeginPopupModal("Confirm", nullptr, ImGuiWindowFlags_MenuBar))
		{
			ImGui::TextUnformatted(pinball::get_rc_string(40, 0));
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				clear_table(dlg_hst);
				ImGui::CloseCurrentPopup();
				high_score::write(dlg_hst);
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::EndPopup();
	}
}

void high_score::update_live_area(high_score_struct* table)
{
	SDL_CreateThread(update_live_area_thread, "update_live_area_thread", table);
}

int high_score::update_live_area_thread(void *tablePtr)
{
	high_score_struct* table = reinterpret_cast<high_score_struct*>(tablePtr);
	
	int result = sceSysmoduleLoadModule(SCE_SYSMODULE_LIVEAREA);
	
#ifdef NETDEBUG
	debugNetPrintf(DEBUG, "Is libLiveArea loaded? %X\n", result);
#endif
	
	char bufferTitle [300];
	
	const char* frameXmlStrTitle = 
    "<frame id='frame1' rev='1'>" 
        "<liveitem id='1'><text><str size=\"40\" color=\"#ffffff\" shadow=\"on\">Highscore</str></text></liveitem>" 
    "</frame>";
	
	sprintf(bufferTitle, frameXmlStrTitle);
	
#ifdef NETDEBUG
	debugNetPrintf(DEBUG, "Frame: %s\n", bufferTitle);
#endif
	
	result = sceLiveAreaUpdateFrameSync(
		SCE_LIVEAREA_FORMAT_VER_CURRENT,
		bufferTitle, 
		strlen(bufferTitle),
		"app0:my_livearea_update",
		SCE_LIVEAREA_FLAG_NONE);
	
#ifdef NETDEBUG
	debugNetPrintf(DEBUG, "Is Live Area updated? %X\n", result);
#endif
	
	const char* frameXmlStr = 
    "<frame id='frame%d' rev='1'>" 
        "<liveitem id='1'><text align=\"left\" word-scroll=\"on\" margin-left=\"20\"><str size=\"30\" color=\"#ffffff\">%d. %s: %s</str></text></liveitem>" 
    "</frame>";
	
	for (auto position = 0; position < 5; ++position)
	{
		char buffer [300];
		char score [30];
		char name [30];
		
		auto tablePtr = &table[position];
		
		sprintf(score, "%d", tablePtr->Score);
		sprintf(name, "%s", tablePtr->Name);
		sprintf(buffer, frameXmlStr, position + 2, position + 1, name, score);
		
	#ifdef NETDEBUG
		debugNetPrintf(DEBUG, "Frame: %s\n", buffer);
	#endif
		
		result = sceLiveAreaUpdateFrameSync(
			SCE_LIVEAREA_FORMAT_VER_CURRENT,
			buffer, 
			strlen(buffer),
			"app0:my_livearea_update",
			SCE_LIVEAREA_FLAG_NONE);
			
	#ifdef NETDEBUG
		debugNetPrintf(DEBUG, "Is Live Area updated? %X\n", result);
	#endif
	}
	
	char bufferFramelogo [300];
	
	const char* frameLogoXmlStr = 
    "<frame id='frame7' rev='1'>" 
        "<liveitem id='1'><image>text_frame7.png</image></liveitem>" 
    "</frame>";
	
	sprintf(bufferFramelogo, frameLogoXmlStr);
	
#ifdef NETDEBUG
	debugNetPrintf(DEBUG, "Frame: %s\n", bufferFramelogo);
#endif
	
	result = sceLiveAreaUpdateFrameSync(
		SCE_LIVEAREA_FORMAT_VER_CURRENT,
		bufferFramelogo, 
		strlen(bufferFramelogo),
		"app0:sce_sys/livearea/contents",
		SCE_LIVEAREA_FLAG_NONE);
	
#ifdef NETDEBUG
	debugNetPrintf(DEBUG, "Is Live Area updated? %X\n", result);
#endif

	result = sceSysmoduleUnloadModule(SCE_SYSMODULE_LIVEAREA);
	
#ifdef NETDEBUG
	debugNetPrintf(DEBUG, "Is libLiveArea unloaded? %X\n", result);
#endif
}

void high_score::vita_start_text_input(const char *guide_text, const char *initial_text, int max_length)
{
	if (vita_keyboard_get(guide_text, initial_text, max_length, _IME_Buffer)) {
		SDL_CreateThread(vita_input_thread, "vita_input_thread", (void *)_IME_Buffer);
	}
}

int high_score::vita_keyboard_get(const char *guide_text, const char *initial_text, int max_len, SceWChar16 *buf)
{
	SceWChar16 title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
	SceWChar16 text[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
	SceInt32 res;

	SDL_memset(&title, 0, sizeof(title));
	SDL_memset(&text, 0, sizeof(text));
	utf8_to_utf16((const uint8_t *)guide_text, title);
	utf8_to_utf16((const uint8_t *)initial_text, text);

	SceImeDialogParam param;
	sceImeDialogParamInit(&param);

	param.supportedLanguages = 0;
	param.languagesForced = SCE_FALSE;
	param.type = SCE_IME_TYPE_DEFAULT;
	param.option = 0;
	param.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_WITH_CLEAR;
	param.maxTextLength = max_len;

	param.title = title;
	param.initialText = text;
	param.inputTextBuffer = buf;

	res = sceImeDialogInit(&param);
	if (res < 0) {
		return 0;
	}

	return 1;
}

int high_score::vita_input_thread(void *ime_buffer)
{
	while (1) {
		// update IME status. Terminate, if finished
		SceCommonDialogStatus dialogStatus = sceImeDialogGetStatus();
		if (dialogStatus == SCE_COMMON_DIALOG_STATUS_FINISHED) {
			uint8_t utf8_buffer[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
			SceImeDialogResult result;

			SDL_memset(&result, 0, sizeof(SceImeDialogResult));
			sceImeDialogGetResult(&result);
			
			if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
			{
				// Convert UTF16 to UTF8
				utf16_to_utf8((SceWChar16 *)ime_buffer, utf8_buffer);

				// send sdl event
				SDL_Event event;
				event.text.type = SDL_TEXTINPUT;
				SDL_utf8strlcpy(event.text.text, (const char *)utf8_buffer, SDL_arraysize(event.text.text));
				SDL_PushEvent(&event);
			}
			
			sceImeDialogTerm();
			high_score::vita_done_input();
			break;
		}
	}
	return 0;
}

void high_score::utf16_to_utf8(const uint16_t *src, uint8_t *dst)
{
	for (int i = 0; src[i]; i++) {
		if ((src[i] & 0xFF80) == 0) {
			*(dst++) = src[i] & 0xFF;
		} else if ((src[i] & 0xF800) == 0) {
			*(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		} else if ((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
			*(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
			*(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
			*(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
			*(dst++) = (src[i + 1] & 0x3F) | 0x80;
			i += 1;
		} else {
			*(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
			*(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		}
	}

	*dst = '\0';
}

void high_score::utf8_to_utf16(const uint8_t *src, uint16_t *dst)
{
	for (int i = 0; src[i];) {
		if ((src[i] & 0xE0) == 0xE0) {
			*(dst++) = ((src[i] & 0x0F) << 12) | ((src[i + 1] & 0x3F) << 6) | (src[i + 2] & 0x3F);
			i += 3;
		} else if ((src[i] & 0xC0) == 0xC0) {
			*(dst++) = ((src[i] & 0x1F) << 6) | (src[i + 1] & 0x3F);
			i += 2;
		} else {
			*(dst++) = src[i];
			i += 1;
		}
	}

	*dst = '\0';
}