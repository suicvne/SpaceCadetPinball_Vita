#pragma once
#include "pinball.h"
#include <psp2/types.h>

struct high_score_struct
{
	char Name[32];
	int Score;
};

class high_score
{
public:
	static int read(high_score_struct* table);
	static int write(high_score_struct* table);
	static void clear_table(high_score_struct* table);
	static int get_score_position(high_score_struct* table, int score);
	static int place_new_score_into(high_score_struct* table, int score, LPSTR scoreStr, int position);

	static void show_high_score_dialog(high_score_struct* table);
	static void show_and_set_high_score_dialog(high_score_struct* table, int score, int pos, LPCSTR defaultName);
	static void RenderHighScoreDialog();
	static void vita_done_input();
	static void update_live_area(high_score_struct* table);
private :
	static int dlg_enter_name;
	static int dlg_score;
	static int dlg_position;
	static char default_name[32];
	static high_score_struct* dlg_hst;
	static bool ShowDialog;
	static void vita_start_text_input(const char *guide_text, const char *initial_text, int max_length);
	static int vita_keyboard_get(const char *guide_text, const char *initial_text, int max_len, SceWChar16 *buf);
	static int vita_input_thread(void *ime_buffer);
	static void utf16_to_utf8(const uint16_t *src, uint8_t *dst);
	static void utf8_to_utf16(const uint8_t *src, uint16_t *dst);
};
