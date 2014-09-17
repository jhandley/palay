# palay [![Build Status](https://travis-ci.org/jhandley/palay.svg)](https://travis-ci.org/jhandley/palay)

Lightweight page layout with Lua

## Building

Palay requires the following packages to build:

    sudo apt-get install g++ make qt4-qmake libqt4-dev liblua5.2-dev poppler-utils ttf-dejavu

The `ttf-dejavu` may be called `font-dejavu`. `poppler-utils` is only needed
if you want to run the unit tests. On Ubuntu 12.04, you also need to install
`libicu48`


```
qmake palay.pro
make
make install
```

If you are running palay on a headless server, you need to start an X Server.

    sudo apt-get install xvfb x11-apps
    export DISPLAY=:99.0

    # Run this
    xvfb-run xlogo &

    # or this
    Xvfb :99 -ac 2>/dev/null &

