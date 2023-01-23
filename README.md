# TmForeverResFix
A simple patch for Nadeo's TrackMania Forever games to allow for running the game at arbitrary resolutions in windowed mode, alongside borderless windowed.  
Tested to work with both [TrackMania United Forever](https://store.steampowered.com/app/7200/Trackmania_United_Forever/) and [TrackMania Nations Forever](https://store.steampowered.com/app/11020/TrackMania_Nations_Forever/).

## Installation instructions
 - Pull down MinHook as a submodule
 - Compile the repository with your favourite flavour of Visual Studio
 - Place the new `d3d9.dll` file next to your TmForever.exe

## Configuration
Create a new INI file next to TmForever.exe named `TmWindow.ini`.
Inside that file, the syntax looks like this:
```ini
[Window]
Title=some custom window title here
Width=1280 # window width
Height=720 # window height
FullscreenMode=0 # Fullscreen mode - 0 windowed, 1 exclusive, 2 borderless
```

## License
MIT