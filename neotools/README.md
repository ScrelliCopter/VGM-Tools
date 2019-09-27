Neo-Geo VGM tools
=================

A hodge-podge of tools for working on NG VGM files,
these files are provided in the hope that they may be useful to someone.

Included tools (sources included).
 - **adpcm**:
    ADPCM Type-A to WAV converter.
 - **adpcmb**:
    ADPCM Type-B encoding tool that also does decoding to WAV.
 - **neoadpcmextract**:
    Scans a .vgm and dumps all ADPCM type A&B data to raw .pcm files.
 - **autoextract**:
    Convenience shell/batch script that uses the above tools to dump all samples to WAVs.

Building
--------
Linux:
```shell script
make -f adpcm.Makefile
make -f adpcmb.Makefile
make -f neoadpcmextract.Makefile
```

Windows:
- The updated Makefiles haven't been tested on Windows yet.
- Working Windows binaries are available from the [Releases](https://github.com/ScrelliCopter/VGM-Tools/releases) tab.

How to use
----------
Linux:
 - Run `./autoextract.sh` from this directory with a path to your .vgm or .vgz,
   a directory for the song will be created containing wav samples.

Windows:
 - You will need gzip.exe (provided with the aforementioned release).
 - Copy your .vgm or .vgz into this directory, drag it onto autoextract.bat,
   the script will create a directory of the same name and dump all the converted samples there.

That's all there is to it.

TODO
----

 - Unify all tools into one & obsolete the crappy glue scripts.
