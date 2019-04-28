all:
	@make -f scripts/libs.mk
	@cd src && make

clean:
	@find . -iname "*.o" -exec rm {} \;
	@find . -iname "*~" -exec rm {} \;
	@find . -iname "*.ds_store" -exec rm {} \;
	@find . -iname "*#*#" -exec rm {} \;

clean_all: clean
	@rm -rf libsrc/ImageMagick libsrc/galib libsrc/libimagequant libsrc/libjpeg libsrc/libpng libsrc/mujs libsrc/zlib libsrc/libpngwolf/libpngwolf.a libsrc/libpngwolf/gconf.inc scripts/config.inc multitool lib

clean_downloads:
	@rm -rf downloads
