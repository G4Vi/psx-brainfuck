TARGET = brainfuck
USE_FUNCTION_SECTIONS = false
TYPE = ps-exe

CPPFLAGS = -DNOFLOATINGPOINT
CPPFLAGS += -I.
CPPFLAGS += -Inugget/third_party/uC-sdk/libc/include
CPPFLAGS += -Inugget/third_party/uC-sdk/os/include
CPPFLAGS += -IuC-sdk-glue

SRCS = \
uC-sdk-glue/BoardConsole.c \
uC-sdk-glue/BoardInit.c \
uC-sdk-glue/init.c \
\
nugget/third_party/uC-sdk/libc/src/cxx-glue.c \
nugget/third_party/uC-sdk/libc/src/errno.c \
nugget/third_party/uC-sdk/libc/src/initfini.c \
nugget/third_party/uC-sdk/libc/src/malloc.c \
nugget/third_party/uC-sdk/libc/src/qsort.c \
nugget/third_party/uC-sdk/libc/src/rand.c \
nugget/third_party/uC-sdk/libc/src/reent.c \
nugget/third_party/uC-sdk/libc/src/stdio.c \
nugget/third_party/uC-sdk/libc/src/string.c \
nugget/third_party/uC-sdk/libc/src/strto.c \
nugget/third_party/uC-sdk/libc/src/unistd.c \
nugget/third_party/uC-sdk/libc/src/xprintf.c \
nugget/third_party/uC-sdk/libc/src/xscanf.c \
nugget/third_party/uC-sdk/libc/src/yscanf.c \
nugget/third_party/uC-sdk/os/src/devfs.c \
nugget/third_party/uC-sdk/os/src/filesystem.c \
nugget/third_party/uC-sdk/os/src/fio.c \
nugget/third_party/uC-sdk/os/src/hash-djb2.c \
nugget/third_party/uC-sdk/os/src/init.c \
nugget/third_party/uC-sdk/os/src/osdebug.c \
nugget/third_party/uC-sdk/os/src/romfs.c \
nugget/third_party/uC-sdk/os/src/sbrk.c \
\
nugget/common/syscalls/printf.s \
nugget/common/crt0/uC-sdk-crt0.s \
brainfuck.c \

include nugget/common.mk

