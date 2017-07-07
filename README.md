# HyperRogue Cut Down

I would like to know how [ZenoRogue's](https://github.com/zenorogue) [HyperRogue](https://github.com/zenorogue/hyperrogue) works, so I am slicing parts away from it in order to make it more understandable.

I'm starting with version 9.4g as downloaded from [here](http://www.roguetemple.com/z/hyper/hyperrogue94g-src.tgz) though it should correspond to [this tag on github](https://github.com/zenorogue/hyperrogue/tree/v9.4g) more or less.

# Build instructions
The build instructions from [ZenoRogue's downloads page](http://www.roguetemple.com/z/hyper/download.php), are reproduced below.


```bash
sudo apt-get install gcc libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev
unzip hyperrogue-74h.zip
cd hyperrogue-74h
make
```

You also need to copy the font file DejaVuSans-Bold.ttf, and (optionally) sound and music files (e.g. from the Windows package).
