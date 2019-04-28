include scripts/config.inc

all: lib/libjpeg.a lib/libz.a lib/libpng.a

#######################
###     LIBJPEG     ###
#######################
downloads/jpegsrc.v9c.tar.gz:
	@mkdir -p downloads
	@echo "Fetching $@"
	@$(DL) $@ "http://www.ijg.org/files/jpegsrc.v9c.tar.gz"
libsrc/libjpeg: downloads/jpegsrc.v9c.tar.gz
	@echo "Extracting $<..."
	@mkdir -p libsrc
	@tar -xf $< -C libsrc
	@mv libsrc/jpeg-9c $@
	@touch $@
lib/libjpeg.a: libsrc/libjpeg
	@echo "Building $@..."
	@mkdir -p lib
	@cd $< \
	&& ./configure --enable-shared=no --enable-static=yes \
	&& make
	@cp -v $</.libs/libjpeg.a $@
	@touch $@

####################
###     ZLIB     ###
####################
downloads/zlib-1.2.11.tar.gz:
	@mkdir -p downloads
	@echo "Fetching $@"
	@$(DL) $@ "https://www.zlib.net/zlib-1.2.11.tar.gz"
libsrc/zlib: downloads/zlib-1.2.11.tar.gz
	@echo "Extracting $<..."
	@mkdir -p libsrc
	@tar -xf $< -C libsrc
	@mv libsrc/zlib-1.2.11 $@
	@touch $@
lib/libz.a: libsrc/zlib
	@echo "Building $@..."
	@mkdir -p lib
	@cd $< \
	&& ./configure --static \
	&& make
	@cp -v $</libz*.a $@
	@touch $@

######################
###     LIBPNG     ###
######################
downloads/libpng-1.6.35.tar.gz:
	@mkdir -p downloads
	@echo "Fetching $@"
	@$(DL) $@ "https://download.sourceforge.net/libpng/libpng-1.6.35.tar.gz"
libsrc/libpng: downloads/libpng-1.6.35.tar.gz
	@echo "Extracting $<..."
	@mkdir -p libsrc
	@tar -xf $< -C libsrc
	@mv libsrc/libpng-1.6.35 $@
	@touch $@
lib/libpng.a: libsrc/libpng libsrc/zlib
	@echo "Building $@..."
	@mkdir -p lib
	@cd $< \
	&& ./configure \
		LDFLAGS=-L../../lib CPPFLAGS=-I../zlib \
		--enable-shared=no --enable-static=yes \
	&& make
	@cp -v $</.libs/libpng*.a $@
	@touch $@

