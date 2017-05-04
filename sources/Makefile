CFLAGS += -O3 -Wstrict-prototypes -fomit-frame-pointer -g3 -Wall
libdir = /usr/lib
LDFLAGS += -pthread -rpath $(libdir) -version-info 1
CC = gcc
CXX = g++
LD = ld
ARCH = $(shell uname -m)

# Check Architecture
SUPPORTED_ARCH = NO

ifeq ($(ARCH), x86_64)
SUPPORTED_ARCH = YES
endif

ifeq ($(SUPPORTED_ARCH), NO)
$(error Your architecture $(ARCH) is not currently supported. See README.)
endif

define compile_rule
	libtool --mode=compile --tag=CC \
	$(CC) $(CFLAGS) $(CPPFLAGS) -Iincludes -c $<
endef

define cxx_compile_rule
	libtool --mode=compile --tag=CC \
	$(CXX) $(CFLAGS) $(CPPFLAGS) -Iincludes -c $<
endef

define link_rule
	libtool --mode=link --tag=CC \
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endef

LIBS = libcmalloc.la
libcmalloc_OBJS = $(subst .c,.lo,$(wildcard *.c))

%.lo: %.c
	$(call compile_rule)
%.lo: %.cpp
	$(call cxx_compile_rule)

# build and install
all: libcmalloc.la

libcmalloc.la: $(libcmalloc_OBJS)
	$(call link_rule)
	cp .libs/libcmalloc.so ./
	cp .libs/libcmalloc.a ./

install/%.la: %.la
	libtool --mode=install \
	install -c $(notdir $@) $(libdir)/$(notdir $@)

install: $(addprefix install/,$(LIBS))
	libtool --mode=finish $(libdir)

clean:
	libtool --mode=clean rm *.la *.lo *.a *.so -f

