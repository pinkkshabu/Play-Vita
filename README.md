# Play! #
Play! is a PlayStation2 emulator for Windows, macOS, UNIX, Android, iOS & web browser platforms.

Compatibility information is available on the official [Compatibility Tracker](https://github.com/jpd002/Play-Compatibility).
If a specific game doesn't work with the emulator, please create a new issue there.

For more information, please visit [purei.org](https://purei.org).

You can try the experimental web browser version here: [playjs.purei.org](https://playjs.purei.org).

For general discussion, you're welcome to join our Discord: https://discord.gg/HygQnQP.

## Command Line Options (Windows/macOS/Linux) ##

The following command line options are available:
- `--disc "disc image path"` : Boots a disc image.
- `--elf "elf file path"` : Boots a ELF file.
- `--arcade "arcade id"` : Boots an arcade game.
- `--state "slot number"` : Loads a state from slot number.
- `--fullscreen` : Starts the emulator in fullscreen mode.

## General Troubleshooting ##

#### Failed to open CHD file ####

There are a lot of dumps out there that are not in the proper format, especially for games using CDs or DVDs. The emulator expects CDVD images
to be compressed using chdman's `createcd` or `createdvd` command. If they are not, you will get an error. To check if a game uses CDVD or HDD
images, look inside the associated `arcadedef` file for `cdvd` or `hdd` settings.

It's possible to use `chdman` to verify whether your CDVD image is really a CDVD image. Use this command on your image:

```
chdman info -i image.chd
```

If you see `CHT2` metadata in the output, it means your image is a CDVD image.
If you see `GDDD` metadata in the output, it means your image is a HDD image and that it needs to be converted to CDVD. 

Conversion from HDD to CDVD can be done this way:

```
mv image.chd image.chd.orig
chdman extracthd -i image.chd.orig -o image.iso
chdman createcd -i image.iso -o image.chd
```

## Building ##

### Getting Started ###
Clone the repo and required submodules:
 ```
 git clone --recurse-submodules https://github.com/jpd002/Play-.git
 cd Play-
 ```
vitasdk is required alongside psp2cgc.exe for shaders.
### Building for PlayStation Vita ###
```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake -DCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES=$VITASDK/arm-vita-eabi/include/ -DBUILD_TESTS:BOOL=OFF
make
```
