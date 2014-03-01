#ifndef __X86_64_KPRINTF_H__
#define __X86_64_KPRINTF_H__

/* Standard printf() function. The first argument 
 * is a string, the rest of the arguments are the
 * things to format. Does some error checking, but
 * may not be super robust.
 *
 * To specify maximum string output length use the
 * the letter 'l' followed by the length followed
 * by 's'. For example to print the first 10
 * characters of a string use the format string
 *
 *   "%l10s"
 *
 * Length and padding can also be used with this
 * new format specifier. This format specifier is
 * only valid for strings. When given a null
 * terminated string the null character takes
 * precedence and printing will cease when the
 * character is encountered.
 *
 * Format syntax:
 *
 *   %[-][0][width][](c|d|u|s|x|o|%|l[digit]s)
 *
 * -       => Left adjust
 * 0       => Pad with zeros
 * width   => Amount of padding
 * c       => Character
 * d       => Decimal number
 * u       => Unsigned decimal number
 * b       => Binary number
 * s       => String
 * l[num]s => Print num characters
 * x       => Hexidecimal number
 * o       => Octal number
 * %       => Print the percent character
 */
void kprintf(const char* format, ...);

#endif
