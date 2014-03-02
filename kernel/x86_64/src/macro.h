#ifndef __X86_64_MACRO_H__
#define __X86_64_MACRO_H__

// Contains some useful pre-processor macros.
// This file is also the location of any CPP macros
// that are needed in multiple files.

// http://stackoverflow.com/questions/5641427/how-to-make-preprocessor-generate-a-string-for-line-keyword
// Converts a number literal to a string at compile time
#define S(X) #X
#define SX(X) S(X)
#define S__LINE__ SX(__LINE__)

#endif
