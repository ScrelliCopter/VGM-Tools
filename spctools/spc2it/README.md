SPC2IT
======

Convert SPC files to IT (Impulse Tracker) files.

## Compiling

With GNU make:
```bash
make -j$(nproc)
```

With CMake:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Installing

```bash
# Install to /usr/local by default
sudo make install

# Install to /usr instead
sudo make install PREFIX=/usr
```

## Running

Run `spc2it` (or `./sp2it` locally) with no arguments to see the syntax.
```
 SPC2IT - converts SPC700 sound files to the Impulse Tracker format

 Usage:  spc2it [options] <filename>
 Where <filename> is any .spc or .sp# file

 Options: -t x        Specify a time limit in seconds        [60 default]
          -d xxxxxxxx Voices to disable (1-8)                [none default]
          -r xxx      Specify IT rows per pattern            [200 default]
```

## More information

Cloned from: https://github.com/uyjulian/spc2it

For more information, read the documentation in `doc/`.
Also, see https://www.romhacking.net/forum/index.php?topic=10164.0
