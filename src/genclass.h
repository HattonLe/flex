/* Convenience header for conditional use of GNU <libintl.h>.
   Copyright (C) 1995-1998, 2000-2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

#ifndef _GENCLASS_H
#define _GENCLASS_H 1

// Convert placeholders within an entire file.
// Just in case, its capable of handling files with Linux, Mac or Windows EOL termination.
// We can use this temporarily to mostly process old style generated cpp files into new form
// until that original cpp generation code can get updated.
void SubstPlaceholdersInFile(char *InFilename, char *OutFilename, char *DerivedClassName);

// Creates a derived class definition named "<ClassName>FlexLexer"
// If ClassName is NULL, <ClassName> defaults to "yy".
// Output to file, if not existing, creates new Flex class files and for now auto modifies a copy of the original cpp class file.
// Class definitions come from the embedded copies (flexTemplate?.h) of the old "/usr/lib/FlexLexer.h" file.
void CreateClassFiles(char *ClassName);

// Use this method for testing out where to do this generation with a flushed flex cpp output file ready.
void RunGenerator(int trace);

#endif /* _GENCLASS_H */
