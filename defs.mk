PLATFORM=x86_64
CC=gcc
LD=ld

override CFLAGS := -Wall -Werror -Wextra -pedantic -std=c99 $(CFLAGS)
