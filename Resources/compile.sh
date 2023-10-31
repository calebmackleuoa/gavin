echo "main.cpp [COMPILING]"

g++ main.cpp -std=c++17 -o output -lSDL2 -lSDL2_image -lSDL2_ttf \
    -Iexternal/include \
    external/src/serial.cc \
    external/src/impl/unix.cc \
    external/src/impl/list_ports/list_ports_osx.cc

echo "main.cpp [COMPILING COMPLETE]"
