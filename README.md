## Building

### Linux, OS X, BSD

Start by executing:
```
./bootstrap
```

To build all PVR addons and install them directly (so that you just have to enable them in Kodi), execute the following (change the prefix path to your actual installation):

```
./configure --prefix=/usr/local/lib/kodi
make install
```

To build all PVR addons and package them into individual ZIP archives (which can then be installed manually), execute the following:
```
./configure
make zip
```

#### Building addons with dependencies:

The build method described above excludes addons that depend on runtime libraries. This is because the installed versions of the dependend libraries on the build-machine and the target machine have to match exactly. Therefore this addons cannot be distributed easily.

Distribution package maintainers and users who build the addons on their target machine can enable the build of addons with dependencies:

```
./configure --enable-addons-with-dependencies
```

#### List of addons with dependencies:

* Filmon addon: jsoncpp, crypto++, curl
* IPTV Simple addon: zlib

### Windows

#### Building the addons standalone

1. Install [Visual Studio Express 2013](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop)
2. Run `project\BuildDependencies\DownloadBuildDeps.bat`
3. Open the solution from `project\VS2010Express\xbmc-pvr-addons.sln`
4. Select the wanted configuration ("Debug" or "Release")
5. Press F7 to build the solution
6. The Windows build system unfortunately doesn't generate ZIP archives of the addons. A workaround is to copy the desired addons to your Kodi installation manually, e.g. by copying `addons\pvr.demo\addon\*.*` to `YOUR_KODI_DIR\addons\pvr.demo\*.*`

#### Building the addons together with Kodi

First ensure that you have separate working build environments for both Kodi and the addons, then import the addon projects into the Kodi solution like this:

1. Open "XBMC for Windows.sln"
2. Right-click the solution in Solution Explorer and select Add -> Existing Project
3. Change file type filter to "Solution Files (*.sln)"
4. Browse to and select the PVR addons solution (xbmc-pvr-addons.sln)
5. Dismiss any warnings about projects already existing in the solution
6. If you only develop on certain addons you can remove unwanted addon projects from the XBMC solution

The PVR addon projects already contain a PostBuild action that only runs when part of the XBMC solution, that copies their output into the Kodi's solution's addon directory. This mean no manual copying is needed like when building the addons standalone.

Remember, if you make any modifications to the Kodi solution to include PVR addons, don't include those changes when you do pull requests!
