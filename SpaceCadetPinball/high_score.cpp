#include "pch.h"
#include "high_score.h"

#include "memory.h"
#include "options.h"

#if VITA
#include <psp2/ime_dialog.h>
#include <psp2/apputil.h>
#include <codecvt>
#ifndef NDEBUG
#include <debugnet.h>
#endif
#include "winmain.h"

#define DISPLAY_WIDTH			960
#define DISPLAY_HEIGHT			544
#define DISPLAY_STRIDE_IN_PIXELS	1024

static SceImeDialogParam _InternalIMEParams = {0};
static char16_t _IMEInput[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1] = {0};
static bool _HasInit = false;
static bool _JustGotWord = false;

static void _Vita_ShowIME()
{
	if(_HasInit == false)
	{
		debugNetPrintf(DEBUG, "Starting text input.\n");
		SDL_StartTextInput();
		_HasInit = true;
	}
}

void high_score::vita_done_input()
{
	debugNetPrintf(DEBUG, "vita_done_input\n");
	SDL_StopTextInput();
	_JustGotWord = true;
}
// TODO: Do I need to sceSysmoduleLoadModule(SCE_SYSMODULE_IME); 
// Or does SDL itself already do this for me by design? (It shows dialogs, but maybe not IMEs?)
#endif

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
				#ifdef VITA
					ImGui::PushItemWidth(400);
				#else
					ImGui::PushItemWidth(200);
				#endif
					ImGui::InputText("###name_input", default_name, IM_ARRAYSIZE(default_name));

				#ifdef VITA
					auto &io = ImGui::GetIO();
					auto active = ImGui::GetActiveID();
					auto last = ImGui::GetID("###name_input");
					if (io.WantCaptureKeyboard && active != 0 && active == last)
					{
						if(_JustGotWord)
						{
							_JustGotWord = false;
							SDL_StopTextInput();
							ImGui::SetFocusID(ImGui::GetID("Rank"), ImGui::GetCurrentWindow());
							ImGui::SetActiveID(ImGui::GetID("Rank"), ImGui::GetCurrentWindow());
							_HasInit = false;
						}
						else
						{
							debugNetPrintf(DEBUG, "Please capture keyboard for %u. Last: %u; Equal: %d\n", active, last, active == last);
						#ifdef VITA
							_Vita_ShowIME();
						#endif
						}
					}
				#endif
					// END VITA HACK
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
