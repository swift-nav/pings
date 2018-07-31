[![pings][pings-img]][pings]

# [pings][pings]

Collection of networking ping utilities for monitoring NTRIP and Skylark. These
utilities use the same libraries and mechanisms as the Piksis.

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
            --url    <url>
            --lat    <latitude>
            --lon    <longitude>
            --height <height>

Different resources can be requested from different locations. By default, the
Geo++ RTCM3_SWFT PRS mountpoint will be used from the office latitude,
longitude, and height.

The `skylarkping` utility has the following usage:

    Usage: skylarkping

    Main options
            --url    <url>
            --device <uuid>
            --lat    <latitude>
            --lon    <longitude>
            --height <height>
            --upload
            --download

Different endpoints can be requested from different locations. By default, the
production endpoint will be used from the office latitude, longitude, and
height. Additionally, the upload and download processes are controlled by
`--upload` and `--download` flags - setting both flags or not setting both will
enable both uploading and downloading; otherwise, the flags will enable one or
the other.

[pings]:     https://github.com/swift-nav/pings
[pings-img]: https://user-images.githubusercontent.com/60851/37629767-e2d7e994-2b9d-11e8-8e7d-fc02f79eab28.jpg
