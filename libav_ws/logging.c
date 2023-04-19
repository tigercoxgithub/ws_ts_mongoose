#include "logging.h" // Include the header file that contains the function declaration

#include <stdarg.h> // for va_list
#include <stdio.h> // for fprintf

void logging(const char *fmt, ...)
{
	va_list args;
	fprintf(stderr, "LOG: ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

