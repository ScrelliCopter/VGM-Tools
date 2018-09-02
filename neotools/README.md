Neo-Geo VGM tools
-----------------

A hodge-podge of tools for working on NG VGM files,
these files are provided in the hope that they may be useful to someone.

Makefiles have currently not been tested but shouldn't be hard to get working.
Windows binaries are available under the Releases tab.

Included tools (sources included).
 - **adpcm.exe**:
    ADPCM Type-A to WAV converter.
 - **adpcmb.exe**:
    ADPCM Type-B encoding tool that also does decoding to WAV.
 - **neoadpcmextract.exe**:
    Scans a .vgm and dumps all ADPCM type A&B data to raw .pcm files.
 - **autoextract.bat**:
    Convenience batch script that uses the above tools to dump all samples to WAVs.

How to use
----------
Copy your .vgz into this directory, drag it onto autoextract.bat,
the script will create a directory of the same name and dump all the converted samples there.

That's all there is to it.

TODO
----

 - Replace the batch script with something less shite (and actually cross-platform).
 - Write a makefile for neoadpcmextract.
 - Make this stuff build on Linux.
