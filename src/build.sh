#!/bin/bash
printf "#define IMGCONV_BUILD_DATE \"%s\"\n" "`date`" > build.h.tmp
printf "#define IMGCONV_BUILD_REV \"%s\"\n" `grep BUILD_REV build.h | sed -e "s,^[^\"]*\",0," -e "s,\".*$,+1," | bc` >> build.h.tmp
mv build.h.tmp build.h
