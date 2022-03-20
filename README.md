# Space Cadet Pinball for Playstation Vita
Reverse engineered by [@k4zmu2a](https://github.com/k4zmu2a) and ported to the PS Vita by [@Axiom](https://github.com/suicvne). Build process documented and small improvements done by [@AlphaNERD-](https://github.com/AlphaNERD-).

Find out more about the reverse engineering project here: [k4zmu2a/SpaceCadetPinball](https://github.com/k4zmu2a/SpaceCadetPinball)

Controls:

X - Plunger  
L - Left Bumper  
R - Right Bumper  
DPAD left, up and right - Nudge Table in the respective direction  
Touch Screen - For now, just the IMGUI gui.

### TODO Vita
- ~~Bind tilt controls (left dpad? square and circle? gyro eventually?)~~
- ~~Fix music~~
- (Milestone goal) More Vita-like ImGui style.
- (Milestone goal) "Vertical" orientation mode
- (Stretch goal) Re-bind Vita controls.
- ~~I need to increase the size of the IMGUI controls and add support for the full range of Vita input.~~ DONE
- ~~I would also like to bind the main Pinball menus to the Vita Dpad/Cross button.~~ DONE

## How to play
1. Download .vpk file from Releases page. (Alternatively build your own .vpk file from source.)
2. Load the .vpk file onto your PS Vita.
3. Install the .vpk using VitaShell.
4. Copy the files from Space Cadet Pinball to your PS Vita into the directory ux0:data/SpaceCadetPinball and enjoy! (Google is absolutely helpful if you don't have the files already)

## How to build from source
### Part 1: Install the VITASDK
Instructions on how to install the VITASDK are available on [vitasdk.org](https://vitasdk.org/)

### Part 2: Build and install AudioCodecs
To build the AudioCodecs, run the following commands. Don't worry, all these commands work Windows too. That's why we use MSYS2. No WSL or other VM with Linux necessary.
```sh
wget --no-check-certificate --content-disposition https://github.com/WohlSoft/SDL-Mixer-X/releases/download/2.5.0-1/AudioCodecs-2021-09-23.zip
unzip AudioCodecs-2021-09-23.zip
cd AudioCodecs-2021-09-23
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake ../
make
```

After the build is done, run the following commands to install everything.
```sh
cp -a lib/. $VITASDK/arm-vita-eabi/lib/
cd ..
cp -a FluidLite/include/. $VITASDK/arm-vita-eabi/include
cp -a libFLAC/include/. $VITASDK/arm-vita-eabi/include
cp -a libgme/include/. $VITASDK/arm-vita-eabi/include
cp -a libmad/include/. $VITASDK/arm-vita-eabi/include
cp -a libmikmod/include/. $VITASDK/arm-vita-eabi/include
cp -a libmodplug/include/. $VITASDK/arm-vita-eabi/include
cp -a libopenmpt/include/. $VITASDK/arm-vita-eabi/include
cp -a libOPNMIDI/include/. $VITASDK/arm-vita-eabi/include
cp -a libopus/include/. $VITASDK/arm-vita-eabi/include
cp -a libopusfile/include/. $VITASDK/arm-vita-eabi/include
cp -a libtimidity-sdl/include/. $VITASDK/arm-vita-eabi/include
cp -a libvorbis/include/. $VITASDK/arm-vita-eabi/include
cp -a libxmp/include/. $VITASDK/arm-vita-eabi/include
```

### Part 3: Build and install SDL-Mixer-X
This process is similar to what we did with the AudioCodecs. Run these commands to build and install SDL-Mixer-X.
```sh
wget --no-check-certificate --content-disposition https://github.com/WohlSoft/SDL-Mixer-X/archive/refs/tags/2.5.0-1.zip
unzip SDL-Mixer-X-2.5.0-1.zip
cd SDL-Mixer-X-2.5.0-1/
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake DSDL2_LIBRARY=$VITASDK/arm-vita-eabi/lib/libSDL2.a -DSDL2_INCLUDE_DIR=$VITASDK/arm-vita-eabi/include/SDL2 ../
make
cp -a lib/. $VITASDK/arm-vita-eabi/lib/
cd ..
cp include/SDL_mixer.h $VITASDK/arm-vita-eabi/include/SDL_mixer_ext.h
```

### Part 4: Actually build Space Cadet Pinball
Now we're finally ready to build the game! Run these commands to build the .vpk file. You won't need to do the previous steps for future builds.
```sh
git clone https://github.com/suicvne/SpaceCadetPinball_Vita.git
cd SpaceCadetPinball_Vita/
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake -DSDL2_PATH=$VITASDK/arm-vita-eabi/lib/libSDL2.a -DSDL2_INCLUDE_DIR=$VITASDK/arm-vita-eabi/include/SDL2 -DSDL2_MIXER_LIBRARY=$VITASDK/arm-vita-eabi/lib/libSDL2_mixer_ext.a -DSDL2_MIXER_INCLUDE_DIR=$VITASDK/arm-vita-eabi/include ../
make
```

### Debugging