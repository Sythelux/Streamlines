/*

Command Language Interpreter.

Greg Turk

---------------------------------------------------------------------

Copyright (c) 1996 The University of North Carolina.  All rights reserved.   
  
Permission to use, copy, modify and distribute this software and its   
documentation for any purpose is hereby granted without fee, provided   
that the above copyright notice and this permission notice appear in   
all copies of this software and that you do not sell the software.   
  
The software is provided "as is" and without warranty of any kind,   
express, implied or otherwise, including without limitation, any   
warranty of merchantability or fitness for a particular purpose.   

*/

#include <stdio.h>
#include <strings.h>
#include <iostream>
#include <stdlib.h>
#include "cli.h"
#include <cstring>

extern char *calloc();

#define  toupper(c)  (((c) >= 'a' && (c) <= 'z') ? (c) - 'a' + 'A' : (c))
#define  tolower(c)  (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

#define  TRUE  1
#define  FALSE 0
#define  MAX_STRING 1024

typedef char String [MAX_STRING];

typedef struct afile { FILE *fp;
                       struct afile *next;
                     } File, *File_ptr;

static File_ptr current_file = NULL;   /* pointer to list of input files */

static String input_line;       /* current line being processed */
static String the_command;      /* first parameter in command line */
static char *line_ptr;          /* active position of input line */
static String last_match;       /* last word matched by 'match' routine */

static int input_done = 0; /* flag set if there is no more input */
static int help_on = 0;    /* flag set if we are in help mode */
static int comment_on = 0; /* flag set if current line is a comment */
static int echo_on = 0;    /* flag set if lines from file should be printed */
static int audit_on = 0;   /* flag set if we're writing an audit file */
static int echo_name = 1;  /* flag set if filename printed when read in */

static FILE *audit_file;   /* audit file pointer */

static void open_file (char *);
static void close_file();
static int get_command (char *, char *, char *);
static void process_command (char *, char *);
static int match (char *);


/******************************************************************************
Returns 1 if a CLI input file is open, 0 otherwise.
******************************************************************************/

int file_is_open()
{
  return (current_file != NULL);
}


/******************************************************************************
Add an extension to a filename if none already exists.

Entry:
  extension - extension to add if necessary
  name      - name to add to

Exit:
  name - possibly modified file name
******************************************************************************/

void add_extension (char *name, char *extension)
{
  char *pos;

  for (pos = name; *pos != '.' && *pos != '\0'; pos++) ;

  if (*pos != '.') {
    *pos++ = '.';
    while (*extension != '\0')
      *pos++ = *extension++;
    *pos = '\0';
  }
}


/******************************************************************************
Read a file name from the current input line.

Entry:
  extension - default file extension to use

Exit:
  name - file name from the input line
******************************************************************************/

void get_file_name (char *name, char *extension)
{
  get_parameter (name);
  add_extension (name, extension);
  if (echo_name)
    printf ("File name : %s.\n", name);
}


/******************************************************************************
Get a file name from the current parameter and have commands read from the
file.
******************************************************************************/

void open_cli_file (char *extension)
{
  String filename;

  get_file_name (filename, extension);
  open_file (filename);
}


/******************************************************************************
Open a CLI file to read commands from.

Entry:
  name - name of file to open
******************************************************************************/

static void open_file (char *name)
{
  File_ptr new_file;
  FILE *fp;

  fp = fopen (name, "r");

  if (fp == NULL)
    printf ("File %s could not be opened.\n", name);
  else {
    new_file = (File_ptr) calloc (1, sizeof (File));
    new_file->next = current_file;
    new_file->fp = fp;
    current_file = new_file;
  }
}


/******************************************************************************
Close the current CLI input file.
******************************************************************************/

static void close_file()
{
  fclose (current_file->fp);
  current_file = current_file->next;
}


/******************************************************************************
Get a command line from the standard input or a file.

Entry:
  prompt    - prompt for printing if input is from keyboard
  extension - extension to prompt

Exit:
  line - command string that was read
  returns 1 if a command line was read, a 0 if there were no lines to read
******************************************************************************/

static int get_command (char *line, char *prompt, char *extension)
{
  char *pos;
  char *c_status;
  int status;

  if (file_is_open()) {

    c_status = fgets (line, MAX_STRING, current_file->fp);
    if (c_status == NULL)
      status = NULL;
    else
      status = 1;

    /* get rid of newline character at end of line */

    for (pos = line; *pos != '\0'; pos++)
      if (*pos == '\n')
        *pos = '\0';

    /* if we are at EOF, get a line elsewhere */

    if (status == NULL) {
      close_file();
      status = get_command (line, prompt, extension);
    }
    else if (echo_on)
      printf ("%s>%s%s\n", prompt, extension, line);
  }
  else {
    printf ("%s%s", prompt, extension);

    c_status = fgets (line, MAX_STRING, stdin);
    if (c_status == NULL)
      status = NULL;
    else
      status = 1;

    if (status == NULL)
      input_done = TRUE;
  }

  /* write command line to audit file if auditing */

  if (audit_on && status)
    fprintf (audit_file, "%s\n", line);

  return (status);
}


/******************************************************************************
Remove commas from a line and read extra lines if there is a continuation
symbol "&".

Entry:
  line   - line to process
  prompt - input prompt in case more lines need to be read

Exit:
  line - the processed line
******************************************************************************/

static void process_command (char *line, char *prompt)
{
  char *pos;            /* position in line */
  char *pos_extra;      /* position in extra line */
  String extra;         /* extra line for reading continuation */

  /* replace commas with blanks */

  for (pos = line; *pos != '\0'; pos++)
    if (*pos == ',')
      *pos = ' ';

  /* look for continuation symbol */

  for (pos = line; *pos != '\0' && *pos != '&'; pos++) ;

  /* get an extra line if neccessary */

  if (*pos == '&' && get_command (extra, prompt, "--")) {
    process_command (extra, prompt);
    *pos++ = ' ';
    for (pos_extra = extra; *pos_extra != '\0'; pos++, pos_extra++)
      *pos = *pos_extra;
    *pos = '\0';
  }
}


/******************************************************************************
Get parameter from the current line and turn on or off the file echo
accordingly.
******************************************************************************/

void set_echo()
{
  echo_on = get_boolean();
}


/******************************************************************************
Read a command line from the standard input or a file.

Entry:
  prompt - prompt for printing if input is from keyboard
******************************************************************************/

void read_command (char *prompt)
{
  int got_one;

  got_one = get_command (input_line, prompt, "> ");

  if (! got_one)
    return;

  process_command (input_line, prompt);

  line_ptr = input_line;
  get_parameter (the_command);

  /* set various flags */

  input_done = input_done || match ("exit") || match (".");
  help_on = match ("help") || match ("?") || match ("/");
  comment_on = (input_line [0] == ' ')
            || (input_line [0] == '\0')
            || (input_line [0] == '!');
}


/******************************************************************************
Sees if a word matches the current input line.

Entry:
  word - command word to try to match

Exit:
  returns 1 if word matches, 0 otherwise
******************************************************************************/

static int match (char *word)
{
  char *save_word;    /* save the word */
  char *pos;          /* position in first parameter of current command */

  /* see how far the command word and command line are similar */

  save_word = word;
  pos = the_command;

  while (toupper (*pos) == toupper (*word) && *pos != '\0') {
    pos++;
    word++;
  }

  if (*pos == '\0' && pos != the_command) {
    strcpy (last_match, save_word);
    return (1);
  }
  else
    return (0);
}


/******************************************************************************
Returns pointer to last command matched.
******************************************************************************/

char *get_last_match()
{
  return (last_match);
}


/******************************************************************************
Processes a request to match a command word with the command line.

Entry:
  word - command word to try to match

Exit:
  returns 1 if word matches, 0 otherwise
******************************************************************************/

int cli_command (char *word)
{
  /* if there is no input to match or we are in comment mode, return */

  if (input_done || comment_on)
    return (0);

  /* if we are in help mode, print the command word and leave */

  if (help_on) {
    printf ("%s\n", word);
    return (0);
  }

  /* see if the command word matches the first word of the command line */

  return (match (word));
}


/******************************************************************************
Creates a string containing the next parameter in a command line.

Exit:
  param - the next parameter (contains no spaces)
  returns 1 if there was a parameter to return, 0 otherwise
******************************************************************************/

int get_parameter (char *param)
{
  int count;

  /* get rid of leading spaces */

  while (*line_ptr == ' ')
    line_ptr++;

  /* copy characters from input line to parameter string */

  count = 0;

  while (*line_ptr != ' ' && *line_ptr != '\0') {
    *param++ = *line_ptr++;
    count++;
  }

  *param = '\0';

  return (count != 0);
}


/******************************************************************************
Returns the rest of the command line as a string

Exit:
  string - the remainder of the command line
  returns 1 if there was anything to return, 0 otherwise
******************************************************************************/

int get_string (char *string)
{
  int count;

  /* get rid of leading spaces */

  while (*line_ptr == ' ')
    line_ptr++;

  /* copy characters from input line to the string */

  count = 0;

  while (*line_ptr != '\0') {
    *string++ = *line_ptr++;
    count++;
  }

  *string = '\0';

  return (count != 0);
}


/******************************************************************************
Returns whether the next parameter on the command line indicates a boolean
"true" or "false".

Exit:
  returns 0 if next parameter matches "0", "off", "no", "false" or
  if there is no parameter, otherwise returns 1
******************************************************************************/

int get_boolean()
{
  char s [80];
  char *pos;

  /* return FALSE if there is no parameter */

  if (! get_parameter (s))
    return (FALSE);

  /* convert string to lower case */

  for (pos = s; *pos != '\0'; pos++)
    *pos = tolower (*pos);

  /* now see what it matches */

  if (! strcmp (s, "0"))
    return (FALSE);
  else if (! strcmp (s, "off"))
    return (FALSE);
  else if (! strcmp (s, "false"))
    return (FALSE);
  else if (! strcmp (s, "no"))
    return (FALSE);
  else
    return (TRUE);
}


/******************************************************************************
Return an integer value from a command line.

Exit:
  n - integer from command line
  returns 1 if there was a parameter to convert to integer, 0 otherwise
******************************************************************************/

int get_integer (int *n)
{
  String s;
  int got_one;     /* did we get a parameter? */

  got_one = get_parameter (s);

  *n = 0;
  if (got_one)
    sscanf (s, "%d", n);

  return (got_one);
}


/******************************************************************************
Return a floating point value from a command line.

Exit:
  r - float from command line
  returns 1 if there was a parameter to convert to float, 0 otherwise
******************************************************************************/

int get_real (float *r)
{
  String s;
  int got_one;     /* did we get a parameter? */

  got_one = get_parameter (s);

  *r = 0.0;
  if (got_one)
    sscanf (s, "%f", r);

  return (got_one);
}


/******************************************************************************
Return a double value from a command line.

Exit:
  r - double from command line
  returns 1 if there was a parameter to convert to double, 0 otherwise
******************************************************************************/

int get_double (double *r)
{
  String s;
  int got_one;     /* did we get a parameter? */

  got_one = get_parameter (s);

  *r = 0.0;
  if (got_one)
    sscanf (s, "%lf", r);

  return (got_one);
}


/******************************************************************************
Error routine that should be called if no command matches could be found.

Entry:
  message - error message to print
******************************************************************************/

void none_of_the_above (char *message)
{
  if ((! input_done) && (! help_on) && (! comment_on))
    printf ("%s\n", message);
}


/******************************************************************************
Prints a goodbye message if there are no more commands to interpret.

Entry:
  message - goodbye message to print

Exit:
  returns 1 if there are no more commands, 0 otherwise
******************************************************************************/

int no_more_commands (char *message)
{
  int result;

  if (input_done)
    printf ("%s\n", message);

  result = input_done;
  input_done = 0;        /* reset flag in case we are exiting a submenu */
  return (result);
}


/******************************************************************************
Open an audit file.  While file is open, all user commands will be written to
this file.
******************************************************************************/

void begin_audit (char *filename)
{
  /* exit if audit file already open */

  if (audit_on) {
    printf ("Audit file already open.\n");
    return;
  }

  audit_file = fopen (filename, "w");

  if (audit_file == NULL) {
    printf ("Error opening audit file.\n");
    audit_on = FALSE;
  }
  else
    audit_on = TRUE;
}


/******************************************************************************
Close the audit file.
******************************************************************************/

void end_audit()
{
  if (audit_on)
    fclose (audit_file);

  audit_on = FALSE;
}


/******************************************************************************
Turn on or off filename echo.

Entry:
  flag - whether to echo file (0 = don't echo, 1 = do echo)  
******************************************************************************/

void set_echo_filename (int flag)
{
  echo_name = flag;
}

