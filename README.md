# Compile

## Prerequested

```
sudo apt install gcc-arm-none-eabi
python3 -m install pycryptodomex
```

## Build

```
mkdir _build
cd _build
cmake -DPICO_SDK_FETCH_FROM_GIT=1 ..
# or cmake -DPICO_SDK_PATH=<PICO_SDK_PATH> ..
# if PICO_SDK_PATH is defined in the environment
# cmake ..
make -j$(nproc)
```
