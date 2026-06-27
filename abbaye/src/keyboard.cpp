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

static const KeyMap spcKeyMap[] = {
	{ SDLK_LSHIFT,			0, 0x02, "SHIFT"   },
	{ SDLK_RSHIFT,			0, 0x02, "SHIFT"   },
	{ SDLK_LCTRL,			0, 0x04, "CTRL"    },
	{ SDLK_RCTRL,			0, 0x04, "CTRL"    },
	{ SDLK_PAUSE,			0, 0x10, "BREAK"   },
	{ SDLK_RALT,			0, 0x40, "GRAPH"   },
	{ SDLK_LGUI,			0, 0x40, "GRAPH"   },
	{ SDLK_RGUI,			0, 0x40, "GRAPH"   },

	{ SDLK_EQUALS,			1, 0x01, "^"       },
	{ SDLK_HOME,			1, 0x02, "HOME"    },
	{ SDLK_SPACE,			1, 0x04, "SPACE"   },
	{ SDLK_RETURN,			1, 0x08, "RETURN"  },
	{ SDLK_c,				1, 0x10, "C"       },
	{ SDLK_a,				1, 0x20, "A"       },
	{ SDLK_q,				1, 0x40, "Q"       },
	{ SDLK_1,				1, 0x80, "1"       },

	{ SDLK_CAPSLOCK,		2, 0x01, "LOCK"    },
	{ SDLK_z,				2, 0x04, "Z"       },
	{ SDLK_RIGHTBRACKET,	2, 0x08, "]"       },
	{ SDLK_v,				2, 0x10, "V"       },
	{ SDLK_s,				2, 0x20, "S"       },
	{ SDLK_w,				2, 0x40, "W"       },
	{ SDLK_2,				2, 0x80, "2"       },

	{ SDLK_BACKSPACE,		3, 0x01, "DEL"     },
	{ SDLK_ESCAPE,			3, 0x04, "ESC"     },
	{ SDLK_LEFTBRACKET,		3, 0x08, "["       },
	{ SDLK_b,				3, 0x10, "B"       },
	{ SDLK_d,				3, 0x20, "D"       },
	{ SDLK_e,				3, 0x40, "E"       },
	{ SDLK_3,				3, 0x80, "3"       },

	{ SDLK_RIGHT,			4, 0x04, "RIGHT"   },
	{ SDLK_BACKSLASH,		4, 0x08, "\\"      },
	{ SDLK_n,				4, 0x10, "N"       },
	{ SDLK_f,				4, 0x20, "F"       },
	{ SDLK_r,				4, 0x40, "R"       },
	{ SDLK_4,				4, 0x80, "4"       },

	{ SDLK_F1,				5, 0x02, "F1"      },
	{ SDLK_LEFT,			5, 0x04, "LEFT"    },
	{ SDLK_m,				5, 0x10, "M"       },
	{ SDLK_g,				5, 0x20, "G"       },
	{ SDLK_t,				5, 0x40, "T"       },
	{ SDLK_5,				5, 0x80, "5"       },

	{ SDLK_F2,				6, 0x02, "F2"      },
	{ SDLK_AT,				6, 0x04, "@"       },
	{ SDLK_x,				6, 0x08, "X"       },
	{ SDLK_COMMA,			6, 0x10, ","       },
	{ SDLK_h,				6, 0x20, "H"       },
	{ SDLK_y,				6, 0x40, "Y"       },
	{ SDLK_6,				6, 0x80, "6"       },

	{ SDLK_F3,				7, 0x02, "F3"      },
	{ SDLK_UP,				7, 0x04, "UP"      },
	{ SDLK_p,				7, 0x08, "P"       },
	{ SDLK_PERIOD,			7, 0x10, "."       },
	{ SDLK_j,				7, 0x20, "J"       },
	{ SDLK_u,				7, 0x40, "U"       },
	{ SDLK_7,				7, 0x80, "7"       },

	{ SDLK_F4,				8, 0x02, "F4"      },
	{ SDLK_DOWN,			8, 0x04, "DOWN"    },
	{ SDLK_QUOTE,			8, 0x08, ":"       },
	{ SDLK_SLASH,			8, 0x10, "/"       },
	{ SDLK_k,				8, 0x20, "K"       },
	{ SDLK_i,				8, 0x40, "I"       },
	{ SDLK_8,				8, 0x80, "8"       },

	{ SDLK_F5,				9, 0x02, "F5"      },
	{ SDLK_MINUS,			9, 0x04, "-"       },
	{ SDLK_0,				9, 0x08, "0"       },
	{ SDLK_SEMICOLON,		9, 0x10, ";"       },
	{ SDLK_l,				9, 0x20, "L"       },
	{ SDLK_o,				9, 0x40, "O"       },
	{ SDLK_9,				9, 0x80, "9"       },

	{ SDLK_UNKNOWN,			-1, 0x00, "NULL"    }
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
        keyMatrix[kmx>>8] &= ~(kmx & 0xFF);
    }
}

CKeyboard::CKeyboard()
{
}

void CKeyboard::init()
{
	clearMatrix();
}

void CKeyboard::handle_event(SDL_Event event)
{
	SDL_Keycode sym = event.key.keysym.sym;
	if (event.type == SDL_KEYDOWN) ProcessKeyDown(sym);
	else if (event.type == SDL_KEYUP) ProcessKeyUp(sym);
}

void CKeyboard::ProcessKeyDown(SDL_Keycode sym)
{
	for (int i = 0; spcKeyMap[i].keyMatIdx != -1; i++)
	{
		if (spcKeyMap[i].sym == sym)
		{
			keyMatrix[spcKeyMap[i].keyMatIdx] &= ~(spcKeyMap[i].keyMask);
#ifdef DEBUG_MODE
			printf("%08x [%s] key down\n",
				spcKeyMap[i].sym, spcKeyMap[i].keyName);
#endif
			break;
		}
	}
}

void CKeyboard::ProcessKeyUp(SDL_Keycode sym)
{
	for (int i = 0; spcKeyMap[i].keyMatIdx != -1; i++)
	{
		if (spcKeyMap[i].sym == sym)
		{
			keyMatrix[spcKeyMap[i].keyMatIdx] |= spcKeyMap[i].keyMask;
#ifdef DEBUG_MODE
			printf("%08x [%s] key up\n",
				spcKeyMap[i].sym, spcKeyMap[i].keyName);
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
