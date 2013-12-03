
In order to build this client in Windows with Visual Studio:

1) get the 'frodo' or 'gotham' branch from the xbmc pvr gethub:
https://github.com/opdenkamp/xbmc-pvr-addons/tree/frodo

2) Place your downloaded pvr.wmc directory in the 'addons' folder, along side the other pvr addons

3) Load the VS2010 solution 'xbmc-pvr-addons-***\project\VS2010Express\xbmc-pvr-addons.sln'

4) Choose 'Add existing project', and place 'pvr.wmc' project in the solution (its located in pvr.wmc\project\VS2010Express)

---------------------

Prior to doing the first build:

You need to first compile "platform", before you do that however put platform's
Pre-Build command inside of quotes, example:

"$(SolutionDir)\ConfigureAddonXML.bat"

then platform should build.

----------------------

After platform is built, you should be able to build pvr.wmc.