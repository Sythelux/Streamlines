/***************************************************************************/
/***************************************************************************/
/***                                                                     ***/
/***                   CLI macros to simplify things                     ***/
/***                                                                     ***/
/***************************************************************************/
/***************************************************************************/

/*
Copyright (c) 1996 The University of North Carolina.  All rights reserved.   
  
Permission to use, copy, modify and distribute this software and its   
documentation for any purpose is hereby granted without fee, provided   
that the above copyright notice and this permission notice appear in   
all copies of this software and that you do not sell the software.   
  
The software is provided "as is" and without warranty of any kind,   
express, implied or otherwise, including without limitation, any   
warranty of merchantability or fitness for a particular purpose.   
*/


#define  START_CLI(prompt, extension)                  \
                                                      \
         do {                                         \
           read_command (prompt);                     \
           if (cli_command ("help")) ;                \
           else if (cli_command ("echo  on/off")) {   \
             set_echo();                              \
           }                                          \
           else if (cli_command ("read  filename")) { \
             open_cli_file (extension);               \
           }


#define  END_CLI(error, bye)                     \
                                                \
           else if (! cli_command ("exit"))     \
             none_of_the_above (error);         \
         } while (! no_more_commands (bye));


#define  COMMAND(desc)  else if (cli_command (desc))


#define  START_SUBCLI(prompt, extension)                \
                                                       \
         do {                                          \
           read_command (prompt);                      \
           if (cli_command ("help")) ;                 \
           else if (cli_command ("read  filename")) {  \
             open_cli_file (extension);                \
           }


#define  END_SUBCLI(error, bye)                  \
                                                \
           else if (! cli_command ("exit"))     \
             none_of_the_above (error);         \
         } while (! no_more_commands (bye));


#define  COMMAND_SET(set)  else if (set()) ;

#define  START_SET  if (0) ;

#define  END_SET    else return (0);    \
                  return (1);

/* external declarations */

int file_is_open();

void add_extension(char *, char *);

void get_file_name(char *, char *);

void open_cli_file(char *);

void set_echo();

void read_command(char *);

[[maybe_unused]] char *get_last_match();

int cli_command(char *);

int get_parameter(char *);

[[maybe_unused]] int get_string(char *);

int get_boolean();

int get_integer(int *);

int get_real(float *);

[[maybe_unused]] int get_double(double *);

void none_of_the_above(char *);

int no_more_commands(char *);

[[maybe_unused]] void begin_audit(char *);

[[maybe_unused]] void end_audit();

[[maybe_unused]] void set_echo_filename(int);

