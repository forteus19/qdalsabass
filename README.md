# qdalsabass

ALSA sequencer implementation for BASSMIDI.

## Usage

While the programs is open, ALSA sequencer events can be piped into qdalsabass's port and the events will be translated into BASSMIDI calls.

To use qdalsabass inside of a WINE application, just select qdalsabass as the output MIDI device.

## Compiling

Grab required dependencies:

Ubuntu:
```
sudo apt install git cmake make-dfsg alsa-lib libsdl2
```

Fedora:
```
sudo dnf install git cmake alsa-lib-devel SDL2-devel
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

## To-do

- Make the configurator better
- Turn the program into a daemon that can run in the background
- Implement an event buffer


## License

qdalsabass is licensed under GPL-3.0.