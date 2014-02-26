FOR_MDEV := 0
DESTDIR  :=

_ALL_TARGETS := diskid

# control build behavior: static and/or minimal executable?
ifeq ($(FOR_MDEV),$(filter $(FOR_MDEV),y Y 1 yes YES true TRUE))
	SBIN    := /lib/mdev
	STATIC  := 1
	MINIMAL := 1
	_ALL_TARGETS += create_diskid_links.sh
else
	SBIN    := /sbin
	STATIC  := 0
	MINIMAL := 0
endif

_SBIN := $(DESTDIR)$(SBIN)
X_DISKID := $(SBIN)/diskid


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


PHONY :=

PHONY += all
all: $(_ALL_TARGETS)

diskid: $(COMMON_OBJECTS) $(DISKID_OBJECTS)
	$(LINK_O) $^ -o $@

ata_id: $(COMMON_OBJECTS) $(ATAID_OBJECTS)
	$(LINK_O) $^ -o $@

%.sh: %.sh.in
	sed -e "s|@@X_DISKID@@|$(X_DISKID)|g" $< > $@.make_tmp
	sh -n $@.make_tmp
	chmod +x $@.make_tmp
	mv -f -- $@.make_tmp $@

$(O):
	mkdir -p $(O)

$(O)/%.o: $(SRCDIR)/%.c | $(O)
	$(COMPILE_C) $< -o $@

PHONY += clean
clean:
	-rm -f -- $(COMMON_OBJECTS) $(DISKID_OBJECTS) $(ATAID_OBJECTS) diskid ata_id
	-rmdir $(O)


# install targets
PHONY += install
install:
	install -d -m 0755 -- $(_SBIN)
	install -m 0755 -t $(_SBIN) -- $(_ALL_TARGETS)


PHONY += uninstall
uninstall:
	rm -f -- $(addprefix $(_SBIN)/, $(_ALL_TARGETS))


PHONY += help
help:
	@echo  'Targets:'
	@echo  '  all           - build all targets marked with [*]'
	@echo  '  clean         - remove generated files'
	@echo  '  install       - install diskid to DESTDIR/SBIN'
	@echo  '                  (default: $(_SBIN))'
	@echo  '  uninstall     -'
	@echo  '* diskid        - build diskid'
	@echo  '  ata_id        - build ata_id'
	@echo  ''
	@echo  'Options/Vars:'
	@echo  '  MINIMAL=0|1   - whether to build a minimal variant of diskid/ata_id'
	@echo  '                  (default: $(MINIMAL))'
	@echo  '  STATIC=0|1    - whether to build a static variant of diskid/ata_id'
	@echo  '                  (default: $(STATIC))'
	@echo  '  DESTDIR, SBIN - paths for [un]install'
	@echo  '  O             - build dir'
	@echo  '                  (default: $(O))'
	@echo  '  FOR_MDEV=0|1  - use mdev-specific defaults:'
	@echo  '                  * SBIN    = /lib/mdev'
	@echo  '                  * MINIMAL = 1'
	@echo  '                  * STATIC  = 1'
	@echo  '                  Also, build/install create_diskid_links.sh'
	@echo  '                  (default: $(FOR_MDEV))'

.PHONY: $(PHONY)
