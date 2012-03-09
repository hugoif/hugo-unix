/*
	HEGCC.C (by Bill Lash lash@tellabs.com)

	(revised for Hugo v2.5.x by Kent Tessman)

	Non-portable functions specific to GCC and UNIX

	This file provides the user interface functions specific
	to the port, as well as some file handling and allocation
	routines.

	This version finally gets a lot of the screen handling issues
	of the previous ports working properly.  In prior versions,
	the title at the top of the help menus was not being shown
	properly.  I also finally got around to adding color to the
	port when using ncurses (the color handling can be enabled
	and disabled in the makefile.gcc by setting the DO_COLOR 
	define).

	Kent added a few features to Hugo that affected the port as
	well.  The first is ability to specify bold, italic, underline
	and proportional fonts.  This port supports bold, underline,
	and italic (italic is indicated as reverse video).  No support
	is provided here for proportional fonts.

	Kent also added support for special characters (such as characters
	with umlauts, cedilla, the British pound sign, etc).  I have
	added the support to this port.  It seems to work ok as long
	as the terminal supports these characters.  I use xterms with
	the fixed font at work, and there the fixed font does not contain
	these special characters.  However, if I use the courier font
	the characters do exist.  Under Linux at home, both fonts have
	the special characters.  If you want to compile without support
	for these characters, you may remove the define for DO_SPECIAL
	in the makefile.gcc.
*/

#include "heheader.h"

#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

/* Function prototypes: */
void hugo_addcommand(void);
void hugo_restorecommand(int);
int hugo_color(int c);

/* Specific to hegcc.c: */
void ResetXtermDimensions(void);
void ConstrainCursor(void);

/* Defined Hugo colors: */
#define HUGO_BLACK 	   0
#define HUGO_BLUE	   1
#define HUGO_GREEN	   2
#define HUGO_CYAN	   3
#define HUGO_RED	   4
#define HUGO_MAGENTA	   5
#define HUGO_BROWN	   6
#define HUGO_WHITE	   7
#define HUGO_DARK_GRAY	   8
#define HUGO_LIGHT_BLUE	   9
#define HUGO_LIGHT_GREEN   10
#define HUGO_LIGHT_CYAN	   11
#define HUGO_LIGHT_RED	   12
#define HUGO_LIGHT_MAGENTA 13
#define HUGO_YELLOW	   14
#define HUGO_BRIGHT_WHITE  15


#define HISTORY_SIZE    16      /* for command-line editing */
int hcount = 0;
char *history[HISTORY_SIZE];

WINDOW *current_window;

/* Now some variables and constants to manage text/graphics display--
   all specific to hegcc.c (taken from hedjgpp.c):
*/
int text_windowleft, text_windowtop, text_windowright, text_windowbottom,
	text_windowwidth;
int insert_mode = true;
int print_cursor = false;       /* probably never be needed */
int current_text_col, current_text_row, current_text_color, current_back_color;

#if defined (DEBUGGER)
#include "hdheader.h"
void *AllocMemory(size_t size);
extern int fake_graphics_mode;
extern int monochrome_mode;
extern int colors[];
char debugger_invert = false;
#endif


/*
    MEMORY ALLOCATION:

    hugo_blockalloc(), and hugo_blockfree() are necessary because of
    the way MS-DOS handles memory allocation across more than 64K.
    For most systems, these will simply be normal ANSI function calls.
*/

void *hugo_blockalloc(long num)
{
        return malloc(num * sizeof(char));
}

void hugo_blockfree(void *block)
{
        free(block);
}


/*
    FILENAME MANAGEMENT:

    Different operating systems will have their own ways of naming
    files.  The following routines are simply required to know and
    be able to dissect/build the components of a particular filename,
    storing/restoring the compenents via the specified char arrays.

    For example, in MS-DOS:

        hugo_splitpath("C:\HUGO\FILES\HUGOLIB.H", ...)
                becomes:  C:, HUGO\FILES, HUGOLIB, H

    and

        hugo_makepath(..., "C:", "HUGO\FILES", "HUGOLIB", "H")
                becomes:  C:\HUGO\FILES\HUGOLIB.H

    The appropriate equivalent nomenclature should be used for the
    operating system in question.
*/

void hugo_splitpath(char *path, char *drive, char *dir, char *fname, char *ext)
{
char *file;
char *extension;

        strcpy(drive,"");
        strcpy(dir,"");
        strcpy(fname,"");
        strcpy(ext,"");

        if ((file = strrchr(path,'/')) == 0)
        {
                if ((file = strrchr(path,':')) == 0) file = path;
        }
        strncpy(dir,path,strlen(path)-strlen(file));
        *(dir+strlen(path)-strlen(file)) = 0;
        extension = strrchr(path,'.');
        if ((extension != 0) && strlen(extension) < strlen(file))
        {
                strncpy(fname,file,strlen(file)-strlen(extension));
                *(fname+strlen(file)-strlen(extension)) = 0;
                strcpy(ext,extension+1);
        }
        else strcpy(fname,file);

        if (strcmp(dir, "") && fname[0]=='/') strcpy(fname, fname+1);
}

void hugo_makepath(char *path, char *drive, char *dir, char *fname, char *ext)
{
        if (*ext == '.') ext++;
        strcpy(path,drive);
        strcat(path,dir);
        switch (*(path+strlen(path)))
        {
        case '/':
        case ':':
/*        case 0: */
                break;
        default:
                if (strcmp(path, "")) strcat(path,"/");
                break;
        }
        strcat(path,fname);
        if (strcmp(ext, "")) strcat(path,".");
/* Crashes without -fwritable-strings:
        strcat(path,strlwr(ext)); */
	strcat(path, ext);
}


/*
    OVERWRITE:

    Checks to see if the given filename already exists, and prompts to
    replace it.  Returns true if file may be overwritten.
*/

int hugo_overwrite(char *f)
{
	FILE *tempfile;

	if (!(tempfile = fopen(f, "rb")))	/* if file doesn't exist */
		return true;

	fclose(tempfile);

	sprintf(pbuffer, "Overwrite existing \"%s\" (Y or N)?", f);
	RunInput();

	if (words==1 && (!strcmp(strupr(word[1]), "Y") || !strcmp(strupr(word[1]), "YES")))
		return true;

	return false;
}


/*
    CLOSEFILES:

    Closes all open files.  NOTE:  If the operating system automatically
    closes any open streams upon exit from the program, this function may
    be left empty.
*/

void hugo_closefiles()
{
/*
        fclose(game);
  	if (script) fclose(script);
  	if (io) fclose(io);
  	if (record) fclose(record);
*/
}


/*
    GETFILENAME:

    Loads the name of the filename to save or restore (as specified by
    the argument <a>) into the line[] array.
*/

void hugo_getfilename(char *a, char *b)
{
	unsigned int i, p;

	sprintf(line, "Enter path and filename %s.", a);
	AP(line);

	sprintf(line,"%c(Default is %s): \\;", NO_CONTROLCHAR, b);
	AP(line);

	p = var[prompt];
	var[prompt] = 0;        /* null string */

	RunInput();

	var[prompt] = p;

	remaining = 0;

	strcpy(line, "");
	if (words==0)
		strcpy(line, b);
	else
	{
		for (i=1; i<=(unsigned int)words; i++)
			strcat(line, word[i]);
	}
}


/*
    GETKEY:

    Returns the next keystroke waiting in the keyboard buffer.  It is
    expected that hugo_getkey() will return the following modified
    keystrokes:

	up-arrow        11 (CTRL-K)
	down-arrow      10 (CTRL-J)
	left-arrow       8 (CTRL-H)
	right-arrow     21 (CTRL-U)

    Also, accented characters are converted to non-accented characters.
    Invalid keystrokes are returned as 255.
*/

int hugo_getkey(void)
{
	int b;

        /*
         * In case this is used in halfdelay() or nodelay() mode
         * need to make sure we don't pass bad data to getline()
         */
        while ((b = getch())==ERR)
        {
        }

	switch (b)
	{
		case KEY_UP:
			{b = 11; break;}
		case KEY_LEFT:
			{b = 8; break;}
		case KEY_RIGHT:
			{b = 21; break;}
		case KEY_DOWN:
			{b = 10; break;}
			
		case (21):	/* Ctrl+U */
			{b = 27; break;}
	}
        return b;
}

/*
    GETLINE

    Gets a line of input from the keyboard, storing it in <buffer>.

    (NOTE:  The function keys used here are QuickC/MS-DOS specific.
    They have been chosen to somewhat mirror the command-line editing
    keys of MS-DOS.  For other systems, the 'if (b==<value>)' lines
    will likely need to be changed.)

    If a similar library input function is available, it will almost
    certainly be preferable to use that; in QuickC such was unfortunately
    not the case.

    Note also that this input function assumes a non-proportional font.
*/

#define CURRENT_CHAR(c) ((c<(int)strlen(buffer))?buffer[c]:' ')

void hugo_getline(char *p)
{
	char ch[2];
	int a, b, thiscommand;
	int c;                          /* c is the character being added */
	int x, y, oldx, oldy;

NewPrompt:
	hugo_settextcolor(fcolor);
	hugo_setbackcolor(bgcolor);

	strcpy(buffer, "");
	c = 0;
	thiscommand = hcount;

	oldx = (currentpos+charwidth)/charwidth;
	oldy = currentline;
	y = oldy;
	x = oldx + hugo_strlen(p);

	hugo_print(p);

GetKey:

	hugo_settextpos(x, y);

	b = hugo_getkey();

	hugo_settextcolor(icolor);
	hugo_setbackcolor(bgcolor);

	/* Now, start key checking */
	switch (b)
	{
#if defined (DEBUGGER)
		case (9):                       /* Tab */
		{
			during_input = true;
			Debugger();
			during_input = false;

			/* If the debugger is stopping execution, quitting,
			   etc., a blank string must be returned in the
			   input buffer.
			*/
			if (debugger_collapsing)
			{
				strcpy(buffer, "");
				if (active_screen==GAME)
					hugo_scrollwindowup();
				return;
			}

			goto GetKey;
		}
#endif

		case (13):                      /* Enter */
	        case KEY_ENTER:
		{
			hugo_settextpos(x, y);
			sprintf(ch, "%c", CURRENT_CHAR(c));
			hugo_print(ch);
			full = 0;

			if (script) fprintf(script, "%s%s\n", p, buffer);

			hugo_settextpos(1, y + 2);
			if (y >= physical_windowheight/lineheight - 1)
				hugo_scrollwindowup();

			strcpy(buffer, Rtrim(buffer));
			hugo_addcommand();
//#if defined (NO_LATIN1_CHARSET)
//			hugo_stripaccents(buffer);
//#endif
			return;
		}
	        case KEY_IC:    /* Insert */
		{
			insert_mode = !insert_mode;
			goto GetKey;
		}
	        case KEY_BACKSPACE:
	        case KEY_DC:
	        case (127):
	        case (4):	/* Ctrl+D */
		{
			if (strlen(buffer)>0)
			{
				if (b==KEY_BACKSPACE || c==strlen(buffer))
				{
					b = KEY_BACKSPACE;
					if (c==0) goto GetKey;
					c--;
				}

				buffer[strlen(buffer)+1] = '\0';
				for (a=c; a<=(int)strlen(buffer); a++)
					buffer[a] = buffer[a+1];

				if (b==KEY_BACKSPACE) x--;

				hugo_settextpos(oldx+hugo_strlen(p), y);
				hugo_print(buffer);
				hugo_settextpos(oldx+hugo_strlen(p)+strlen(buffer), y);
				hugo_print("  ");
			}
			goto GetKey;
		}
		case (8):                       /* left-arrow */
	        case KEY_LEFT:
		case (2):			/* Ctrl+B */
		{
			if (c > 0)
			{
				hugo_settextpos(x, y);
				sprintf(ch, "%c", CURRENT_CHAR(c));
				hugo_print(ch);
				x--, c--;
			}
			goto GetKey;
		}
		case (21):                      /* right-arrow */
	        case KEY_RIGHT:
		case (6):			/* Ctrl+F */
		{
			if (c<(int)strlen(buffer))
			{
				hugo_settextpos(x, y);
				sprintf(ch, "%c", CURRENT_CHAR(c));
				hugo_print(ch);
				x++, c++;
			}
			goto GetKey;
		}
/*
		case CTRL_LEFT_KEY:
		{
			if (c)
			{
				hugo_settextpos(x, y);
				sprintf(ch, "%c", CURRENT_CHAR(c));
				hugo_print(ch);
				do
				{
					do
						c--, x--;
					while (c && buffer[c-1]!=' ');
				}
				while (c && buffer[c]==' ');
			}
			goto GetKey;
		}
		case CTRL_RIGHT_KEY:
		{
			if (c<(int)strlen(buffer))
			{
				hugo_settextpos(x, y);
				sprintf(ch, "%c", CURRENT_CHAR(c));
				hugo_print(ch);
				do
				{
					do
						c++, x++;
					while (c<(int)strlen(buffer) &&
						buffer[c-1]!=' ');
				}
				while (c<(int)strlen(buffer) && buffer[c]==' ');
			}
			goto GetKey;
		}
*/
	        case KEY_HOME:
		case (1):			/* Ctrl+A */
		case (362):
		{
			hugo_settextpos(x, y);
			sprintf(ch, "%c", CURRENT_CHAR(c));
			hugo_print(ch);
			c = 0;
			x = oldx + hugo_strlen(p);
			goto GetKey;
		}
		case KEY_END:
		case (5):			/* Ctrl+E */
		case (385):
		{
			hugo_settextpos(x, y);
			sprintf(ch, "%c", CURRENT_CHAR(c));
			hugo_print(ch);
			c = strlen(buffer);
			x = oldx + hugo_strlen(p) + strlen(buffer);
			goto GetKey;
		}
		case (27):                      /* Escape */
		case (24):                      /* Ctrl+X */
		{
BlankLine:
			hugo_settextpos(oldx+hugo_strlen(p), y);

			memset(buffer, ' ', text_windowwidth);

			buffer[text_windowwidth-oldx-hugo_strlen(p)] = '\0';
			hugo_print(buffer);
			hugo_settextpos(oldx, y);

			goto NewPrompt;
		}
                case KEY_F(3):                  /* F3 scan code */
	        case KEY_UP:
		case (11):                      /* up-arrow */
		{
			if (--thiscommand<0)
			{
				thiscommand = 0;
				goto GetKey;
			}
			a = strlen(buffer);
RestoreCommand:
			hugo_restorecommand(thiscommand);
			x = oldx + strlen(buffer) + hugo_strlen(p);
			hugo_settextpos(oldx+hugo_strlen(p), y);
			hugo_print(buffer);
			while (a >= (int)strlen(buffer))
				{hugo_print(" ");
				a--;}
			hugo_settextpos(x-1, y);
			c = strlen(buffer);
			goto GetKey;
		}
		case (10):                      /* down-arrow */
		case KEY_DOWN:
		{
			a = strlen(buffer);
			if (++thiscommand>=hcount) goto BlankLine;
			goto RestoreCommand;
		}
	}

	/* Disallow invalid keystrokes */
	if (b < 32 || b>=256) goto GetKey;
	
	/* Hugo circa v2.2 allowed '^' && '~' for '\n' && '\"',
	   respectively
	*/
	if (game_version<=22 && (b=='^' || b=='~')) goto GetKey;

	if (oldx+strlen(buffer) >= text_windowwidth-2)
		goto GetKey;

	hugo_settextpos(x++, y);
	sprintf(ch, "%c", b);                   /* add the new character */
	hugo_print(ch);
	buffer[strlen(buffer)+1] = '\0';
	if (c<(int)strlen(buffer) && insert_mode)
	{
		hugo_settextpos(x, y);
		hugo_print(buffer+c);
		for (a=strlen(buffer); a>c; a--)
			buffer[a] = buffer[a-1];
	}
	buffer[c] = (char)b;

	c++;

	goto GetKey;
}


/* hugo_iskeywaiting

    Returns true if a keypress is waiting to be retrieved.
*/

int hugo_iskeywaiting(void)
{
        int c;

        /*
         * Normally we want to be in cbreak mode so that nothing happens
         * as long as the user doesn't press a key.  To sort of mimic
         * the behavior of kbhit() we can switch to either halfdelay or
         * nodelay() mode.  Some curses implementations don't have ungetch().
         * Deleting the ungetch() call would cause the character to be swallowed
         * This doesn't seem to cause a problem (I originally had it that way).
         *
         * Kent's note:  circumventing ungetch() may cause a bit of a problem
         * since I changed all key input (such as via hugo_waitforkey()) to
         * be routed through hugo_getkey() for keycode translation.
         *
         */

        halfdelay(1);
        if((c=getch()) != ERR)
        {
                nocbreak();
                cbreak();
                ungetch(c);
                return 1;
        }
        else
        {
                nocbreak();
                cbreak();
                return 0;
        }
}

/*
    WAITFORKEY:

    Provided to be replaced by multitasking systems where cycling while
    waiting for a keystroke may not be such a hot idea.
*/

int hugo_waitforkey(void)
{
        wmove(current_window, current_text_row-1, current_text_col-1);
        
        while (hugo_iskeywaiting())       /* clear key buffer */
        {
                /*
                 * If hugo_keywaiting() were made to swallow
                 * the character, we would want to delete this call.
                 */
                hugo_getkey();
        }
        while (!hugo_iskeywaiting())
        {
        }
        /*
         * If hugo_keywaiting() were made to swallow
         * the character, we would want to delete this call.
         */
        return hugo_getkey();
}


/* hugo_timewait

    Waits for 1/n seconds.  Returns false if waiting is unsupported.
*/

int hugo_timewait(int n)
{
	wtimeout(current_window, 1000/n);
	return true;
}


/*
    COMMAND HISTORY:

    To store/retrieve player inputs for editing.
*/

void hugo_addcommand(void)
{
	int i;

	if (!strcmp(buffer, "")) return;

        if (hcount>=HISTORY_SIZE)
	{
		hugo_blockfree(history[0]);
		for (i=0; i<HISTORY_SIZE-1; i++)
			history[i] = history[i+1];
		hcount = HISTORY_SIZE-1;
	}

	/* Because the debugger might use (up to) all available memory for
	   code line storage, a different means of memory allocation is
	   needed (at least in MS-DOS due to limited available memory to
	   begin with).
	*/
#if !defined (DEBUGGER)
	if ((history[hcount] = (char *)hugo_blockalloc((long)((strlen(buffer)+1)*sizeof(char))))==NULL)
#else
	if ((history[hcount] = (char *)AllocMemory((size_t)((strlen(buffer)+1)*sizeof(char))))==NULL)
#endif
	{
		hugo_blockfree(history[0]);
		if (hcount)
		{
			for (i=0; i<hcount; i++)
				history[i] = history[i+1];
			hcount--;
		}
		return;
	}

	for (i=0; i<=(int)strlen(buffer); i++)
		history[hcount][i] = buffer[i];
	hcount++;
}

void hugo_restorecommand(int n)
{
	int i;

	if (n < 0 || (n>=hcount && hcount!=HISTORY_SIZE-1)) return;

	i = 0;
	do
		buffer[i] = history[n][i];
	while (history[n][i++]!='\0');
}


/*
    DISPLAY CONTROL:

    These functions are specific library calls to QuickC/MS-DOS
    for screen control.  Each _function() should be replaced by
    the equivalent call for the operating system in question.  If
    a particular operation is unavailable, in some cases it may be
    left empty.  In other cases, it may be necessary to write a
    brief function to achieve the desired effect.
*/

/* ResetXtermDimensions

	Since you can dynamically resize an xterm window, try to
	adjust the current display dimensions to follow.
*/

int last_SCREENWIDTH, last_SCREENHEIGHT;

void ResetXtermDimensions(void)
{
	getmaxyx(current_window, SCREENHEIGHT, SCREENWIDTH);

	if (SCREENWIDTH!=last_SCREENWIDTH || SCREENHEIGHT!=last_SCREENHEIGHT)
	{
		if (text_windowright==last_SCREENWIDTH)
			text_windowright = SCREENWIDTH;
		if (text_windowbottom==last_SCREENHEIGHT)
			text_windowbottom = SCREENHEIGHT;

		hugo_settextwindow(text_windowleft, text_windowtop,
				   text_windowright, text_windowbottom);
	}

	last_SCREENWIDTH = SCREENWIDTH, last_SCREENHEIGHT = SCREENHEIGHT;
}
	
void hugo_setgametitle(char *t)
{}

void rectangle(int left, int top, int right, int bottom)
{
	int i, j;

	for (i=top; i<=bottom; i++)
	{
		for (j=left; j<=right; j++)
		{
			wmove(current_window, i-1, j-1);
			waddch(current_window, ' ');
		}
	}
}

void hugo_clearfullscreen(void)
{
/* Clears everything on the screen, moving the cursor to the top-left
   corner of the screen */

	rectangle(1, 1, SCREENWIDTH, SCREENHEIGHT);

	/* Must be set: */
	currentpos = 0;
	currentline = 1;
}


void hugo_clearwindow(void)
{
/* Clears the currently defined window, moving the cursor to the top-left
   corner of the window */

	rectangle(text_windowleft, text_windowtop, text_windowright, text_windowbottom);

	/* Must be set: */
	currentpos = 0;
	currentline = 1;
}


void hugo_settextmode(void)
{
/* This function does whatever is necessary to set the system up for
   a standard text display */
        charwidth = FIXEDCHARWIDTH = 1;          /* again, for non-proportional */
        lineheight = FIXEDLINEHEIGHT = 1;

	/* Must be set: */
	getmaxyx(current_window, SCREENHEIGHT, SCREENWIDTH);

	/* Must be set: */
	hugo_settextwindow(1, 1,
		SCREENWIDTH/FIXEDCHARWIDTH, SCREENHEIGHT/FIXEDLINEHEIGHT);
}

void hugo_settextwindow(int left, int top, int right, int bottom)
{
/* Once again, the arguments for the window are passed using the same
   unit of measurement as SCREENRIGHT, SCREENLEFT, PAGETOP, etc., whether
   that is pixels or numbers of characters.

   The text window, once set, represents the scrolling/bottom part of the
   screen. */

	/* Character, not pixel, coordinates: */
	text_windowtop = top;
	text_windowbottom = bottom;
	text_windowleft = left;
	text_windowright = right;
	text_windowwidth = text_windowright-text_windowleft+1;

	/* Must be set: */
	/* (Engine-required parameters) */
	physical_windowleft = (left-1)*FIXEDCHARWIDTH;
	physical_windowtop = (top-1)*FIXEDLINEHEIGHT;
	physical_windowright = right*FIXEDCHARWIDTH-1;
	physical_windowbottom = bottom*FIXEDLINEHEIGHT-1;
	physical_windowwidth = (right-left+1)*FIXEDCHARWIDTH;
	physical_windowheight = (bottom-top+1)*FIXEDLINEHEIGHT;

	ConstrainCursor();
}

void hugo_settextpos(int x, int y)
{
/* The top-left corner of the current active window is (1, 1).

   (In other words, if the screen is being windowed so that the top row
   of the window is row 4 on the screen, the (1, 1) refers to the 4th
   row on the screen, and (2, 1) refers to the 5th.)

   This function must also properly set currentline and currentpos (where
   currentline is a the current character line, and currentpos may be
   either in pixels or characters, depending on the measure being used).
*/
	/* Must be set: */
	currentline = y;
	currentpos = (x-1)*charwidth;   /* Note:  zero-based */

	current_text_col = text_windowleft-1+x;
	current_text_row = text_windowtop-1+y;
	ConstrainCursor();

	wmove(current_window, current_text_row-1, current_text_col-1);
}

void ConstrainCursor(void)
{
	/* ConstrainCursor() is needed since HEDJGPP.C's cursor-
	   positioning doesn't automatically pay attention to any
	   currently-defined window.
	*/
	if (current_text_col > text_windowright) current_text_col = text_windowright;
	if (current_text_col < text_windowleft) current_text_col = text_windowleft;
	if (current_text_row > text_windowbottom) current_text_row = text_windowbottom;
	if (current_text_row < text_windowtop) current_text_row = text_windowtop;
}

void hugo_print(char *a)
{
	char just_repositioned = false;
        char last_was_newline = false;
	int i, len;


	len = strlen(a);

	for (i=0; i<len; i++)
	{
                last_was_newline = false;

		switch (a[i])
		{
			case '\n':
				++current_text_row;
				wmove(current_window, current_text_row, current_text_col);
				just_repositioned = true;
                                last_was_newline = true;
				break;
			case '\r':
CarriageReturn:
				current_text_col = text_windowleft;
				wmove(current_window, current_text_row, current_text_col);
				just_repositioned = true;
				break;
			default:
			{
				wmove(current_window, current_text_row-1, current_text_col-1);
				waddch(current_window, a[i]);

				if (++current_text_col > text_windowright)
				{
					current_text_col = text_windowleft;
					++current_text_row;
				}
				just_repositioned = false;
			}
		}

		if (current_text_row > text_windowbottom)
		{
			current_text_row = text_windowbottom;
                        hugo_scrollwindowup();
		}
	}

	if (!just_repositioned) wmove(current_window, current_text_row, current_text_col);

        /* Because '\n' should imply '\r' */
	if (last_was_newline)
        {
                last_was_newline = false;
                goto CarriageReturn;
        }
}


void hugo_scrollwindowup()
{
	int x, y;
	chtype c, attr;

	/* Basically just copies a hunk of screen to an upward-
	   shifted position and fills in the screen behind/below it
	*/
	for (x=text_windowleft; x<=text_windowright; x++)
        for (y=text_windowtop+1; y<=text_windowbottom; y++)
	{
		wmove(current_window, y-1, x-1);
		c = winch(current_window);      /* get the character */
                attr = c & A_ATTRIBUTES;        /* extract attributes */
                
		wmove(current_window, y-2, x-1);
		wattrset(current_window, attr);
		wbkgdset(current_window, attr | ' ');
		waddch(current_window, c);
	}

  	rectangle(text_windowleft, text_windowbottom, text_windowright, text_windowbottom);
}


/* The following static variable are used to save the current attributes and
   color pair being used. */
int current_color_pair = 0;
int real_attributes = 0;


/* The following defines are used for the real_attributes variable.  Note that
   both REAL_BOLD and REAL_BRITE_COLOR cause the curses BOLD attribute to be set.
   Also note that REAL_ITALIC is mapped to reverse video. */

#define REAL_BOLD  1
#define REAL_ITALIC  2
#define REAL_UNDERLINE 4
#define REAL_BRITE_COLOR 8

void	hugo_real_attributes_mapper(int attrib, int pairnum)
{
	int	curs_attr=0;
	if(attrib & REAL_ITALIC)
	{
		curs_attr|=A_REVERSE;
	}
	if(attrib & (REAL_BOLD | REAL_BRITE_COLOR))
	{
		curs_attr|=A_BOLD;
	}
	if(attrib & REAL_UNDERLINE)
	{
		curs_attr|=A_UNDERLINE;
	}
#if  defined DO_COLOR
	if(has_colors())
	{
		curs_attr |= COLOR_PAIR(pairnum);
	}
#endif
#ifdef DEBUGGER
	if (monochrome_mode && debugger_invert)
		curs_attr |= A_REVERSE;
#endif
	wattrset(current_window,curs_attr);
	wbkgdset(current_window,curs_attr | ' '); 
}

void hugo_font(int f)
{
	real_attributes &= ~(REAL_BOLD | REAL_ITALIC | REAL_UNDERLINE);

        if (f & UNDERLINE_FONT)
	{
		real_attributes |= REAL_UNDERLINE;
	}
        if (f & ITALIC_FONT)
	{
		real_attributes |= REAL_ITALIC;
	}
        if (f & BOLD_FONT)
	{
		real_attributes |= REAL_BOLD;
	}

	hugo_real_attributes_mapper(real_attributes,current_color_pair);
}

/* The following local varibles are used to keep track of the current
   foreground and background colors. */
int local_fore_color= DEF_FCOLOR;
int local_back_color = DEF_BGCOLOR;

int find_color_pair(int fgc,int bgc)
{
	int	pair;
#if defined DO_COLOR
	int	max_col=COLORS;
	
	if(max_col > 8)
	{
		max_col=8;
	}
	if(fgc > (max_col-1))
	{
		fgc=max_col-1;
	}
	if(bgc > (max_col-1))
	{
		bgc=max_col-1;
	}

	pair=bgc*max_col + max_col - 1  - fgc;
	if(pair > (COLOR_PAIRS-1)) 
	{
		pair = 0;
	}
#else
	pair = 0;
#endif
	return	pair;
	
}
void hugo_settextcolor(int c)   /* foreground (print) color */
{
	local_fore_color=hugo_color(c);

        /* If the color value is greater than 8, make the text bold */ 
        if(local_fore_color>=8)
        {
		local_fore_color -= 8;
		real_attributes |= REAL_BRITE_COLOR;
        }
        else
        {
		real_attributes &= ~REAL_BRITE_COLOR;
        }
	current_color_pair=find_color_pair(local_fore_color,local_back_color);
	hugo_real_attributes_mapper(real_attributes,current_color_pair);
}

void hugo_setbackcolor(int c)   /* background color */
{
#ifdef DEBUGGER
	if (c==color[SELECT_BACK] || c==color[MENU_SELECTBACK])
		debugger_invert = true;
	else
		debugger_invert = false;
#endif
	local_back_color=hugo_color(c);

        if(local_back_color>=8)
        {
		local_fore_color -= 8;
        }
	current_color_pair=find_color_pair(local_fore_color,local_back_color);
	hugo_real_attributes_mapper(real_attributes,current_color_pair);
}

int hugo_gettextcolor()
{
	return local_fore_color;
}

int hugo_getbackcolor()
{
	return local_back_color;
}

int hugo_color(int c)
{
	/* Color-setting functions should always pass the color through
	   hugo_color() in order to properly set default fore/background
	   colors:
	*/

        if (c==16)      c = DEF_FCOLOR;
        else if (c==17) c = DEF_BGCOLOR;
	else if (c==18) c = DEF_SLFCOLOR;
	else if (c==19) c = DEF_SLBGCOLOR;
	else if (c==20) c = hugo_color(fcolor);
 
#if defined DO_COLOR
        if (c==0)       c = COLOR_BLACK;
        else if (c==1)  c = COLOR_BLUE;
        else if (c==2)  c = COLOR_GREEN;
        else if (c==3)  c = COLOR_CYAN;
        else if (c==4)  c = COLOR_RED;
        else if (c==5)  c = COLOR_MAGENTA;
        else if (c==6)  c = COLOR_YELLOW;
        else if (c==7)  c = COLOR_WHITE;
        else if (c==8)  c = COLOR_BLACK + 8;
        else if (c==9)  c = COLOR_BLUE + 8;
        else if (c==10) c = COLOR_GREEN + 8;
        else if (c==11) c = COLOR_CYAN + 8;
        else if (c==12) c = COLOR_RED + 8;
        else if (c==13) c = COLOR_MAGENTA + 8;
        else if (c==14) c = COLOR_YELLOW + 8;
        else if (c==15) c = COLOR_WHITE + 8;
#endif
        return c;
}


/* CHARACTER AND TEXT MEASUREMENT

   	For non-proportional printing, these should be pretty much as
	described below, as screen dimensions will be given in characters,
	not pixels.

	For proportional printing, screen dimensions need to be in
	pixels, and each width routine must take into account the
	current font and style.

	The hugo_strlen() function is used to give the length of
	the string not including any non-printing control characters.
*/

int hugo_charwidth(char a)
{
	if (a==FORCED_SPACE)
		return FIXEDCHARWIDTH;	  /* same as ' ' */

	else if ((unsigned char)a >= ' ') /* alphanumeric characters */

		return FIXEDCHARWIDTH;    /* because printing 
					     is non-proportional */
	return 0;
}

int hugo_textwidth(char *a)
{
	int i, slen, len = 0;

	slen = strlen(a);

	for (i=0; i<slen; i++)
	{
		if (a[i]==COLOR_CHANGE) i+=2;
		else if (a[i]==FONT_CHANGE) i++;
		else
			len += hugo_charwidth(a[i]);
	}

	return len;
}

int hugo_strlen(char *a)
{
	int i, slen, len = 0;

	slen = strlen(a);

	for (i=0; i<slen; i++)
	{
		if (a[i]==COLOR_CHANGE) i+=2;
		else if (a[i]==FONT_CHANGE) i++;
		else len++;
	}

	return len;
}


void hugo_init_screen(void)
{
        /*
         * Need to initialize the screen before use by curses
         */
        initscr();
#if defined DO_COLOR
	start_color();
	if(has_colors())
	{
		int	max_col=COLORS;
		int	i;
		if(max_col > 8)
		{
			max_col=8;
		}
		for(i=1;i<COLOR_PAIRS;i++)
		{
			init_pair(i,(max_col-1)-(i % max_col),i/max_col);
			if(((max_col-1)-(i % max_col))==(i/max_col))
			{
				init_pair(i,COLOR_WHITE,COLOR_BLACK);
			}
		}
	}
#endif
        cbreak();
        noecho();
        nonl();
	current_window = stdscr;
//        idlok(current_window, TRUE);
        keypad(current_window, TRUE);

	/* We're going to use our own scrolling trigger */
	scrollok(current_window, FALSE);
}

void hugo_cleanup_screen(void)
{
        /*
         * This will return the terminal to its original settings
         */
	hugo_settextcolor(DEF_FCOLOR);
	hugo_setbackcolor(DEF_BGCOLOR);
	hugo_font(0);
	hugo_clearfullscreen();
        endwin();
}


#if !defined (COMPILE_V25)
int hugo_hasvideo(void)
{
	return false;
}

int hugo_playvideo(HUGO_FILE infile, long reslength,
	char loop_flag, char background, int volume)
{
	fclose(infile);
	return true;
}

void hugo_stopvideo(void)
{}
#endif

int hugo_hasgraphics(void)
{
#if defined (DEBUGGER)
	return fake_graphics_mode;
#else
	return false;
#endif
}

int hugo_displaypicture(FILE *infile, long len)
{
        fclose(infile);         /* since infile will be open */

        return 1;
}

#if !defined (SOUND_SUPPORTED)	
int hugo_playmusic(FILE *f)		/* from hesound.c */
{
	fclose(f);
	return true;	/* not an error */
}

void hugo_musicvolume(int vol)
{}

void hugo_stopmusic(void)
{}

int hugo_playsample(FILE *f)
{
	fclose(f);
	return true;	/* not an error */
}

void hugo_samplevolume(int vol)
{}

void hugo_stopsample(void)
{}
#endif
