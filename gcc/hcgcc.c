/*
	HCGCC.C (by Bill Lash lash@tellabs.com)

	Non-portable functions specific to GCC and UNIX

	This file has changed very little since the hugo 2.0 port.
	All of the changes that were needed to other files for the
	2.0 and 2.1 ports are no longer necessary.
	
*/

#include "hcheader.h"

/*  MEMORY ALLOCATION: */

void *hugo_malloc(size_t a)
{
	return malloc(a);		/* i.e. malloc() */
}

void hugo_free(void *block)
{
	free(block);			/* i.e. free() */
}


/*
    FILENAME MANAGEMENT:

    Different operating systems will have their own ways of naming
    files.  The following routines are simply required to know and
    be able to dissect/build the components of a particular filename,
    storing/restoring the components via the specified char arrays.

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
    CLOSEFILES:
*/

void hugo_closefiles()
{
/*
          fclose(sourcefile);
          fclose(objectfile);
          fclose(textfile);
          fclose(allfile);
          fclose(listfile);
*/
}


/*
    DATE FUNCTION:

    hugo_date() is only required to produce a date-type, 8-character string,
    stored in the specified string array.
*/

char *hugo_date(char *a)
{
        time_t now;
	time(&now);
	strftime(a,9,"%m-%d-%y",localtime(&now));	/* Stored; no strcpy() will be performed */
	return a;
}


/*
    SUPPORT FUNCTIONS:
*/

void rmtmp(void)
{
}

