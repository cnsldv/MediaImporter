#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <psp2/ctrl.h>
#include <psp2/apputil.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/net/http.h>
#include <psp2/io/stat.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>

#include "sqlite3.h"
#include "debugScreen.h"

#include "video.h"
#include "music.h"

const char *video_dirs[] = {"ux0:/video", NULL};;
const char *music_dirs[] = {"ux0:/music", NULL};;

static unsigned buttons[] = {
	SCE_CTRL_SELECT,
	SCE_CTRL_START,
	SCE_CTRL_UP,
	SCE_CTRL_RIGHT,
	SCE_CTRL_DOWN,
	SCE_CTRL_LEFT,
	SCE_CTRL_LTRIGGER,
	SCE_CTRL_RTRIGGER,
	SCE_CTRL_TRIANGLE,
	SCE_CTRL_CIRCLE,
	SCE_CTRL_CROSS,
	SCE_CTRL_SQUARE,
};

int get_key(void)
{
	static unsigned prev = 0;
	SceCtrlData pad;
	while (1) {
		memset(&pad, 0, sizeof(pad));
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned new = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (int i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i)
			if (new & buttons[i])
				return buttons[i];

		sceKernelDelayThread(1000); // 1ms
	}
}

void menu(void)
{
	psvDebugScreenClear(COLOR_BLACK);
	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("Media Importer by cnsldv\n");
	printf("========================\n");
	printf("\n");
	printf("Press CROSS to update the database.\n");
	printf("Press TRIANGLE to empty the database.\n");
	printf("Press CIRCLE to exit.\n");
	printf("\n");
}

int main(void)
{
	int ret;
	int key = 0;
	int idx;
	int done = 0;
	int madd = 0, mrem = 0;

	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));

	sceAppUtilInit(&init_param, &boot_param);
	psvDebugScreenInit();
	sceAppUtilMusicMount();

	while (!done) {
		menu();
		key = get_key();
		switch (key) {
		case SCE_CTRL_CROSS:
			menu();

			printf("Managing videos, please wait\n");
			idx = 0;
			while (video_dirs[idx]) {
				add_videos(video_dirs[idx]);
				idx++;
			}
			clean_videos();
			update_video_icons();

			printf("Managing music, please wait\n");
			idx = 0;
			while (music_dirs[idx]) {
				madd += add_music(music_dirs[idx]);
				idx++;
			}
			mrem = clean_music();
			if (madd > 0 || mrem > 0) {
				refresh_music_db();
			}
			psvDebugScreenSetFgColor(COLOR_GREEN);
			printf("Done.\n");
			psvDebugScreenSetFgColor(COLOR_WHITE);
			printf("Press a button to continue.\n");
			get_key();
			break;

		case SCE_CTRL_TRIANGLE:
			menu();
			printf("Removing all videos\n");
			empty_videos();
			printf("Removing all music\n");
			empty_music();
			psvDebugScreenSetFgColor(COLOR_GREEN);
			printf("Done.\n");
			psvDebugScreenSetFgColor(COLOR_WHITE);
			printf("Press a button to continue.\n");
			get_key();
			break;

		case SCE_CTRL_CIRCLE:
			done = 1;
			break;

		default:
			break;
		}
	}

	sceAppUtilMusicUmount();
	return 0;
}

