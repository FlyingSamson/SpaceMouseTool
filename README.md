Description
---
This Cura plugin allows to use a 3Dconnexion space mouse to rotate, and pan the camera in Cura.


Supported Platforms
---
The plugin should work on Mac OSX, Linux, and Windows. I successfully tested it on
* Mac OSX 10.13.6 (High Sierra),
* Ubuntu 16.04 (Xenial),
* Ubuntu 18.04 (Bionic), and
* Window 10 Pro

each running Cura 4.6.1.

If desired I could also provide libs for ARM architectures such as used by the Raspberry Pi B, 2B, and 3B.


State of development
---
### Currently implemented
* Free movement (translation and rotation) of the camera around the current rotation center using a free orbit (constraint orbit might follow in the future, but is currently not in progress.
* The same rotation center as when rotating with the mouse is used. Especially when `center selected model` is activated in Cura the camera will rotate around that model.
* `Top`, `Right`, `Front` buttons of the space mouse work as expected, i.e. they move the camera to top, right, or front view, respectively.
* Additionally when holding down `Shift` either on the space mouse (if it has such a key) or on the keyboard while hitting `Top`, `Right`, or `Front`, the corresponding other side is shown, i.e. the camera moves to bottom, left or rear view.
* The `Rot CW` button of the space mouse works as expected, i.e. it rotates the space clockwise around the view axis by 90 degrees.
* Again holding down `Shift` on the space mouse or on the keyboard will cause the camera to rotate counterclockwise around that axis by 90 degrees.
* Pressing the `Fit` button while one or multiple models are selected will translate/zoom the camera in such a way that those objects are centered and completely visible in the viewport (there is still a little bug here, as the top banner of Cura overlaps the viewport and thus the selected models, I will fix this when I have the time).
* The modifier keys, `Shift`, `Ctrl`, and `Alt` work as expected (on linux, on OSX and Windows you can just map them on the corresponding keyboard keys using the 3DConnexion configuration tool), i.e. they send the appropriate keyboard signal to Cura. I'm not aware of any place where the Esc key is used in Cura, but if there is one enlighten me, and I will try to also add it.
* Rotation lock currently only works on OSX and Windows as it can be configured in the 3DConnexion interface. (I plan to support it in linux in the future, too.)

### Plans for future development (descending by importance)
* Rotation lock on linux
* Constraint orbit movement
* Menu button to open `Print settings`
* Esc key support if required
* I'm always open for suggestions and contributions
* Maybe include the plugin in the Cura marketplace


Installation
---
### Prerequirements
#### Mac OSX / Windows
You need to have the 3Dconnexion drivers installed and running to use the plugin. Those can be found on their [website](https://www.3dconnexion.de/service/drivers.html).
#### Linux
You need to have the [spacenavd daemon](http://spacenav.sourceforge.net) installed and running to use the plugin. On Ubuntu, e.g., you can install it using aptget via the command
```
sudo apt install spacenavd
```

### Installation of the plugin itself
1. Open Cura.
2. Go to `Help -> Show Configuration Folder`. This will open the folder holding your personal Cura configurations and plugins.
3. Close Cura (you will have to restart Cura to make the new plugin available anyway).
4. Download the [latest release](https://github.com/FlyingSamson/SpaceMouseTool/releases/latest) of this plugin.
5. Extract the downloaded file into the `plugins` folder inside the folder found in 2.
6. Start Cura.
7. Enjoy.

### Configuration of the spacemouse
#### Mac OSX
Use the configuration tool provided with the 3Dconnexion Driver. It can be found in the System Preferences under the 3Dconnexion entry. Select Ultimaker Cura in the drop or use `Add Application...` to add it if it is not listed yet.
#### Windows
Use the configuration tool provided with the 3Dconnexion Driver. It can be found in the Start Menu under `3Dconnexion -> 3Dconnexion Properties`. Note that you need to have Cura open in the background in order for the configuration tool to recognize the app for which you want to customize the space mouse.
#### Linux
You can use the graphical tool provided [here](https://github.com/FreeSpacenav/spnavcfg/releases) to customize the behavior of your space mouse.  


Building the plugin from source
---
### Prerequirements
#### Mac OSX / Linux
* You will need Python 3.5 with pymalloc deactivated. I built version 3.5.9 from [source](https://www.python.org/downloads/release/python-359/), as the non-pymalloc version wasn't available through apt-get or macports. To disable pymalloc use the `--without-pymalloc` flag during configuration.
* You will also need a standard build environment including g++/gcc, etc.  
* Finally, on Linux you will need the development package of libspacenav. On Ubuntu you can install those using.
```
sudo apt install libspnav-dev
```

#### Windows
* You will need Python 3.5. I used Python 3.5.4 as it is the latest version of Python 3.5 available on the [download page](https://www.python.org/downloads/windows/).
* You will also need Visual Studio 2015 (not that easy to come by without an Microsoft developer account, but the iso image can still be found in a stackoverflow post). At least I used version 2015, but as I'm not normally a Windows developer I don't know whether newer versions will also work.
* Finally, you will need the 3Dconnexion SDK which is available in the [developer section of the 3Dconnexion website](https://www.3dconnexion.de/service/software-developer.html) (requires you to create an account).

### Building the code
1. Get the source code of the [latest release](https://github.com/FlyingSamson/SpaceMouseTool/releases/latest) (or any other commit that you want to build) and extract it to the `plugins` folder of your Cura configuration directory as explained above.
2. Using the (Power)shell navigate to the src directory inside the extracted folder.
3. Run
```
python3.5 setup.py build
```
to build the library. Make sure to use the python version without pymalloc.
4. If step 3 fails because some symbols or headers were not found, have a look in `setup.py` and check that the include and link paths are set correctly for your system.

Included dependencies
---
### 3Dconnexion SDK
The c++ OSX and Windows libraries included in this plugin are linked against the 3Dconnexion client and 3Dconnexion SDK libraries, respectively.
> 3D input device development tools and related technology are provided under license from 3Dconnexion. (c) 3Dconnexion 1992 - 2016. All rights reserved.


### libspnav library
The c++ linux library included in this plugin is linked against the libspnav library
maintained by John Tsiombikas (nuclear@member.fsf.org)
The libspnav library is licensed under the modified (3-clause) BSD license:

> Copyright (C) 2007-2018 John Tsiombikas <nuclear@member.fsf.org>  
> Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
>
> 1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
> 2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
> 3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.
>
> THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
