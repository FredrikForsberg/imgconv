# imgconv
Tiny image converter with no dynamic linkings (super fast start up) written in C.

## Building
Run <code>./configure && make</code> - This will also download and build static libraries (zlib, libpng, libjpeg), so a internet connection is needed. If everyting succeded you'll have a imgconv binary in root folder.

## Usage

### Transcoding
<code>imgconv [-codec RGB|RGBA|PNG] [-dimension WIDTHxHEIGHT] -input FILE [-codec RGB|RGBA|JPEG|PNG] [-quality 0..100] -output FILE</code>

-dimension argument is only for data dump formats RGB/RGBA

-quality argument is only for JPEG compression

### Image information
<code>imgconv -info [-codec RGB|RGBA|PNG] -input FILE</code>

## Supported formats
Currently only supported file formats are JPEG and PNG. Data dump formats are RGB (8+8+8bits) and RGBA (8+8+8+8bits)
