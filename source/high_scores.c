#include "high_scores.h"
#include "common.h"

high_score_entry high_scores[5];

void load_save() {
	// check for the magic
	if (sram_mem[0] != 'G' || sram_mem[1] != 'a' || sram_mem[2] != 'b' || sram_mem[3] != 'e') {
		sram_mem[0] = 'G';
		sram_mem[1] = 'a';
		sram_mem[2] = 'b';
		sram_mem[3] = 'e';

		memset(high_scores, 0, sizeof(high_scores));
		memcpy(sram_mem + sizeof(u32), high_scores, sizeof(high_scores));
		return;
	}

	// load from sram
	vu8* v_sram = (vu8*)sram_mem;
	u8* high_scores_ptr = (u8*)high_scores;
	for (int i = 0; i < sizeof(high_scores); i++) {
		high_scores_ptr[i] = v_sram[i+4];
	}
}

bool is_high_score(int score) {
	for (int i = 0; i < sizeof(high_scores) / sizeof(high_score_entry); i++) {
		if (score > high_scores[i].score) {
			return true;
		}
	}
	return false;
}

// add a high score and save it to sram
void insert_high_score(high_score_entry* new_entry) {
	for (int i = 0; i < sizeof(high_scores) / sizeof(high_score_entry); i++) {
		if (new_entry->score > high_scores[i].score) {
			memmove(&high_scores[i+1], &high_scores[i], ((sizeof(high_scores) / sizeof(high_score_entry) - 1 - i) * sizeof(high_score_entry)));
			memcpy(&high_scores[i], new_entry, sizeof(high_score_entry));
			break;
		}
	}

	vu8* v_sram = (vu8*)sram_mem;
	u8* high_scores_ptr = (u8*)high_scores;
	for (int i = 0; i < sizeof(high_scores); i++) {
		v_sram[i+4] = high_scores_ptr[i];
	}
}

// draw the high scores
void draw_high_scores() {
	char cur_score[6];
	for (int i = 0; i < sizeof(high_scores) / sizeof(high_score_entry); i++) {
		if (high_scores[i].name[0] == 0) {
			draw_string(3, 4 + (i * 3), "_________");
		} else {
			draw_string(3, 4 + (i * 3), high_scores[i].name);
		}
		itoa(high_scores[i].score, cur_score, 10);
		draw_string(23, 4 + (i * 3), cur_score);
	}
}
