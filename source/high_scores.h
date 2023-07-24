#ifndef _HIGH_SCORES_H
#define _HIGH_SCORES_H

#include <tonc_core.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_input.h>
#include <tonc_video.h>
#include <string.h>
#include <stdlib.h>

typedef struct _high_score_entry {
	char name[10];
	int score;
} high_score_entry;

void load_save();
bool is_high_score(int score);
void insert_high_score(high_score_entry* new_entry);
void draw_high_scores();

#endif
