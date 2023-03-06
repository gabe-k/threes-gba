
#include <tonc_core.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_input.h>
#include <tonc_video.h>
#include <string.h>

#include "sprites.h"

typedef enum _game_tile {
	empty = -1,
	tile_1,
	tile_2,
	tile_3,
	tile_6,
	tile_12,
	tile_24,
	tile_48,
	tile_96,
	tile_192,
	tile_384,
	tile_768,
	tile_1536,
	tile_3072,
	tile_6144
} game_tile;


char board[4][4];

game_tile random_game_tile() {
	return qran_range(tile_1, tile_6);
}

void oam_init(OBJ_ATTR *obj, uint count)
{
    u32 nn= count;
    u32 *dst= (u32*)obj;

    // Hide each object

    while(nn--)
    {
        *dst++= ATTR0_HIDE;
        *dst++= 0;
    }
    // init oam
    oam_copy(oam_mem, obj, count);
}


OBJ_ATTR test_objs[128];

typedef struct _POS2D {
	short x;
	short y;
} POS2D;

void draw_board() {
	for(int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			if ((u8)(board[x][y])
			 == (u8)empty) {
				obj_hide(&test_objs[(y * 4) + x]);
				continue;
			}

			obj_set_attr(&test_objs[(y * 4) + x], ATTR0_SQUARE | ATTR0_8BPP | ATTR0_REG, ATTR1_SIZE_32, ATTR2_PALBANK(0) | (board[x][y] * 32) | ATTR2_PRIO(1));
			obj_set_pos(&test_objs[(y * 4) + x], 50 + (x * 27), 32 + (y * 32));
		}
	}

	oam_copy(oam_mem, test_objs, 4 * 4);
}

void reset_board() {
	memset(board, 0xff, sizeof(board));

	for (int i = 0; i < 9; i++) {
		int cx, cy;
		do {
			cx = qran_range(0, 4);
			cy = qran_range(0, 4);
		} while ((u8)(board[cx][cy]) != (u8)empty);

		board[cx][cy] = random_game_tile();
	}
}

int main() {
	vu8* vram = MEM_VRAM;
	dma3_cpy(&tile_mem[4][0], spritesTiles, spritesTilesLen);

	vu8* pal = MEM_PAL;
	dma3_cpy(pal_obj_mem, spritesPal, spritesPalLen);

	oam_init(test_objs, 128);
	REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D;

	while(1) {
		vid_vsync();
		key_poll();
		if (key_hit(KEY_START)) {
			reset_board();
		}
		draw_board();
	}


	return 0;
}
