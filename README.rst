=====================
 diskid - 26.11.2013
=====================

Introduction
============

`diskid` reads disk identifiers from devices and is a replacement for udev's
`ata_id` program. This is useful for creating /dev/disk/by-id/ links on systems
where udev/systemd is not installed, e.g. when /dev is managed by mdev or
devtmpfs.

Some differences to udev's `ata_id`:

* unofficial, unsupported
* does not depend on libudev
* accepts the ``--mdev/-m`` option, which is a very reduced variant of ``--export``
* is able to process more than one device node:

  * in ``--export``/``--mdev`` mode, all variables are prefixed with
    ``<DEVICE NAME>_``, e.g. ``SDA_ID_SERIAL=...``
  * in the default mode, all disk ids are prefixed with ``<device path>:``,
    e.g. ``/dev/sda:<disk id>``


Building diskid
===============

Dependencies:

* GNU Make
* a C Compiler (gcc preferred)
* libc (tested with glibc 2.15; uclibc to follow)


Simply run ``make`` or ``make diskid``.

``MINIMAL=1`` may be passed to ``make`` if the full output of the
``--export`` option is not required, which saves some space.
Likewise, ``STATIC=1`` instructs ``make`` to do a static build::

   $ make MINIMAL=1 STATIC=1 diskid

The `ata_id` program, which (mostly) behaves like its original, can be built with ``make ata_id``.

When cross-compiling, ``CROSS_COMPILE`` or ``TARGET_CC`` should be set.


Installing diskid
=================

* Install diskid to ``$SBIN/diskid`` for system-wide access
  (``$SBIN=$DESTDIR/sbin``, ``$DESTDIR`` is empty by default)::

     $ make install

* Install diskid for mdev only, assuming your mdev scripts expect to find it
  at `/lib/mdev/diskid`::

     $ make SBIN=/lib/mdev


Running it
==========

Usage::

   $ diskid [-h,--help] [-x,--export] [-m,--mdev] <device> [<device>...]
   $ ata_id [-h,--help] [-x,--export] <device>

Options:

-h, --help
   print the help message

-x, --export
   output (many) environment variables

-m, --mdev
   output environment variables required for setting up ``/dev/disk/by-id``
   (`ID_BUS`, `ID_SERIAL`, `ID_WWN_WITH_EXTENSION`)


Note that the output of ``--export`` is identical to ``--mdev``
if diskid has been built with ``MINIMAL=1``.


----------
 Examples
----------

Get a device's disk id::

   $ diskid /dev/sda

Get several disk ids::

   $ diskid /dev/sd?

Get disk info as environment variables::

   $ diskid --export /dev/sda
   ## or
   $ diskid --export /dev/sd?

set `ID_BUS`, `ID_SERIAL` and `ID_WWN_WITH_EXTENSION` in your current shell::

   $ eval "$(diskid --mdev /dev/sda)"
