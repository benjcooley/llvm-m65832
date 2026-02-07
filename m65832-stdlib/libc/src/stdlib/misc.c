#include <stdio.h>

/* Global errno */
int errno = 0;

/* Standard FILE streams */
static FILE _stdin_file = { 0 };
static FILE _stdout_file = { 1 };
static FILE _stderr_file = { 2 };

FILE *stdin = &_stdin_file;
FILE *stdout = &_stdout_file;
FILE *stderr = &_stderr_file;
