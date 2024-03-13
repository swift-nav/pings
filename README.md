# [pings][pings]

A debug utility for monitoring and inspecting NTRIP. This utility uses the same
libraries and mechanisms as the Piksi Multi.  An alternate version
with pre-built binaries is available in the [ntripping](https://github.com/swift-nav/ntripping) project.

[![pings][pings-img]][pings]

## Build and Installation

Building these utilities requires autoconf and automake.  On ubuntu, these packages are eponymous.
libcurl is also required; libcurl4-gnutls-dev is the Swift recommened package on Ubuntu.  

Ping utilities can be installed on multiple platforms with:

    autoreconf --install
    ./configure
    make
    make install

## Usage

The `ntripping` utility has the following usage:

    Usage: ntripping

    Main options
            --url     <url>
            --lat     <latitude>
            --lon     <longitude>
            --height  <height>
            --no-eph  Disable ephemerides on connection (X-No-Ephemerides-On-Connection)
            --verbose Enable curl verbose output
            --debug   Enable curl debug output
Different resources can be requested from different locations. By default, a San
Francisco latitude, longitude, and height will be used.

[pings]:     https://github.com/swift-nav/pings
[pings-img]: https://user-images.githubusercontent.com/60851/37629767-e2d7e994-2b9d-11e8-8e7d-fc02f79eab28.jpg
