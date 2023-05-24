# Compile

## Prerequested

```
sudo apt install gcc-arm-none-eabi 
```

## Build

```
mkdir _build
cd _build
cmake -DPICO_SDK_FETCH_FROM_GIT=1 ..
make -j$(nproc)
```
