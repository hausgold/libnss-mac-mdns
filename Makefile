DESTDIR ?=
NSSDIR ?= /usr/lib
SRC_DIR ?= src

CC = gcc
VALGRIND ?= valgrind
LDCONFIG ?= ldconfig
INSTALL ?= install
MKDIR ?= mkdir

CFLAGS = -fPIC -Wall -Werror -ggdb `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0`

MODULE = libnss_mac_mdns.so.2
BINS = \
	$(SRC_DIR)/$(MODULE) \
	$(SRC_DIR)/nss-test
OBJS = \
	$(SRC_DIR)/nss.o
TEST_OBJS = \
	$(SRC_DIR)/nss-test.o

$(SRC_DIR)/$(MODULE): $(OBJS) Makefile
	$(CC) -fPIC -shared -o $@ -Wl,-soname,$(@:$(SRC_DIR)/%=%) $< $(LDFLAGS)

test: build
	LD_LIBRARY_PATH="$(abspath $(SRC_DIR))" \
		PATH="$(abspath $(SRC_DIR)):$(PATH)" \
		$(VALGRIND) \
			--leak-check=full --error-exitcode=2 \
			--suppressions=valgrind.supp \
			--gen-suppressions=all \
			$(SRC_DIR)/nss-test

all: $(BINS) $(TEST_OBJS)
build: all

install: all
	$(MKDIR) -p $(DESTDIR)$(NSSDIR)
	$(INSTALL) -m 0644 $(SRC_DIR)/$(MODULE) $(DESTDIR)$(NSSDIR)/$(MODULE)
	$(LDCONFIG)

clean:
	rm -f $(BINS) $(OBJS) $(TEST_OBJS)

.PHONY: all install clean
.DEFAULT_GOAL := all
