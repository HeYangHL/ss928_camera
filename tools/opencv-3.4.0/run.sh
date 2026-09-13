cmake ../ -DCMAKE_C_COMPILER=/opt/linux/x86-arm/aarch64-mix210-linux/bin/aarch64-mix210-linux-gcc -DCMAKE_CXX_COMPILER=/opt/linux/x86-arm/aarch64-mix210-linux/bin/aarch64-mix210-linux-g++ -DCMAKE_INSTALL_PREFIX=../_install -DTOOLCHAIN_PATH=/opt/linux/x86-arm/aarch64-mix210-linux/bin/ -DCMAKE_SYSTEM_NAME=linux -DCMAKE_CXX_FLAGS="-Dopencv_core_EXPORTS" -DCMAKE_MAKE_PROGRAM:PATH=/usr/bin/make -DCMAKE_TOOLCHAIN_FILE=../platforms/linux/aarch64-gnu.toolchain.cmake

make

make install
