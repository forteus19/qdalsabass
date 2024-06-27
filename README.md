# qdalsabass

ALSA sequencer implementation for BASSMIDI.

## Usage

While the programs is open, ALSA sequencer events can be piped into qdalsabass's port and the events will be translated into BASSMIDI calls.

To use qdalsabass inside of a WINE application, just select qdalsabass as the output MIDI device.

## Compiling

Grab required dependencies:

Ubuntu:
```
sudo apt install git make cmake libasound2-dev libsdl2-dev libspdlog-dev
```

Fedora:
```
sudo dnf install git make cmake alsa-lib-devel SDL2-devel spdlog-devel
```

Then qdalsabass can be compiled by using basic git and CMake commands:

```
git clone --recursive https://github.com/forteus19/qdalsabass.git
cd qdalsabass
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## License

qdalsabass is licensed under GPL-3.0.