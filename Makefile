DESTDIR :=
SBIN    := $(DESTDIR)/sbin

# control build behavior: static and/or minimal executable?
STATIC  ?= 0
MINIMAL ?= 0

# default -W... flags for CFLAGS
_WARNFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter
_WARNFLAGS += -Wwrite-strings -Wdeclaration-after-statement
_WARNFLAGS += -Wtrampolines
_WARNFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
CFLAGS         ?= -Os -pipe $(_WARNFLAGS)
CPPFLAGS       ?=
CC             := gcc
LDFLAGS        ?= -Wl,-O1 -Wl,--as-needed
TARGET_CC      := $(CROSS_COMPILE)$(CC)
EXTRA_CFLAGS   ?=

O              := ./build
SRCDIR         := ./src
COMMON_OBJECTS := $(addprefix $(O)/,udev_util.o disk_type.o ata_id.o)
ATAID_OBJECTS  := $(addprefix $(O)/,ata_id_main.o)
DISKID_OBJECTS := $(addprefix $(O)/,main.o)


CFLAGS   += $(EXTRA_CFLAGS)
CC_OPTS  :=
CC_OPTS  += -std=gnu99
# _GNU_SOURCE should be set
CPPFLAGS += -D_GNU_SOURCE

ifeq ($(STATIC),$(filter $(STATIC),y Y 1 yes YES true TRUE))
	CC_OPTS += -static
endif

ifeq ($(MINIMAL),$(filter $(MINIMAL),y Y 1 yes YES true TRUE))
	CPPFLAGS += -DENABLE_MINIMAL
endif


COMPILE_C = $(TARGET_CC) $(CC_OPTS) $(CPPFLAGS) $(CFLAGS) -c
LINK_O    = $(TARGET_CC) $(CC_OPTS) $(CPPFLAGS) $(LDFLAGS)


.PHONY =

all: diskid

diskid: $(COMMON_OBJECTS) $(DISKID_OBJECTS)
	$(LINK_O) $^ -o $@

ata_id: $(COMMON_OBJECTS) $(ATAID_OBJECTS)
	$(LINK_O) $^ -o $@

$(O):
	mkdir -p $(O)

$(O)/%.o: $(SRCDIR)/%.c | $(O)
	$(COMPILE_C) $< -o $@

.PHONY += clean
clean:
	-rm -f -- $(COMMON_OBJECTS) $(DISKID_OBJECTS) $(ATAID_OBJECTS) diskid ata_id
	-rmdir $(O)


# install targets
.PHONY += install
install:
	install -d -m 0755 -- $(SBIN)
	install -m 0755 -t $(SBIN) -- diskid
