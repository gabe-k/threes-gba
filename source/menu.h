#ifndef _MENU_H
#define _MENU_H

typedef struct _menu_item {
	int x;
	int y;
	char* str;
} menu_item;

void draw_menu_items();
void menu_tick(int tick);
void initialize_menu();

#endif
