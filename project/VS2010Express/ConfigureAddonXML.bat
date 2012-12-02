@ECHO OFF

SETLOCAL

SET CUR_PATH="%CD%"
SET SCRIPT_PATH="%~dp0"
SET SED="%~dp0..\BuildDependencies\bin\sed.exe"

cd "%SCRIPT_PATH%..\..\addons"

FOR /F "tokens=*" %%S IN ('dir /B "pvr.*"') DO (
  echo Configuring %%S
  cd "%%S\addon"
  %SED% "s/@OS@/wingl windx/" addon.xml.in > addon.xml
  cd ..\..
)

cd "%CUR_PATH%"
