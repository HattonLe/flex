/* gen - actual generation (writing) of flex scanners */

/*  This file is part of flex. */

/*  Redistribution and use in source and binary forms, with or without */
/*  modification, are permitted provided that the following conditions */
/*  are met: */

/*  1. Redistributions of source code must retain the above copyright */
/*     notice, this list of conditions and the following disclaimer. */
/*  2. Redistributions in binary form must reproduce the above copyright */
/*     notice, this list of conditions and the following disclaimer in the */
/*     documentation and/or other materials provided with the distribution. */

/*  Neither the name of the University nor the names of its contributors */
/*  may be used to endorse or promote products derived from this software */
/*  without specific prior written permission. */

/*  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR */
/*  PURPOSE. */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "flexdef.h"
#include "genclass.h"

// Assumed the flexTemplate .txt files use this unique string for the <ClassName> placeholder text.
#define PlaceHolder "yyFlexLexer"

#define CharEolCR '\n'
#define CharEolLF '\r'

// Read from file into buffer this size, chunk bytes at a time
#define BUFFERSIZE 256
#define CHUNKSIZE  128

// Template .txt files can have Linux, Mac or Windows EOL terminations.
#include "flexTemplateB.h"
#include "flexTemplateD.h"

#include "flexTemplateI.h"

// Name of the flex generated C++ .cpp & .h file
#define FlexLexerFileCpp "FlexLexer.cpp"
#define FlexLexerFileH "FlexLexer.h"

static bool DoneBaseClass = false;

// Like memchr only better :op
// Bounded by end position rather than size. Using an absolute end saves recomputing size for advancing buffer ptrs.
// Used to avoid the problem of char arrays that are not NUL terminated, so no guarantee that string prims like
// strchr wouldn't search forever. 
static char *arraychr(char * __s, int __c, char * __e)
{
	char *f;

	f = __s;
	while ((__e > f) && (__c != *f))
	{
		f++;
	}
	if (__e == f)
	{
		f = NULL;
	}
	return f;
}

// Find a line of text from a buffer, starting at the given buffer ptr.
// Bounded by end position rather than size. Using an absolute end saves recomputing size for advancing buffer ptrs.
// Just in case, its capable of handling Linux, Mac or Windows EOL terminations.
// returns ptr to 1st EOL terminator found, or NULL if none found in buffer.
static char *FindLine(char * __s, char * __e)
{
	char *f;

	// Look for a CR
	f = arraychr(__s, CharEolCR, __e);
	// None found, so...
	if (NULL == f)
	{
		// Look for a LF instead
		f = arraychr(__s, CharEolLF, __e);
	}
	return f;
}

// Gets the length of the line found by FindLine(), starting at the given buffer ptr.
// Bounded by end position rather than size. 
// Just in case, its capable of handling lines with Linux, Mac or Windows EOL termination.
// The length includes the line terminator character(s).
static int GetLineLen(char * __s, char * __e, char *f)
{
	int Len;

	if (CharEolCR == *f)
	{
		f++;
	}
	// prevent from attempting to looking past end of buffer.
	if (__e > f)
	{
		if (CharEolLF == *f)
		{
			f++;
		}
	}

	// Length of line including terminator(s)
	return f - __s;
}

// Replace any placeholder text in the given Line with that of the DerivedClassName
// Line is a writable buffer, of size SizeOfLine, containing a NUL terminated string.
static void SubstPlaceholders(char *Line, int SizeOfLine, char *DerivedClassName)
{
	char *Found;
	int DeltaStr;

	// TBD would be nice to take this out of the inner loop to save recomputing for every line in a file
	// Determine if we will need to expand or contract Line during text replacement
	// +ve means Line string will grow.
	DeltaStr = strlen(DerivedClassName) - strlen(PlaceHolder);

	// If we need to update a placeholder
	Found = strstr(Line, PlaceHolder);
	while (NULL != Found)
	{
		// If expanding or contracting required
		if (0 != DeltaStr)
		{
			char *PlaceHolderEnd;
			int ToShift;

			PlaceHolderEnd = Found + strlen(PlaceHolder);
			// Need to include NUL terminator as well
			ToShift = strlen(Line) - strlen(DerivedClassName) + 1;

			// If Growing the Line string
			if (0 < DeltaStr)
			{
				int FreeSpace;

				FreeSpace = SizeOfLine - strlen(Line);

				// Check we won't overrun Line[]
				if (FreeSpace < DeltaStr)
				{
					char errmsg[MAXLINE];

					sprintf(errmsg, "SubstPlaceholders() Expansion overrun %u chars\n", ToShift);
					fprintf (stderr, "%s: %s\n", program_name, errmsg);
				}
				else
				{
					// Grow Line[] string
					memmove(PlaceHolderEnd + DeltaStr, PlaceHolderEnd, ToShift);
				}
			}
			else
			{
				// shrink Line[] string (note DeltaStr is -ve, so we add)
				memcpy(PlaceHolderEnd + DeltaStr, PlaceHolderEnd, ToShift);
			}
		}

		// overwrite placeholder with the value of DerivedClassName
		memcpy(Found, DerivedClassName, strlen(DerivedClassName));

		// any more placeholders to do
		Found = strstr(Line, PlaceHolder);
	}
}

char BodgeText[] =
{
	"// BODGE Preprocessing of original cpp file text performed by flex genclass.c SubstPlaceholdersInFile()\n"
	"//       in lieu of reworking of internal cpp file generator.\n"
};

// Convert placeholders within an entire file.
// Just in case, its capable of handling files with Linux, Mac or Windows EOL termination.
// We can use this temporarily to mostly process old style generated cpp files into new form
// until that original cpp generation code can get updated.
void SubstPlaceholdersInFile(char *InFilename, char *OutFilename, char *DerivedClassName)
{
	FILE *FileIn;
	FILE *FileOut;
	static int LineCount = 1;

	FileIn = fopen(InFilename, "r");
	if (NULL == FileIn)
	{
		char errmsg[MAXLINE];

		sprintf(errmsg, "SubstPlaceholdersInFile() Error reading '%s'.\n", InFilename);
		fprintf (stderr, "%s: %s\n", program_name, errmsg);
	}
	else
	{
		FileOut = fopen(OutFilename, "w");
   		if (NULL == FileOut)
		{
			char errmsg[MAXLINE];

			sprintf(errmsg, "SubstPlaceholdersInFile() Error writing '%s'.\n", OutFilename);
			fprintf (stderr, "%s: %s\n", program_name, errmsg);
		}
		else
		{
			char Buffer[BUFFERSIZE];
			int BufCnt;
			char *BufferEnd;
			char *ChunkEnd;
			char *buf = Buffer;

			// Prefix output file with temporary notice
			fwrite(BodgeText, 1, sizeof(BodgeText) - 1, FileOut);

			// Buffer end limit, not to be exceeded
			BufferEnd = Buffer + sizeof(Buffer);

			// Read from file text in CHUNKSIZE amounts, up to a BUFFERSIZE bytes worth whilst searching for an EOL terminator.
			// The buffer may end up holding one or more lines of EOL terminated text.
			BufCnt = fread(Buffer, 1, CHUNKSIZE, FileIn);
			ChunkEnd = Buffer + BufCnt;

			// until file content exhausted
			while (0 < BufCnt)
			{
				char *PtrEOL;
				int BufferSpaceUsed;

				// Until we find a complete line of text, run out of buffer, or run out of file content.
				PtrEOL = NULL; //FindLine(Buffer, ChunkEnd);
				while ((NULL == PtrEOL) && (BufferEnd > ChunkEnd) && (0 < BufCnt))
				{
					int RemainingSpace;

					RemainingSpace = BufferEnd - ChunkEnd;

					// No complete line in chunk, so need to read more in
					BufCnt = fread(ChunkEnd, 1, (CHUNKSIZE > RemainingSpace) ? RemainingSpace : CHUNKSIZE, FileIn);
					ChunkEnd += BufCnt;

					// Look for a complete line of text in the buffer.
					PtrEOL = FindLine(Buffer, ChunkEnd);
				}

				// This might represent multiple lines of text.
				BufferSpaceUsed = (ChunkEnd - Buffer);

				// If we didn't find a complete line of text
				if (NULL == PtrEOL)
				{
					// We might have run out of buffer space
   					if (BufferEnd == ChunkEnd)
					{
						char errmsg[MAXLINE];

						sprintf(errmsg, "SubstPlaceholdersInFile() Exceeded buffer searching for EOL, file %s Line:%u.\n", InFilename, LineCount);
						fprintf (stderr, "%s: %s\n", program_name, errmsg);

						// We can still continue
						// but should think about increasing BUFFERSIZE in order to handle longer lines.
						ChunkEnd = Buffer;
					}

					// We might have run out of file content if mid line and read nothing
					if ((0 != BufferSpaceUsed) && (0 == BufCnt))
					{
						char errmsg[MAXLINE];

						sprintf(errmsg, "SubstPlaceholdersInFile() Exhausted file searching for EOL, file %s Line:%u.\n", InFilename, LineCount);
						fprintf (stderr, "%s: %s\n", program_name, errmsg);
					}
				}
				else
				{
					int Len;
					char Line[MAXLINE];

					LineCount++;

					// Get length of the line including any EOL terminator character(s)
					Len = GetLineLen(Buffer, ChunkEnd, PtrEOL);

					// Line may grow during substitution so it needs to be in another buffer, and NUL terminated too.
					strncpy(Line, Buffer, Len);
					Line[Len] = '\0';

					// Substitute any placeholder text
					SubstPlaceholders(Line, sizeof(Line), DerivedClassName);

					// Write out the possibly updated line
					fwrite(Line, 1, strlen(Line), FileOut);

					// a complete line was processed, so reuse its buffer space...
					// if we have unprocessed line data remaining in the buffer
					if (Len < BufferSpaceUsed)
					{
						// reclaim buffer space used by that line.
						// a bit inefficient but saves dealing with ring buffer searching etc.
						memmove(Buffer, &Buffer[Len], BufferSpaceUsed - Len);
						ChunkEnd -= Len;
					}
					else
					{
						// Nothing to keep so just reuse the entire buffer
						ChunkEnd = Buffer;
					}
				}
			}

			// close file
			fclose(FileOut);
			FileOut = NULL;
		}

		fclose(FileIn);
		FileIn = NULL;
	}
}

// Creates a derived class definition named "<ClassName>FlexLexer"
// If ClassName is NULL, <ClassName> defaults to "yy".
// Output to file, if not existing, creates the Flex header file containing the base class definition.
// Class definitions come from the embedded copies (flexTemplate?.h) of the old "/usr/lib/FlexLexer.h" file.
void CreateClassHeader(char *ClassName)
{
	char DerivedClassName[256];
	char DerivedFilename[259];
	FILE *File;

	// Default to original class naming convention
	sprintf(DerivedClassName, "%s", (ClassName != NULL) ? ClassName : "yyFlexLexer");
	sprintf(DerivedFilename, "%s.h", DerivedClassName);

   	if (!DoneBaseClass)
	{
		// Ensure we always have the base class header file.
		File = fopen(FlexLexerFileH, "w");
   		if (NULL == File)
		{
			char errmsg[MAXLINE];

			sprintf(errmsg, "CreateClassHeader() Error truncating '%s'.\n", FlexLexerFileH);
			fprintf (stderr, "%s: %s\n", program_name, errmsg);
		}
		else
		{
			// Create the Flex base class header (the upper part of the old /usr/lib/FlexLexer.h)
			// Nothing to change in the template, so just write it out verbatim.
			fwrite(FlexLexerB_txt, 1, FlexLexerB_txt_len, File);
			fclose(File);
			File = NULL;
		}
		// Ensure we always have the base class implementation file.
		File = fopen(FlexLexerFileCpp, "w");
   		if (NULL == File)
		{
			char errmsg[MAXLINE];

			sprintf(errmsg, "CreateClassHeader() Error truncating '%s'.\n", FlexLexerFileCpp);
			fprintf (stderr, "%s: %s\n", program_name, errmsg);
		}
		else
		{
			// Create the Flex base class implementation (parts of the old flex generated .cpp files)
			// Nothing to change in the template, so just write it out verbatim.
			fwrite(FlexLexerI_txt, 1, FlexLexerI_txt_len, File);
			fclose(File);
			File = NULL;
		}
		DoneBaseClass = true;
	}

	// Ensure we always have the derived class header file.
   	File = fopen(DerivedFilename, "w");
   	if (NULL == File)
	{
		char errmsg[MAXLINE];

		sprintf(errmsg, "CreateClassHeader() Error truncating '%s'.\n", DerivedFilename);
		fprintf (stderr, "%s: %s\n", program_name, errmsg);
	}
	else
	{
		char *Chunk;
		char *ChunkEnd;
		int DeltaStr;

		// Determine if we will need to expand or contract Line during text replacement
		// +ve means Line string will grow.
		DeltaStr = strlen(DerivedClassName) - strlen(PlaceHolder);

		Chunk = FlexLexerD_txt;
		ChunkEnd = FlexLexerD_txt + FlexLexerD_txt_len;
		while (ChunkEnd > Chunk)
		{
			char *Found;

			// Extact the next line from the embedded template file.
			// Solved lack of NUL terminator for FlexLexerD_txt, so we can't run off the end of array anymore.
			Found = FindLine(Chunk, ChunkEnd);
			if (NULL != Found)
			{
				int Len;

				// Get length of the line including any EOL terminator character(s)
				Len = GetLineLen(Chunk, ChunkEnd, Found);

				// If the line contains any usable text
				if (2 < Len)
				{
					char Line[MAXLINE];

					// Line string includes EOL terminator character(s)
					strncpy(Line, Chunk, Len);
					Line[Len] = '\0';

					// Substitute any placeholder text
					SubstPlaceholders(Line, sizeof(Line), DerivedClassName);

					fwrite(Line, 1, strlen(Line), File);
				}
				else
				{
					// Just write out the line verbatim.
					fwrite(Chunk, 1, Len, File);
				}

				Chunk += Len;
			}
		}

   		// close the file 
   		fclose(File);
		File = NULL;
	}
}
