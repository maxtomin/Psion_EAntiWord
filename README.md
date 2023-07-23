Antiword ver 0.35
=================

Antiword is an program by Adri van Os that displays Microsoft® Word files. Here is 
a port of this program to EPOC operating system (for use in PDA Psion). It has been 
tested on "Diamond Mako".

Ported version has the following differences:

- It can output text files only. It does not support Postscript and XML output.

- It has simple console interface, which allows to select input doc-file, 
  output txt-file and screen width.

- By default, screen width is set to 0x7FFFFF (decimal 8388607), which allows generate 
  text file without wrapping inside paragraphs (except paragraphs with greater length).

- It can not display hidden (by Word) text.

- It always read character mapping information from "encoding.txt", located in "C:\Antiword" 
  directory or in the directory with "Antiword.exe".

This program is distributed under the GNU General Public License - see the
accompanying COPYING file for more details.

Installation
============

1. On your Psion or connected desktop, launch the file "Stdlib.sis". This will install 
   "C Standart Library" to your Psion. 
   Some programs (such as Opera) is also requires this library, so if you are sure, that 
   it is already installed, you can skip this step.

2. Copy file "antiword.exe" into any folder on your Psion.

3. Copy the necessary character mapping file from accompanying "Resources" folder into the folder 
   with "antiword.exe" or into "C:\Antiword\". For russian documents, mapping file 
   has name "cp1251.txt".

4. Rename copied file to "encoding.txt".

5. Now you can launch "antiword.exe" by selecting it and pressing <Enter>.

Contacts
========
Most recent version of original Antiword for many other platforms you can find here:

==>>  http://www.winfield.demon.nl/index.html  <<==
==>>  http://antiword.cjb.net/  <<==
