#!/bin/bash
printf "#define IMGCONV_BUILD_DATE \"%s\"\n" "`date`" > build.h.tmp
printf "#define IMGCONV_BUILD_REV \"%s\"\n" `git log -1 --format="%H"` >> build.h.tmp
mv build.h.tmp build.h
