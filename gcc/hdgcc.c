/*
	HDGCC.C

	Non-portable functions specific to gcc and Unix/Linux
	using ncurses (or curses)
x*/


#include "heheader.h"
#include "hdheader.h"

#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

/* Function prototypes: */
void debug_breakhandler(void);
int debug_windowreadchar(int x, int y);
void debug_windowwritechar(int x, int y, int c);

/* From hegcc.c: */
void ConstrainCursor(void);
void rectangle(int left, int top, int right, int bottom);

/* If we're replacing main() in he.c */
#if defined (FRONT_END)
int he_main(int argc, char **argv);     /* from he.c */
void Banner(void);
#endif

struct old_window_data
{
	int top, bottom, left, right;
	int textcolor; long backcolor;
	int xpos, ypos;
	int currentline;
};

/* the four different screens for flipping between: */
WINDOW *game_screen = NULL, *debugger_screen = NULL,
	 *help_screen = NULL, *auxiliary_screen = NULL;
WINDOW *active_screen_window;

int fake_graphics_mode = false;
int monochrome_mode = false;

/* from hegcc.c */
extern int text_windowleft, text_windowtop, text_windowright, text_windowbottom;
extern int current_text_color, current_back_color,
	current_text_col, current_text_row;
extern WINDOW *current_window;


/* Keystroke data: */
#define UP_ARROW	11
#define	DOWN_ARROW      10
#define LEFT_ARROW	 8
#define RIGHT_ARROW	21
#define ENTER_KEY	13
#define ESCAPE_KEY	27
#define TAB_KEY		 9

#define DELETE_KEY     KEY_DC
#define BACKSPACE_KEY  KEY_BACKSPACE

#define ALT_KEY(a)	(0x1000+a)
#define SHIFT_KEY	12

#if defined (FRONT_END)
/* main

	Replaces the old main() in HE.C (when FRONT_END is #defined),
	since we need to process MS-DOS-specific switches for special
	debugger options.
*/

int main(int argc, char *argv[])
{
	unsigned int i, j;
	int arg = 1;

	while ((argc>1) && arg < argc && argv[arg][0]=='-')
	{
		for (i=1; i<strlen(argv[arg]); i++)
		{
			switch (tolower(argv[arg][i]))
			{
				case 'g':
					fake_graphics_mode = true;
					break;
					
				case 'm':
					monochrome_mode = true;
					break;

				default:
					goto PrintSwitches;
			}
		}
		arg++;
	}

	/* Now skim through and remove any switches, since
	   ParseCommandLine() in HEMISC.C doesn't know a switch
	   from a hole in the ground
	*/
	for (i=1; i<(unsigned)argc; i++)
	{
		if (argv[i][0]=='-')
		{
			for (j=i; j<(unsigned)argc-1; j++)
				argv[j] = argv[j+1];
			arg--, argc--, i--;
		}
	}

	if (arg==argc)
	{
PrintSwitches:
		Banner();
		printf("SWITCHES:\n");
		printf("\t-g  Fake graphics mode (no actual images)\n");
		printf("\t-m  Monochrome mode\n");
		exit(0);
	}

	return he_main(argc, argv);     /* start the engine */
}
#endif  /* if defined (FRONT_END) */


/* debug_breakhandler

	Called whenever CTRL-C or CTRL-BREAK is pressed in order
	not to break out of the program, but only to return control to the
	Debugger.
*/

void debug_breakhandler(void)
{
	debugger_interrupt = true;
}


/* debug_getevent

	Is called in various debugger states to read an event.  (This
	gcc version relays only keystrokes; a more complex port could
	report, for example, mouse events.)

	Most of these events should be self-explanatory.  The ALT key
	activates/deactivates the menubar.  A SHORTCUT_KEY action is
	a special keypress.  This is machine-specific:  for the PC,
	shortcut keys are ALT-key combinations.  (Other PC shortcuts
	are mapped to the function keys F1-F10.)
*/

void debug_getevent(void)
{
	int key = 0;

GetNewEvent:

	event.action = 0;
	event.object = 0;

	hugo_settextpos(current_text_col, current_text_row);
	key = hugo_getkey();

	/* Meta/Alt-key combinations 2-code values:  27 followed by keycode */
	if (key==27)
	{
		if (hugo_iskeywaiting())
		{
			key = hugo_getkey();
			if (key!=27) key = ALT_KEY(key);
		}
	}

	switch (key)
	{
//		case ALT_KEY(' '):
//		case (1):		// Ctrl+a
//		case ALT_KEY('m'):
		case ' ':
		{
			if (active_menu_heading==MENU_INACTIVE)
				event.action = OPEN_MENUBAR;
			else
				event.action = CANCEL;
			return;
		}
		case UP_ARROW:
			{event.action = MOVE; event.object = UP; break;}
		case DOWN_ARROW:
			{event.action = MOVE; event.object = DOWN; break;}
		case RIGHT_ARROW:
			{event.action = MOVE; event.object = RIGHT; break;}
		case LEFT_ARROW:
			{event.action = MOVE; event.object = LEFT; break;}
		case ENTER_KEY:
ReturnSelect:
			{event.action = SELECT; break;}
		case ESCAPE_KEY:
			{event.action = CANCEL; break;}
		case TAB_KEY:
			{event.action = TAB; break;}
		case DELETE_KEY:
		case 4:
		case 127:
			{event.action = DELETE; break;}

/* Additional navigation keys: */

		case KEY_PPAGE:
			{event.object = PAGEUP; 
ReturnMove:
			event.action = MOVE;
			return;}
		case KEY_NPAGE:
			{event.object = PAGEDOWN; goto ReturnMove;}
		case KEY_HOME:
			{event.object = HOME; goto ReturnMove;}
		case ALT_KEY('H'):
			{event.object = START; goto ReturnMove;}
		case KEY_END:
			{event.object = END; goto ReturnMove;}
		case ALT_KEY('E'):
			{event.object = FINISH; goto ReturnMove;}

/* Various function shortcut keys: */

		case ALT_KEY('X'):		// Alt+Shift+X
			{event.object = MENU_FILE + FILE_EXIT; 
			goto ReturnSelect;}
		case KEY_F(5):
		case 7:				// Ctrl+G
			{event.object = MENU_RUN + RUN_GO;
			goto ReturnSelect;}
		case KEY_F(5)+SHIFT_KEY:
		case 18:			// Ctrl+R
			{event.object = MENU_RUN + RUN_FINISH;
			goto ReturnSelect;}
		case KEY_F(3):
		case 19:			// Ctrl+S
			{event.object = MENU_DEBUG + DEBUG_SEARCH;
			goto ReturnSelect;}
		case KEY_F(9)+SHIFT_KEY:
		case 23:			// Ctrl+W
			{event.object = MENU_DEBUG + DEBUG_WATCH;
			goto ReturnSelect;}
		case KEY_F(7)+SHIFT_KEY:
		case 22:			// Ctrl+V
			{event.object = MENU_DEBUG + DEBUG_SET;
			goto ReturnSelect;}
		case KEY_F(9):
		case 2:				// Ctrl+B
			{event.object = MENU_DEBUG + DEBUG_BREAKPOINT;
			goto ReturnSelect;}
		case KEY_F(2):
		case 20:			// Ctrl+T
			{event.object = MENU_DEBUG + DEBUG_OBJTREE;
			goto ReturnSelect;}
		case KEY_F(2)+SHIFT_KEY:
		case 15:			// Ctrl+O
			{event.object = MENU_DEBUG + DEBUG_MOVEOBJ;
			goto ReturnSelect;}
		case KEY_F(8):
		case 6:				// Ctrl+F
			{event.object = MENU_RUN + RUN_STEP;
			goto ReturnSelect;}
		case KEY_F(10):
		case 5:				// Ctrl+E
			{event.object = MENU_RUN + RUN_STEPOVER;
			goto ReturnSelect;}
		case KEY_F(10)+SHIFT_KEY:
		case 14:			// Ctrl+N
			{event.object = MENU_RUN + RUN_SKIP;
			goto ReturnSelect;}
		case KEY_F(8)+SHIFT_KEY:
			{event.object = MENU_RUN + RUN_STEPBACK;
			goto ReturnSelect;}
		case ALT_KEY('k'):
#ifdef BEOS
		case 25:			// Ctrl+Y
#endif
			{event.object = MENU_HELP + HELP_KEYS;
			goto ReturnSelect;}
		case KEY_F(1)+SHIFT_KEY:
		case ALT_KEY('h'):
		case ALT_KEY('p'):
			{event.object = MENU_HELP + HELP_TOPIC;
			goto ReturnSelect;}
		case KEY_F(6):
		case ALT_KEY('w'):
			{event.action = SWITCH_WINDOW;
			return;}
		case KEY_F(1):
		case ALT_KEY('c'):
			{SearchHelp(context_help);
			goto GetNewEvent;}

		default:
		{
			if (key > ALT_KEY(0))
			{
				event.action = SHORTCUT_KEY;
				event.object = toupper(key - ALT_KEY(0));
			}
			else
			{
				event.action = KEYPRESS;
				event.object = toupper(key);
			}
		}
	}
}


/* debug_input

	Called by InputBox to get an input of maxlen characters.  Must
	return an empty string ("") if the input is cancelled by pressing
	Escape.
*/

char *debug_input(int x, int y, int maxlen, char *def)
{
	char blank_entry = 0;
	static char i[80];
	int pos, k;

	if (maxlen>76) maxlen = 76;

	/* print the default entry */
ReprintEntry:
	if (!blank_entry)
	{
		pos = strlen(def);
		strcpy(i, def);
	}
	else
		pos = 0;

	memset(i+pos, ' ', maxlen-pos);
	i[maxlen] = '\0';
	debug_settextpos(x, y);
	debug_print(i);

	event.action = 0;

	/* endless loop */
	while (true)
	{
		debug_settextpos(x+pos, y);

		switch (k = hugo_getkey())
		{
			case LEFT_ARROW:
		       	case BACKSPACE_KEY:
			case DELETE_KEY:
		        case (127):
		        case (4):	/* Ctrl+D */
			{
				if (pos>0)
				{
					debug_settextpos(x+pos-1, y);
					debug_print(" ");
					i[--pos] = '\0';
				}
				break;
			}
			case ENTER_KEY:
			{
				event.action = SELECT;
				i[pos] = '\0';
				return i;
			}
			case ESCAPE_KEY:
			{
				if (pos>0)
				{
					blank_entry = true;
					goto ReprintEntry;
				}
				event.action = CANCEL;
				return "";
			}
			default:
			{
				if (pos+1<maxlen && k>=' ' && (unsigned char)k<128)
				{
					i[pos++] = (char)k;
					sprintf(line, "%c", k);
					debug_print(line);
				}
			}
		}
	}
}


/* debug_getinvocationpath

	Systems for which main()'s argv[0] does not contain the full
	path of the debugger executable will have to add the current
	path here.
*/

void debug_getinvocationpath(char *argv)
{
	strcpy(invocation_path, argv);
}


/* debug_print

	Prints the supplied string at the current cursor position
	on the debugger screen in the current color.
*/

void debug_print(char *a)
{
	hugo_print(a);
}


/* debug_settextcolor and debug_setbackcolor */

void debug_settextcolor(int c)
{
	hugo_settextcolor(c);
}

void debug_setbackcolor(int c)
{
	hugo_setbackcolor(c);
}


/* debug_hardcopy

	Sends the supplied string to the printer as a single line.
	The string does not end with a newline character.
*/

void debug_hardcopy(FILE *printer, char *a)
{
	if (fprintf(printer, a)<0) device_error = true;
	if (fprintf(printer, "\n")<0) device_error = true;
}


/*  InitDebuggerCursesScreen

	Because we're keeping the engine's hugo_init_screen(), 	which
	would normally be replaced.  (Bad design on my part, probably.)
	There's no place where the debugger explicitly calls it, so we
	hook it from debug_switchscreen().
*/

char debugger_curses_screen_initialized = FALSE;

void InitDebuggerCursesScreen(void)
{
	int i;

	debugger_curses_screen_initialized = TRUE;
	
	hugo_settextmode();

	if ((game_screen = newwin(0, 0, 0, 0))==NULL ||
	    (debugger_screen = newwin(0, 0, 0, 0))==NULL ||
	    (help_screen = newwin(0, 0, 0, 0))==NULL ||
	    (auxiliary_screen = newwin(0, 0, 0, 0))==NULL)
	{
		hugo_clearfullscreen();
		printf("Fatal Error:  Debugger cannot display with existing video configuration\n");
		exit(-1);
	}

	debug_switchscreen(HELP);
	hugo_clearfullscreen();

	debug_switchscreen(AUXILIARY);
	hugo_clearfullscreen();

	debug_switchscreen(DEBUGGER);
	
	getmaxyx(active_screen_window, D_SCREENHEIGHT, D_SCREENWIDTH);

	window[CODE_WINDOW].top = D_SEPARATOR+1;
	window[CODE_WINDOW].height = D_SCREENHEIGHT-2-D_SEPARATOR;
	window[CODE_WINDOW].width = D_SCREENWIDTH-2;

	/* All others are the same height and width */
	for (i=VIEW_WATCH; i<WINDOW_COUNT; i++)
	{
		window[i].top = 3;
		window[i].height = D_SEPARATOR - 3;
		window[i].width = D_SCREENWIDTH-2;
	}

	/* While we're at it, initialize the CTRL-C and CTRL-BREAK
	   handler:
	*/
/*
	signal(SIGINT, debug_breakhandler);
*/
}


/* debug_switchscreen

	Switches the text display/output to either DEBUGGER, GAME, 
	HELP, or AUXILIARY, as supplied.  The cursor should also 
	be switched off or on (if applicable).
*/

extern int current_color_pair;	// from hegcc.c
extern int real_attributes;

void debug_switchscreen(int a)
{
	int last_active_screen;
	static struct old_window_data debug_win, game_win;
	static int game_current_color_pair = 0, game_real_attributes = 0;

	if (!debugger_curses_screen_initialized)
		InitDebuggerCursesScreen();

	last_active_screen = active_screen;

	if (active_screen!=INACTIVE_SCREEN)
		overwrite(current_window, active_screen_window);

	switch (active_screen = a)
	{
                case DEBUGGER:
		{
			/* Temporarily turn off game screen's attribute/
			   color mapping */
			game_current_color_pair = current_color_pair;
			game_real_attributes = real_attributes;
			current_color_pair = 0;
			real_attributes = 0;

			/* first save current (game) window info */
			if (last_active_screen==GAME)
			{
				game_win.top = text_windowtop;
				game_win.left = text_windowleft;
				game_win.bottom = text_windowbottom;
				game_win.right = text_windowright;

				game_win.textcolor = current_text_color;
				game_win.backcolor = current_back_color;
				game_win.xpos = current_text_col;
				game_win.ypos = current_text_row;
				
				game_win.currentline = currentline;
			}

			overwrite(active_screen_window = debugger_screen, current_window);
			debug_cursor(0);

			/* then restore previous debug window info */
			hugo_settextwindow(1, 1, SCREENWIDTH, SCREENHEIGHT);

			current_text_color = debug_win.textcolor;
			current_back_color = debug_win.backcolor;
			current_text_col = debug_win.xpos;
			current_text_row = debug_win.ypos;
			currentline = 0;

			break;
		}
		case GAME:
		{
			/* first save current (debug) window info */
			if (last_active_screen==DEBUGGER)
			{
				debug_win.top = text_windowtop;
				debug_win.left = text_windowleft;
				debug_win.bottom = text_windowbottom;
				debug_win.right = text_windowright;

				debug_win.textcolor = current_text_color;
				debug_win.backcolor = current_back_color;
				debug_win.xpos = current_text_col;
				debug_win.ypos = current_text_row;
			}

			overwrite(active_screen_window = game_screen, current_window);
			debug_cursor(1);

			/* then restore previous game window info */
			hugo_settextwindow(game_win.left,
					game_win.top,
					game_win.right,
					game_win.bottom);

			current_text_color = game_win.textcolor;
			current_back_color = game_win.backcolor;
			current_text_col = game_win.xpos;
			current_text_row = game_win.ypos;
			
			currentline = game_win.currentline;

			current_color_pair = game_current_color_pair;
			real_attributes = game_real_attributes;

			break;
		}
		case HELP:
		case AUXILIARY:
		{
			if (active_screen==HELP)
				overwrite(active_screen_window = help_screen, current_window);
			else
				overwrite(active_screen_window = auxiliary_screen, current_window);
			debug_cursor(0);
			debug_settextpos(1, D_SCREENHEIGHT);
			break;
		}
	}
}


/* debug_cursor

	Turns the cursor on (1) or off (0)--if necessary.
*/

void debug_cursor(int n)
{
	switch(n)
	{
		case (0):
		{
			curs_set(0);
			break;
		}
		default:
			curs_set(1);
	}
}


/* debug_settextpos

	Different from hugo_settextpos() in that it doesn't do any
	accounting for proportional printing.
*/

void debug_settextpos(int x, int y)
{
	hugo_settextpos(x, y);
}


/* debug_windowsave and debug_windowrestore

   	Before drawing a menu or other box, it is necessary to save what
	was behind it to be restored when the menu or box is closed.
*/

void *debug_windowsave(int left, int top, int right, int bottom)
{
	chtype *buf;
	int i, line_width;
	size_t size;

	/* determine array size needed to store the rectangle of
	   existing text */
	size = (bottom-top+1) * (line_width = right-left+1);

	if ((buf = AllocMemory((size+2)*sizeof(chtype)))==NULL)
		DebuggerFatal(D_MEMORY_ERROR);

	buf[0] = size;
	buf[1] = line_width;

	for (i=0; i<(int)size; i++)
		buf[i+2] = debug_windowreadchar(left+i%line_width-1, top+i/line_width-1);

	debug_windowshadow(left, top, right, bottom);

	return buf;
}

void debug_windowrestore(void *b, int xpos, int ypos)	/* note:  (x, y) */
{
	chtype *buf = b;
	int i, size, line_width;

	size = buf[0];
	line_width = buf[1];

	for (i=0; i<size; i++)
		debug_windowwritechar(xpos+i%line_width-1, ypos+i/line_width-1, buf[i+2]);

	free(buf);
}


/* debug_windowreadchar and debug_windowwritechar */

int debug_windowreadchar(int x, int y)
{
	wmove(current_window, y, x);
	return (int)winch(current_window);      /* get the character */
}

void debug_windowwritechar(int x, int y, int c)
{
	chtype attr;

        attr = c & A_ATTRIBUTES;        /* extract attributes */
	wmove(current_window, y, x);
	wattrset(current_window, attr);
	wbkgdset(current_window, attr | ' ');
	waddch(current_window, c);
}


/* debug_windowshadow

	Called, simply for convenience, by debug_windowsave in order to
	reuse its parameters.  Draws a shadow along the right and bottom
	of the just-saved and about-to-be-printed menu space.
*/

extern int local_fore_color, local_back_color;	// from hegcc.c

void debug_windowshadow(int left, int top, int right, int bottom)
{
#ifndef BEOS
	int i, j, c;
	int saved_fg, saved_bg;

	saved_fg = local_fore_color;
	saved_bg = local_back_color;

	/* draw righthand shadow first */
	for (i=top+1; i<=bottom; i++)
	{
		for (j=0; j<=1; j++)
		{
			c = debug_windowreadchar(right-j-1, i-1);

			/* strip away everything but the character code */
			c&=0x00FF;
			if (monochrome_mode) c |= A_REVERSE;

			debug_windowwritechar(right-j-1, i-1, c);
		}
	}

	/* then draw bottom shadow */
	for (i=left+2; i<=right; i++)
	{
		c = debug_windowreadchar(i-1, bottom-1);
		c = (int)(c&0x00FF);
		if (monochrome_mode) c |= A_REVERSE;
		debug_windowwritechar(i-1, bottom-1, c);
	}

	hugo_settextcolor(saved_fg);
	hugo_setbackcolor(saved_bg);
#endif	// ifndef BEOS
}


/* debug_windowbottomrow

	Necessary in case the OS in question forces a linefeed when a
	character is output to the bottom-right corner of the screen.
	This prints a caption at the left side of the screen, and the
	current filename at the right.
*/

void debug_windowbottomrow(char *caption)
{
	char b[256];

	debug_settextpos(1, D_SCREENHEIGHT);
	debug_settextcolor(color[MENU_TEXT]);
	debug_setbackcolor(color[MENU_BACK]);

	strcpy(b, caption);     /* in case line[] is the source */

	strcpy(line, b);
	memset(line+strlen(b), ' ', D_SCREENWIDTH-strlen(b)-strlen(gamefile)-1);
	sprintf(line+D_SCREENWIDTH-strlen(gamefile)-1, "%s", gamefile);
	debug_print(line);

	/* print to the last screen position */
	wmove(current_window, D_SCREENHEIGHT, D_SCREENWIDTH);
	waddch(current_window, ' ');
}


/* debug_windowscroll

	Scrolls the defined window in a direction given by the param
	constant.
*/

void debug_windowscroll(int left, int top, int right, int bottom, int param, int lines)
{
	int x, y;
	chtype c, attr;
	
	debug_settextcolor(color[NORMAL_TEXT]);
	debug_setbackcolor(color[NORMAL_BACK]);

	while (lines--)
	{
	  switch (param)
	  {
		case UP:
			for (y=top+1; y<=bottom; y++)
			for (x=left; x<=right; x++)
			{
				wmove(current_window, y-1, x-1);
				c = winch(current_window);      /* get the character */
				attr = c & A_ATTRIBUTES;        /* extract attributes */
                
				wmove(current_window, y-2, x-1);
				wattrset(current_window, attr);
				wbkgdset(current_window, attr | ' ');
				waddch(current_window, c);
			}

			/* blank line */
			rectangle(left, bottom, right, bottom);

			break;

		case DOWN:
			for (y=bottom; y>=top+1;  y--)
			for (x=left; x<=right; x++)
			{
				wmove(current_window, y-2, x-1);
				c = winch(current_window);      /* get the character */
				attr = c & A_ATTRIBUTES;        /* extract attributes */
                
				wmove(current_window, y-1, x-1);
				wattrset(current_window, attr);
				wbkgdset(current_window, attr | ' ');
				waddch(current_window, c);
			}

			/* blank line */
			rectangle(left, top, right, top);

			break;
	  }
	}
}


/* debug_clearview

        Clears the selected window given by n.  (Basically, however,
	all the windows except Code share the same space.)
*/

void debug_clearview(int n)
{
	/* These are entirely separate full screens in this port */
	if (n==VIEW_HELP || n==VIEW_AUXILIARY)
	{
		hugo_clearfullscreen();
		return;
	}

	debug_settextcolor(color[NORMAL_TEXT]);
	debug_setbackcolor(color[NORMAL_BACK]);

	rectangle(2, window[n].top, D_SCREENWIDTH-1, window[n].top+window[n].height-1);
}


/* debug_shortcutkeys

	Switches to the Auxiliary window/page and prints the shortcut
	keys available to this port/system.
*/

void debug_shortcutkeys(void)
{
	debug_switchscreen(HELP);
	debug_settextcolor(color[NORMAL_TEXT]);
	debug_setbackcolor(color[NORMAL_BACK]);
	hugo_clearfullscreen();

	sprintf(line, "Hugo Debugger v%d.%d%s Shortcut Keys\n\n", HEVERSION, HEREVISION, HEINTERIM);
	debug_settextpos(Center(line), 1);
	debug_settextcolor(color[SELECT_TEXT]);
	debug_setbackcolor(color[SELECT_BACK]);
	debug_print(line);

	debug_settextcolor(color[NORMAL_TEXT]);
	debug_setbackcolor(color[NORMAL_BACK]);

	sprintf(line, "%s: Activate menubar\n", MENUBAR_KEY);
	debug_print(line);
	debug_print("TAB:  Toggle screen\n\n");

	debug_print("F1/Alt+C:  Context help    Shift+F1/Alt+P:  Topic Help    Alt+K:  Shortcut Keys\n\n");
	debug_print("F2/Ctrl+T:  Object Tree      Shift+F2/Ctrl+O:  Move Object\n");
	debug_print("F3/Ctrl+S:  Search\n");
	debug_print("F5/Ctrl+G:  Go               Shift+F5/Ctrl+R:  Finish Routine\n");
	debug_print("F6/Alt+W:   Switch Window\n");
	debug_print("F7/Ctrl+V:  Set Value\n");
	debug_print("F8/Ctrl+F:  Step Forward     Shift+F8:         Step Back\n");
	debug_print("F9/Ctrl+B:  Breakpoint       Shift+F9/Ctrl+W:  Watch Value\n");
	debug_print("F10/Ctrl+E: Step Over        Shift+F10/Ctrl+N: Skip Next\n");
	debug_print("\nAlt+Shift+X:      Exit\n");

	debug_print("\nUse ARROW KEYS, HOME, END, PAGE UP, and PAGE DOWN to navigate.\n");
	debug_print("(Shift+Alt+H and Shift+Alt+E move to window start and end.)\n");
	debug_print("Press ENTER or DELETE to select or delete, where appropriate.\n");
	debug_print("(If your system doesn't support Alt+key, use Esc+key instead.)");

	debug_windowbottomrow("Press any key...");
	hugo_waitforkey();

	debug_windowbottomrow("");
	debug_switchscreen(DEBUGGER);
}
