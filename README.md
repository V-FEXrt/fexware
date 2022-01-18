# Building

```
git clone git@github.com:V-FEXrt/fexware.git
cd fexware

bash ./init.sh

mkdir build
cd build
cmake -DPICO_SDK_PATH=../third_party/pico-sdk ..
make
```

# Flashing

- Boot the keyboard in program mode (power on holding boot button)
- Copy build/fexware.uf2 into the flash drive named "RPI-RP2"

# Assigning Keymaps

After flashing the device, a new flash drive named "MiRage" will appear. Copy your keymaps file onto the flash drive.
Keymap format is compatible with the format of https://github.com/ZackFreedman/MiRage. Extra features are also available which will be documented later (probably). The default or base layer should be called "BaseLayer.kmf".

# Known Issues
- Flash drive doesn't seem to be mounting in this revision (likely getting starved by FreeRTOS)
- Missing the dependency required to draw to the OLEDs (available in an old repo just needs to be copied over)
- only standard and hold presses are detected atm
- The multi-core code seems to be racy and will crash some times
- Stacks sizes need to be profiled and probably increased
- Probably other things, will update as I remember.
