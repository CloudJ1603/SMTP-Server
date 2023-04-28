#include "util.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/** Remove any leading and trailing < > brackets around name
 *
 * Parameters: name:  The name from which to remove any brackets
 *
 * Returns: Either the orginal name if it is not surrounded by 
 * brackets, or the name without the brackets.
 *
 * Examples:
 *     trim_angle_brackets("norm@cs.ubc.ca") returns "norm@cs.ubc.ca"
 *     trim_angle_brackets("<norm@cs.ubc.ca>") returns "norm@cs.ubc.ca"
 *     trim_angle_brackets("<norm@cs.ubc.ca") returns "<norm@cs.ubc.ca"
 **/
char *trim_angle_brackets(char *name) {
   if (name[0] == '<' && name[strlen(name) - 1] == '>') {
	name[strlen(name) - 1] = '\0';
	return &name[1];
    }
    return name;
}

/**
 *  Split a line into individual parts separated by white space
 *
 *  Parameters: line:   The line of text to split
 *                      The characters in the line will be modified by the call.
 *              parts:  An array of char * pointers that will receive the
 *                      pointers to the individual parts of the input line. 
 *                      This array must be long enough to hold all of the
 *                      parts of the line.
 **/
int split(char *buf, char *parts[]) {
    static char *spaces = " \t\r\n";
    int i = 1;
    parts[0] = strtok(buf, spaces);
    do {
        parts[i] = strtok(NULL, spaces);
    } while (parts[i++] != NULL);
    return i - 1;
}

int be_verbose = 1;
/**
 * Print a log message to the standard error stream, if be_verbose is 1
 *
 * Parameters: fmt:     A printf-line formating string
 *
 **/
void dlog(const char *fmt, ...) {
    va_list args;
    if (be_verbose) {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
}
