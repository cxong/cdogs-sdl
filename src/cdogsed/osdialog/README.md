# osdialog

A cross platform wrapper for OS dialogs like file save, open, message boxes, inputs, color picking, etc.

Currently supports MacOS, Windows, and GTK2/GTK3 on Linux.

osdialog is released into the public domain [(CC0)](LICENSE.txt).
If you decide to use osdialog in your project, [please let me know](https://github.com/AndrewBelt/osdialog/issues/9).

## Using

The Makefile is only for building the osdialog test binary.
You don't need to use it for your application, but it might be helpful for suggesting compiler flags.
Simply add `osdialog.h` to your include directory, and add `osdialog.c` and the appropriate `osdialog_*.c/.m` file to your application's sources.
