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

#include "genclass.h"

// Ugly why don't people define header files. Because they're too lazy. grrrrr.
void lerr (const char *msg, ...);

// Assumed the flexTemplate .txt files use this unique string for the <ClassName> placeholder text.
#define PlaceHolder "yyFlexLexer"

#define CharEOL 0x0A

// Assumed the template .txt files have CharEOL line terminator only (i.e. doesn't handle CRLF as yet).
#include "flexTemplateB.h"
#include "flexTemplateD.h"

// Name of the flex generated C++ header file.
#define FlexClassFile "FlexLexer.h"

static bool DoneBaseClass = false;

// Creates a derived class definition named "<ClassName>FlexLexer"
// If ClassName is NULL, <ClassName> defaults to "yy".
// Output to file, if not existing, creates the Flex header file containing the base class definition.
// Class definitions come from the embedded copies (flexTemplate?.h) of the old "/usr/lib/FlexLexer.h" file.
void CreateClassHeader(char *ClassName)
{
	char DerivedClassName[50];
	char DerivedFilename[50];
	FILE *File;

	// Default to original class naming convention
	sprintf(DerivedClassName, "%sFlexLexer", (ClassName != NULL) ? ClassName : "yy");
	sprintf(DerivedFilename, "%s.h", DerivedClassName);

	// why is this not outputting anything ???
	printf("Hello\n");

   	if (!DoneBaseClass)
	{
		// Ensure we always have the base class header file.
		File = fopen(FlexClassFile, "w");
   		if (NULL == File)
		{
   			lerr("CreateClassHeader() Error truncating '%s'.\n", FlexClassFile);
		}
		else
		{
			// No base class, so

			// Create the Flex base class header (the upper part of the old /usr/lib/FlexLexer.h)
			// Nothing to change in the template, so just write it out verbatim.
			fwrite(FlexLexerB_txt, 1, FlexLexerB_txt_len, File);
			fclose(File);
			File = NULL;
		}
		DoneBaseClass = true;
	}

   	// Open file in append mode
   	File = fopen(DerivedFilename, "a");
   	if (NULL == File)
	{
   		lerr("CreateClassHeader() Error opening '%s' for appending!\n", DerivedFilename);
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
			int Len;
			char *Found;
			char Line[100];
			bool IsLineFound;

			// Extact the next line from the embedded template file.
			// NOTE FlexLexerD_txt is not NUL terminated, so we depend on the last value in it being CharEOL.
			// If it isn't stchr() could scan past the array end, but we test for that next.
			Found = strchr(Chunk, CharEOL);
			IsLineFound = (NULL != Found) && (ChunkEnd >= Found);
			if (IsLineFound)
			{
				Len = Found - Chunk + 1;

				// If the line contains any text
				if (1 < Len)
				{
					// Line[] includes the CharEOL character
					strncpy(Line, Chunk, Len);
					Line[Len] = '\0';

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
							ToShift = strlen(Line) - (PlaceHolderEnd - Line) + 1;

							// If Growing the Line string
							if (0 < DeltaStr)
							{
								int FreeSpace;

								FreeSpace = sizeof(Line) - strlen(Line);

								// Check we won't overrun Line[]
								if (FreeSpace < DeltaStr)
								{
			      					lerr("CreateClassHeader() Expansion overrun %u chars\n", ToShift);
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

					fwrite(Line, 1, strlen(Line), File);
				}
				else
				{
					// Just write out the empty line.
					fwrite(Chunk, 1, 1, File);
				}

				Chunk += Len;
			}
		}

   		// close the file 
   		fclose(File);
		File = NULL;
	}
}
