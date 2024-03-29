# taiko_cam_codereader_plugin

A plugin for TAL,using system camera for qr and barcode reading  
based on opencv.

# usage

download and configurate the modified TAL: https://github.com/lty2008one/TaikoArcadeLoader   
(Refactor branch,download release in github action dists)

complie it as a dll file using "-shared" arg or download the release.

make a folder named "plugins" in the same path of Taiko.exe,bnusio.dll...etc

put the cv_qr.dll in the plugins folder.

install dependences(libopencv_....dll...etc) correctly.

run Taiko.exe.

# config

once run the game with this plugin,a config file named "cam_cfg.txt" will be created with 5 nums in it.  
```
0 640 480 0 5
```
0 : use the first camera of system (can be changed to 1..2..3...)  
640 : camera width in pixel (depends on your camera,eats performance)    
480 : camera height in pixel (depends on your camera,eats performance)  
0 : use or not use debug screen (1 : display the camera view, 0 : off)  
5 : fps of camera (depends on your camera,eats performance)  

the default config works fine with most of the camera.  
640x480@5fps is enough, if you see it is obscure , that is because a error focal length on your camera.  
make sure not to use too high config ,or the game may crash.

# known bugs?

**none**

# requirments

to complie it,you need to install opencv453+,best490,and mingw64 posix seh.

