=============================
       Linux, OS-X, BSD
=============================

Start by executing:
./bootstrap

To install add-ons into /path/to/XBMC:
./configure --prefix=/path/to/XBMC
make install

To build all PVR add-ons as .zip archives:
./configure
make zip

Building addons with dependencies:
----------------------------------
The build method described above excludes addons that depend on runtime libraries.
This is because the installed versions of the dependend libraries on the build-machine
and the target machine have to match exactly. Therefore this addons cannot be
distributed easily.

Distribution package maintainers and users who build the addons on their target machine
can enable the build of addons with dependencies:

./configure --enable-addons-with-dependencies

List of addons with dependencies:
---------------------------------
- Filmon addon:
    Build dependencies:   jsoncpp, crypto++, curl
- IPTV Simple addon:
    Build dependencies:   zlib

=============================
           Windows
=============================
Building the pvr addons standalone:
-----------------------------------
Prepare:
1) Install Visual C++ Express 2010 (follow the instructions on the wiki for XBMC itself)
Compile:
2) Run project\BuildDependencies\DownloadBuildDeps.bat
3) Open this solution from project\VS2010Express\xbmc-pvr-addons.sln
4) Select the wanted configuration "Debug" or "Release"
5) Press F7 to build the solution
Install:
Still a TODO. The buildsystem does not yet generate .zip archives
Workaround: copy the wanted pvr addons to your XBMC installation by hand.
Example: addons\pvr.demo\addon\*.* => YOUR_XBMC_DIR\addons\pvr.demo\*.*

Building the pvr addons together with xbmc:
-------------------------------------------
Firstly ensure that you have a working build for both XBMC and xbmc-pvr-addons separately

Then import the xbmc_pvr_addons projects into the XBMC solution
1) Open "XBMC for Windows.sln"
2) Right click the solution in Solution Explorer and select Add -> Existing Project
3) Change file type filter to "Solution Files (*.sln)"
4) Browse to and select the PVR addons solution (xbmc-pvr-addons.sln)
5) Dismiss any warnings about projects already existing in the solution
6) If you only develop on certain addons you can remove unwanted addon projects from the XBMC solution

The pvr projects already contain a PostBuild action that only runs when part of the XBMC solution, that copies their output into the XBMC solution's addon directory

Remember not to include any XBMC solution file modifications to include the PVR addons, in your pull requests!

XBMC Windows installer with included pvr addons:
------------------------------------------------
Not yet possible. Still a TODO.
