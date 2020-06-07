Supported Platforms
---
The plugin should work on Mac OSX, Linux, and Windows. I successfully tested it on
* Mac OSX 10.13.6 (High Sierra),
* Ubuntu 16.04 (Xenial),
* Ubuntu 18.04 (Bionic), and
* Window 10 Pro

If desired I could also provide libs for ARM architektures such as used by the Raspberry Pi B, 2B, and 3B.

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
2. Go to `Help -> Show Configuration Folder`. This will open the folder holding your personal cura configurations and plugins.
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


Included dependencies
---
### 3Dconnexion SDK:
The c++ osx and windows libraries included in this plugin are linked against the 3DconnexionClient and 3Dconnexion SDK libraries, respectively.
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
