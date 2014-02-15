PLATFORM=x86_64
CC=gcc
override CFLAGS := -Wall -Werror -Wextra -pedantic -std=c99 $(CFLAGS)
