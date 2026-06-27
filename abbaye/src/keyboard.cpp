#include "keyboard.h"
#include <string.h>

extern Uint32 SDL_GetTicks(void);

struct KMapEntry {
    const char *key;
    int val;
};

static const KMapEntry kmap_entries[] = {
    {"SHIFT", 0x002},
    {"CTRL", 0x004},
    {"BREAK", 0x010},
    {"GRP", 0x040},
    {"^", 0x101},
    {"HOME", 0x102},
    {"SPACE", 0x104},
    {"RETURN", 0x108},
    {"C", 0x110},
    {"A", 0x120},
    {"Q", 0x140},
    {"1", 0x180},
    {"LOCK", 0x201},
    {"Z", 0x204},
    {"]", 0x208},
    {"V", 0x210},
    {"S", 0x220},
    {"W", 0x240},
    {"2", 0x280},
    {"DEL", 0x301},
    {"ESC", 0x304},
    {"[", 0x308},
    {"B", 0x310},
    {"D", 0x320},
    {"E", 0x340},
    {"3", 0x380},
    {"→", 0x404},
    {"\\", 0x408},
    {"N", 0x410},
    {"F", 0x420},
    {"R", 0x440},
    {"4", 0x480},
    {"F1", 0x502},
    {"←", 0x504},
    {"M", 0x510},
    {"G", 0x520},
    {"T", 0x540},
    {"5", 0x580},
    {"F2", 0x602},
    {"@", 0x604},
    {"X", 0x608},
    {",", 0x610},
    {"H", 0x620},
    {"Y", 0x640},
    {"6", 0x680},
    {"F3", 0x702},
    {"↑", 0x704},
    {"P", 0x708},
    {".", 0x710},
    {"J", 0x720},
    {"U", 0x740},
    {"7", 0x780},
    {"F4", 0x802},
    {"↓", 0x804},
    {":", 0x808},
    {"/", 0x810},
    {"K", 0x820},
    {"I", 0x840},
    {"8", 0x880},
    {"F5", 0x902},
    {"-", 0x904},
    {"0", 0x908},
    {";", 0x910},
    {"L", 0x920},
    {"O", 0x940},
    {"9", 0x980}
};

static int kmap_lookup(const char *key) {
    int num_entries = sizeof(kmap_entries) / sizeof(kmap_entries[0]);
    for (int i = 0; i < num_entries; i++) {
        if (strcmp(kmap_entries[i].key, key) == 0) {
            return kmap_entries[i].val;
        }
    }
    return 0; // return 0 to match old map behavior where key not found returned 0
}

void CKeyboard::setMatrix(const char* code) {
    int kmx = kmap_lookup(code);
    if (kmx != 0) {
        keyMatrix[kmx>>8] &= (~kmx);
    }
}

CKeyboard::CKeyboard()
{
	BuildKeyHashTab();
	clearMatrix();
}

void CKeyboard::handle_event(SDL_Event event)
{
	SDL_Keycode sym = event.key.keysym.sym;
	if (event.type == SDL_KEYDOWN) ProcessKeyDown(sym);
	else if (event.type == SDL_KEYUP) ProcessKeyUp(sym);
}
/**
 * Build Keyboard Hashing Table
 * Call this once at the initialization phase.
 */
void CKeyboard::BuildKeyHashTab(void)
{
	int i;
	static int hashPos[256] = { 0 };

	for (i = 0; spcKeyMap[i].keyMatIdx != -1; i++)
	{
		int index = spcKeyMap[i].sym % 256;

		KeyHashTab[index].numEntry++;
		hashPos[index]++;
	}

	for (i = 0; i < 256; i++)
	{
		KeyHashTab[i].keys
			= (TKeyMap *) malloc(sizeof(TKeyMap) * hashPos[i]);
	}

	for (i = 0; spcKeyMap[i].keyMatIdx != -1; i++)
	{
		int index = spcKeyMap[i].sym % 256;

		hashPos[index]--;
		if (hashPos[index] < 0)
			printf("Fatal: out of range in %s:BuildKeyHashTab().\n",
			__FILE__), exit(1);
		KeyHashTab[index].keys[hashPos[index]]
			= spcKeyMap[i];
	}
}

/**
 * SDL Key-Down processing. Search Hash table and set appropriate keyboard matrix.
 * @param sym SDL key symbol
 */
void CKeyboard::ProcessKeyDown(SDL_Keycode sym)
{
	int i;
	int index = sym % 256;
    //printf(">%d-%c\n", sym);
	for (i = 0; i < KeyHashTab[index].numEntry; i++)
	{
		if (KeyHashTab[index].keys[i].sym == sym)
		{
			keyMatrix[KeyHashTab[index].keys[i].keyMatIdx]
				&= ~(KeyHashTab[index].keys[i].keyMask);
#ifdef DEBUG_MODE
			printf("%08x [%s] key down\n",
				KeyHashTab[index].keys[i].sym, KeyHashTab[index].keys[i].keyName);
#endif
			break;
		}
	}
}

/**
 * SDL Key-Up processing. Search Hash table and set appropriate keyboard matrix.
 * @param sym SDL key symbol
 */
void CKeyboard::ProcessKeyUp(SDL_Keycode sym)
{
	int i;
	int index = sym % 256;
    //printf("<%d-%c\n", sym);
	for (i = 0; i < KeyHashTab[index].numEntry; i++)
	{
		if (KeyHashTab[index].keys[i].sym == sym)
		{
			keyMatrix[KeyHashTab[index].keys[i].keyMatIdx]
				|= (KeyHashTab[index].keys[i].keyMask);
#ifdef DEBUG_MODE
			printf("%08x [%s] key up\n",
				KeyHashTab[index].keys[i].sym, KeyHashTab[index].keys[i].keyName);
#endif
			break;
		}
	}
}

void CKeyboard::KeyPress(char *str)
{
	printf("%s\n", str);
}

void CKeyboard::KeyPress(char *code, bool shift, bool ctrl, bool grp, bool lock, bool single)
{
	if (!code)
		clearMatrix();
	else 
	{
		if (kmap_lookup(code) != 0) 
			setMatrix(code);
		if (shift) 
			setMatrix("SHIFT");
		if (ctrl) 
			setMatrix("CTRL");
		if (grp) 
			setMatrix("GRP");
		if (lock) 
			setMatrix("LOCK");
		pressed = true;
		if (single)
			repeat = 5;
		else
			repeat = -1;
	}
}


/**
 * SDL Key-Down processing. Special Keys only for Emulator
 * @param sym SDL key symbol
 */
void CKeyboard::ProcessSpecialKey(SDL_Keysym ksym)
{
// 	int index = ksym.sym % 256;
// 	int retVal;
// 	FILE *rfp_save;
// 	FILE *wfp_save;
// 	char *str;
//     int r = 0;
//     int t;
// 	switch (ksym.sym)
// 	{
// 	case SDLK_SCROLLLOCK: // turbo mode
// 		TURBO = (TURBO ? 0: 10);  // toggle
// 		if (!TURBO) t = timeGetTime();
// 		printf("turbo %s\n", (TURBO)? "on":"off");
// 		break;
//     // case SDLK_INSERT:
//     //     if (m_uiStr == NULL)
//     //     {
//     //         // str = GetClipboardText(64000);
//     //         // if (str)
//     //         // {
//     //         //     printf("clipboard:%s\n",str);
//     //         //     UI_Paste(str);
//     //         // }
//     //     }
//     //     break;
// 	case SDLK_PRINTSCREEN:
//     case SDLK_SYSREQ:
//         // retVal = SetClipboardText((const char *)spcsys.prt.bufs);
// 		// PRT_Save((const char *)spcsys.prt.bufs, spcsys.prt.length);
//         // printf("Printer Output.(%d)\n%s\n", retVal, spcsys.prt.bufs);
//         break;

// //     case SDLK_F6:
// // 	    if (ksym.mod & KMOD_ALT)
// //         {
// //             snapshots_menu();
// //         } else {
// //             help_menu();
// //         }
// //         break;
// //     case SDLK_F7:
// // 	    if (ksym.mod & KMOD_ALT)
// //         {
// //             settings_menu();
// //         } else {
// //             taps_menu();
// //         }
// //         break;
// // 	case SDLK_F8: // PLAY button
// // 	    printf("SDLK_F8 pressed\n");
// // 		if (spconf.rfp != NULL)
// // 			FCLOSE(spconf.rfp);
// // 		if (ksym.mod & KMOD_ALT) // STOP button
// //         {
// //             spcsys.cas.button = CAS_STOP;
// //             spcsys.cas.motor = 0;
// //             printf("stop button\n");
// //         }
// //         else // PLAY button
// //         {
// // 			if (spconf.wfp != NULL)
// // 			{
// // 				spcsys.cas.button = CAS_REC;
// // 				spcsys.cas.motor = 1;
// // 				printf("rec button pushed\n");
// //  			}
// // 			else if (OpenTapeFile() < 0)
// //                 break;
// // 			else {
// // 				spcsys.cas.button = CAS_PLAY;
// // 				spcsys.cas.motor = 1;
// // 			}
// //             spcsys.cas.lastTime = 0;
// //             ResetCassette(&spcsys.cas);
// //             printf("play button pushed\n");
// //         }
// // 		break;
// // 	case SDLK_F9: // FDD management
// // 		if (ksym.mod & KMOD_ALT)
// // 		{
// // 			VDP_Save();
// // 		}
// // 		else
// // 		{
// // 			printf("Floppy Disk management\n");
// // 			floppy_disk_menu();
// // 		}
// // 		break;
// // 	case SDLK_F10: // Quit
// //         SDL_Quit();
// //         exit(0);
// //         break;
// // 	case SDLK_PAGEUP: // Image Save        puts("q          : Exit Z80 emulation");
// // 		save_s1sfile();
// // 		printf("Image Save\n");
// // 		break;

// // 	case SDLK_PAGEDOWN: // Image Load
// // 		load_s1sfile();
// // //		r = 1;
// // //		spcsys.tick = SDL_GetTicks();
// // //		spcsim.baseTick = SDL_GetTicks();
// // //		spcsim.prevTick = spcsim.baseTick;
// // //		spcsys.cas.button = CAS_STOP;
// // //		spcsys.cas.motor = 0;
// // //		if (spcsys.GMODE & 0x08)
// // //		{
// // //			SetMC6847Mode(SET_GRAPHIC, spcsys.GMODE);
// // ////			UpdateMC6847Gr(MC6847_UPDATEALL);
// // //		}
// // //		else
// // //		{
// // //			SetMC6847Mode(SET_TEXTMODE, spcsys.GMODE);
// // ////			UpdateMC6847Text(MC6847_UPDATEALL);
// // //		}
// // //		printf("Image Load\n");
// // 		break;

// 	case SDLK_F11: // PC Keyboard mode, thanks to zanny
// 	    if (ksym.mod & KMOD_ALT)
//         {
//             // SetPCKeyboard(spcsys.RAM);
//         }
//         else
//         {
//             ToggleFullScreen();
//         }
// 		break;
//     case SDLK_KP_5:
//         spconf.debug = 1;
//         break;
// 	case SDLK_F12: // Reset
//         if (ksym.mod & KMOD_ALT)
//         {
//             spcsys.IPL_SW = 1;
//             printf("Reset with IPL_SW\n");
//         }
//         else
//         {
//             spcsys.IPL_SW = 0;
//             printf("Reset (keeping tape pos.)\n");
//         }
// 		// load_rom();
// 		// InitIOSpace();
// 		SndQueueInit();
// //		SetMC6847Mode(SET_TEXTMODE, 0);
//         spcsys.Z80R.cyc = I_PERIOD;
//         spcsys.Z80R.pc = 0x00;
//         //spcsys.Z80R.SP.W = 0xf000;
//         spcsys.IPLK = 1;
//         spcsys.IPL_SW = 1;
//         // z80mem = spcsys.ROM;
// 		spcsim.baseTick = SDL_GetTicks();
// 		spcsim.prevTick = spcsim.baseTick;
// 		spcsys.intrTime = INTR_PERIOD;
// 		spcsys.tick = 0;
//         spcsys.refrTimer = 0;	// timer for screen refresh
//         spcsys.refrSet = spconf.frameRate;	// init value for screen refresh timer
// 		z80_init(&spcsys.Z80R);
// 		break;
// 	}
}
