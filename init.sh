git submodule update --init

cd third_party/pico-sdk/
git submodule update --init
cd ../..

cd third_party/ooFatFs
git apply ../port/ooFatFs/ooFatFs.patch
cd ../..
