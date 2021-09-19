#!/bin/bash
cd "$(dirname "$0")"
if [ ! -d 'WATCOM' ]; then
  if [ ! -f 'setup.exe' ]; then
    wget 'ftp://ftp.openwatcom.org/install/open-watcom-c-dos-1.9.exe'
    mv 'open-watcom-c-dos-1.9.exe' 'setup.exe'
  fi
  dosbox -c 'mount c .' \
    -c 'c:' \
    -c 'setup -s -ns' \
    -c 'exit'
fi
if [ ! -d 'pdcurses' ]; then
  if [ ! -f '3.9.tar.gz' ]; then
    wget 'https://github.com/wmcbrine/PDCurses/archive/refs/tags/3.9.tar.gz'
  fi
  tar xf '3.9.tar.gz'
  mv 'PDCurses-3.9' 'pdcurses'
fi
dosbox -c 'mount c ..' \
  -c 'c:' \
  -c 'PATH C:\DOS\WATCOM\BINW;%PATH%;' \
  -c 'SET INCLUDE=C:\DOS\WATCOM\H;' \
  -c 'SET WATCOM=C:\DOS\WATCOM' \
  -c 'SET EDPATH=C:\DOS\WATCOM\EDDAT' \
  -c 'SET WIPFC=C:\DOS\WATCOM\WIPFC' \
  -c 'cd dos\pdcurses\dos' \
  -c 'wmake -f Makefile.wcc' \
  -c 'exit'
dosbox -c 'mount c ..' \
  -c 'c:' \
  -c 'PATH C:\DOS\WATCOM\BINW;%PATH%;' \
  -c 'SET INCLUDE=C:\DOS\WATCOM\H;' \
  -c 'SET WATCOM=C:\DOS\WATCOM' \
  -c 'SET EDPATH=C:\DOS\WATCOM\EDDAT' \
  -c 'SET WIPFC=C:\DOS\WATCOM\WIPFC' \
  -c 'cd dos' \
  -c 'wmake' \
  -c 'exit'
mv CSOL.EXE ..
cd ..
rm csol-dos.zip
zip -r csol-dos.zip CSOL.EXE csolrc README.md CHANGES.md themes games
