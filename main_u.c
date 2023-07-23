/*
 * main_u.c
 *
 * Released under GPL
 *
 * Copyright (C) 1998-2003 A.J. van Os
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Description:
 * The main program of 'antiword' (Unix version)
 */

/* Change log:
 *
 * 20.09.2003 Changed by Max Tomin (maxtomin@mail.ru).
 *		  "main_u.c", "options.c", "output.c" and "antiword.h" was changed.
 *              All changes is enclosed in conditional compiling brackets like:
 *		  #ifdef __SYMBIAN32__
 *			//New code
 *		  #else
 *			//Old code
 *		  #endif
 *
 *		  Changes:
 *		  - Ported to EPOC32 (MARM platform), using LIBC library.
 *		  - Define 0x98 #UNDEFINED string in the "cp1251.txt" mapping file
 *		  - For EPOC32, simple console interface was added
 *		  - For EPOC32, default mapping file is now "encoding.txt"
 *		  - "encoding.txt" can be placed both in exe directory and in "C:\Antiword" directory
 * 		  - For EPOC32, default and maximum screen width was set to 0x7FFFFFL (maximum working value)
 */

#include <stdio.h>
#include <stdlib.h>
#if defined(__dos)
#include <fcntl.h>
#include <io.h>
#endif /* __dos */
#if defined(__STDC_ISO_10646__)
#include <locale.h>
#endif /* __STDC_ISO_10646__ */
#if defined(N_PLAT_NLM)
#if !defined(_VA_LIST)
#include "NW-only/nw_os.h"
#endif /* !_VA_LIST */
#include "getopt.h"
#endif /* N_PLAT_NLM */
#include "version.h"
#include "antiword.h"

/* The name of this program */
static const char	*szTask = NULL;
#ifdef __SYMBIAN32__
static const char	*szMinusM = "-m";
static const char	*szEncoding = "encoding.txt";
static const char	*szMinusW = "-w";
static const char	*szWidth[8];
static char	szInputBuffer[256];
static char	szInFile[256];
static char	szOutFile[256];
#endif /* __SYMBIAN32__ */
 

static void
vUsage(void)
{
#ifndef __SYMBIAN32__
	
	fprintf(stderr, "\tName: %s\n", szTask);
	fprintf(stderr, "\tPurpose: "PURPOSESTRING"\n");
	fprintf(stderr, "\tAuthor: "AUTHORSTRING"\n");
	fprintf(stderr, "\tVersion: "VERSIONSTRING"\n");
	fprintf(stderr, "\tStatus: "STATUSSTRING"\n");
	fprintf(stderr,
		"\tUsage: %s [switches] wordfile1 [wordfile2 ...]\n", szTask);
	fprintf(stderr,
		"\tSwitches: [-t|-p papersize|-x dtd][-m mapping][-w #][-i #]"
		"[-Ls]\n");
	fprintf(stderr, "\t\t-t text output (default)\n");
	fprintf(stderr, "\t\t-p <paper size name> PostScript output\n");
	fprintf(stderr, "\t\t   like: a4, letter or legal\n");
	fprintf(stderr, "\t\t-x <dtd> XML output\n");
	fprintf(stderr, "\t\t   like: db (DocBook)\n");
	fprintf(stderr, "\t\t-m <mapping> character mapping file\n");
	fprintf(stderr, "\t\t-w <width> in characters of text output\n");
	fprintf(stderr, "\t\t-i <level> image level (PostScript only)\n");
	fprintf(stderr, "\t\t-L use landscape mode (PostScript only)\n");
	fprintf(stderr, "\t\t-s Show hidden (by Word) text\n");
#endif
} /* end of vUsage */

/*
 * pStdin2TmpFile - save stdin in a temporary file
 *
 * returns: the pointer to the temporary file or NULL
 */
static FILE *
pStdin2TmpFile(long *lFilesize)
{
	FILE	*pTmpFile;
	size_t	tSize;
	BOOL	bFailure;
	UCHAR	aucBytes[BUFSIZ];

	DBG_MSG("pStdin2TmpFile");

	fail(lFilesize == NULL);

	/* Open the temporary file */
	pTmpFile = tmpfile();
	if (pTmpFile == NULL) {
		return NULL;
	}

#if defined(__dos) && !defined(__SYMBIAN32__)
	/* Stdin must be read as a binary stream */
	setmode(fileno(stdin), O_BINARY);
#endif /* __dos !defined(__SYMBIAN32__)*/

	/* Copy stdin to the temporary file */
	*lFilesize = 0;
	bFailure = TRUE;
	for (;;) {
		tSize = fread(aucBytes, 1, sizeof(aucBytes), stdin);
		if (tSize == 0) {
			bFailure = feof(stdin) == 0;
			break;
		}
		if (fwrite(aucBytes, 1, tSize, pTmpFile) != tSize) {
			bFailure = TRUE;
			break;
		}
		*lFilesize += (long)tSize;
	}

#if defined(__dos) && !defined(__SYMBIAN32__)
	/* Switch stdin back to a text stream */
	setmode(fileno(stdin), O_TEXT);
#endif /* __dos && !defined(__SYMBIAN32__) */

	/* Deal with the result of the copy action */
	if (bFailure) {
		*lFilesize = 0;
		(void)fclose(pTmpFile);
		return NULL;
	}
	rewind(pTmpFile);
	return pTmpFile;
} /* end of pStdin2TmpFile */

/*
 * bProcessFile - process a single file
 *
 * returns: TRUE when the given file is a supported Word file, otherwise FALSE
 */
#ifdef __SYMBIAN32__
static BOOL bProcessFile(const char *szFilename, FILE *outfile)
#else
static BOOL bProcessFile(const char *szFilename)
#endif 
{
	FILE		*pFile;
	diagram_type	*pDiag;
	long		lFilesize;
	int		iWordVersion;
	BOOL		bResult;

	fail(szFilename == NULL || szFilename[0] == '\0');

	DBG_MSG(szFilename);

	if (szFilename[0] == '-' && szFilename[1] == '\0') {
		pFile = pStdin2TmpFile(&lFilesize);
		if (pFile == NULL) {
			werr(0, "I can't save the standard input to a file");
			return FALSE;
		}
	} else {
		pFile = fopen(szFilename, "rb");
		if (pFile == NULL) {
			werr(0, "I can't open '%s' for reading", szFilename);
			return FALSE;
		}

		lFilesize = lGetFilesize(szFilename);
		if (lFilesize < 0) {
			(void)fclose(pFile);
			werr(0, "I can't get the size of '%s'", szFilename);
			return FALSE;
		}
	}

	iWordVersion = iGuessVersionNumber(pFile, lFilesize);
	if (iWordVersion < 0 || iWordVersion == 3) {
		if (bIsRtfFile(pFile)) {
			werr(0, "%s is not a Word Document."
				" It is probably a Rich Text Format file",
				szFilename);
		} if (bIsWordPerfectFile(pFile)) {
			werr(0, "%s is not a Word Document."
				" It is probably a Word Perfect file",
				szFilename);
		} else {
#if defined(__dos)
			werr(0, "%s is not a Word Document or the filename"
				" is not in the 8+3 format.", szFilename);
#else
			werr(0, "%s is not a Word Document.", szFilename);
#endif /* __dos */
		}
		(void)fclose(pFile);
		return FALSE;
	}
	/* Reset any reading done during file testing */
	rewind(pFile);

#ifdef __SYMBIAN32__
	pDiag = pCreateDiagram(szTask, szFilename, outfile);
#else
	pDiag = pCreateDiagram(szTask, szFilename);
#endif 
	if (pDiag == NULL) {
		(void)fclose(pFile);
		return FALSE;
	}

	bResult = bWordDecryptor(pFile, lFilesize, pDiag);
	vDestroyDiagram(pDiag);

	(void)fclose(pFile);
	return bResult;
} /* end of bProcessFile */

#ifdef __SYMBIAN32__
int InputString(char *buffer, int MaxCount) { //By Max Tomin
	int CurCount = 0;
	char ch;
	do {
		ch = getchar();
		switch (ch) {
			case 8:
			case 127:
				if (CurCount) CurCount--;
				break;
			case 27:
				putchar(13);
				break;
			default:
				if (ch >= 32) buffer[CurCount++] = ch;

		}
	} while (ch != 10 && ch != 27);
	return (ch != 27);
}
#endif /* __SYMBIAN32__ */

int
main(int argc, char **argv)
{
	options_type	tOptions;
	const char	*szWordfile;
	int	iFirst, iIndex, iGoodCount;
	BOOL	bUsage, bMultiple, bUseTXT, bUseXML;
#ifdef __SYMBIAN32__
	argv[1] = szMinusM; 
	argv[2] = szEncoding; 
	argc = 3;

	printf("Released under GPL\n");
	printf("\n");
	printf("Copyright (C) 1998-2002 A.J. van Os\n");
	printf("\n");
	printf("This program is distributed in the hope that it will be useful,\n");
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	printf("GNU General Public License for more details.\n");
	printf("\n");
	printf("You should have received a copy of the GNU General Public License\n");
	printf("along with this program; if not, write to the Free Software\n");
	printf("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
	printf("\n");

	printf("Enter input file name: ");
	if (!InputString(&szInFile, 256) || !szInFile[0]) return 0;

	printf("Enter output file name: ");
	if (!InputString(&szOutFile, 256) || !szOutFile[0]) return 0;

	printf("Enter screen width (<Enter> for maximum width) : ");
	if (!InputString(&szWidth, 7)) return 0;

	if (szWidth[0]) {
		argv[argc++] = szMinusW;
		argv[argc++] = szWidth;
	}
	argv[argc++] = szInFile; 
#endif /* __SYMBIAN32__ */

	if (argc <= 0) {
		return EXIT_FAILURE;
	}

	szTask = szBasename(argv[0]);

	if (argc <= 1) {
		iFirst = 1;
		bUsage = TRUE;
	} else {
		iFirst = iReadOptions(argc, argv);
		bUsage = iFirst <= 0;
	}
	
	if (bUsage) {
		vUsage();
		return iFirst < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
	}

#if defined(N_PLAT_NLM) && !defined(_VA_LIST)
	nwinit();
#endif /* N_PLAT_NLM && !_VA_LIST */

	vGetOptions(&tOptions);

#if defined(__STDC_ISO_10646__)
	/*
	 * If the user wants UTF-8 and the envirionment variables support
	 * UTF-8, than set the locale accordingly
	 */
	if (tOptions.eEncoding == encoding_utf8 && is_locale_utf8()) {
		if (setlocale(LC_CTYPE, "") == NULL) {
			werr(1, "Can't set the UTF-8 locale! "
				"Check LANG, LC_CTYPE, LC_ALL.");
		}
		DBG_MSG("The UTF-8 locale has been set");
	}
#endif /* __STDC_ISO_10646__ */

	bMultiple = argc - iFirst > 1;
	bUseTXT = tOptions.eConversionType == conversion_text;
	bUseXML = tOptions.eConversionType == conversion_xml;
	iGoodCount = 0;

	if (bUseXML) {
		fprintf(stdout,
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	"<!DOCTYPE %s PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\n"
	"\t\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">\n",
		bMultiple ? "set" : "book");
		if (bMultiple) {
			fprintf(stdout, "<set>\n");
		}
	}

	#ifdef __SYMBIAN32__
	{
		FILE *outfile = fopen(szOutFile, "w");
		if (!outfile) 
			werr(1, "Output file not found");
		for (iIndex = iFirst; iIndex < argc; iIndex++) {
			if (bMultiple && bUseTXT) {
				szWordfile = szBasename(argv[iIndex]);
				fprintf(outfile, "::::::::::::::\n");
				fprintf(outfile, "%s\n", szWordfile);
				fprintf(outfile, "::::::::::::::\n");
			}
			if (bProcessFile(argv[iIndex], outfile)) {
				iGoodCount++;
			}
		}
		fclose(outfile);
	}
	
	#else /* not defined __SYMBIAN32__ */
	for (iIndex = iFirst; iIndex < argc; iIndex++) {
		if (bMultiple && bUseTXT) {
			szWordfile = szBasename(argv[iIndex]);
			fprintf(stdout, "::::::::::::::\n");
			fprintf(stdout, "%s\n", szWordfile);
			fprintf(stdout, "::::::::::::::\n");
		}
		if (bProcessFile(argv[iIndex])) {
			iGoodCount++;
		}
	}
	#endif /* __SYMBIAN32__ */


	if (bMultiple && bUseXML) {
		fprintf(stdout, "</set>\n");
	}

	DBG_DEC(iGoodCount);
#ifdef __SYMBIAN32__
	if (iGoodCount >= 1)
		printf("Converting complete.\n");
#endif /* __SYMBIAN32__ */
	return iGoodCount <= 0 ? EXIT_FAILURE : EXIT_SUCCESS;
} /* end of main */
