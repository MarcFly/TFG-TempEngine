mkdir cmake_bloat_towin & 
cd cmake_bloat_towin & 
cmake ../ -UToAndroid -DToWindows=1 -UToLinux -DUSE_IMGUI=1
cmake --build . --config Release

#cmake ../ -ToAndroid=1 -UToWindows -UToLinux
#cmake --build . --config Release

#cmd /k