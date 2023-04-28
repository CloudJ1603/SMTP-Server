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
extern char *trim_angle_brackets(char *name);

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
extern int   split(char *buf, char *parts[]);

extern int   be_verbose;
/**
 * Print a log message to the standard error stream, if be_verbose is 1
 *
 * Parameters: fmt:     A printf-line formating string
 *
 * The __attribute__ in this function allows the compiler to provide
 * useful warnings when compiling the code.
 **/
extern void  dlog(const char *fmt, ...)
		__attribute__ ((format(printf, 1, 2)));

