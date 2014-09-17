# palay [![Build Status](https://travis-ci.org/jhandley/palay.svg)](https://travis-ci.org/jhandley/palay)

Lightweight page layout with Lua

## Building

Palay requires the following packages to build:

    sudo apt-get install g++ make qt4-qmake libqt4-dev libicu48 liblua5.2-dev poppler-utils ttf-dejavu

The `ttf-dejavu` may be called `font-dejavu`. `poppler-utils` is only needed
if you want to run the unit tests.

```
qmake palay.pro
make
make install
```

If you are running palay on a headless server, you need to start an X Server.

    sudo apt-get install xvfb x11-apps
    export DISPLAY=:99
    xvfb-run xlogo &
