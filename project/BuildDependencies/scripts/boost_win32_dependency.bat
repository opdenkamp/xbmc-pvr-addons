@ECHO ON

IF EXIST %CD%\boost GOTO END

SET LOC_PATH=%CD%
SET FILES=%CD%\boost_d.txt

SET CUR_PATH=%CD%\..\..\..\project\BuildDependencies
SET WGET=%CUR_PATH%\bin\wget
SET ZIP=%CUR_PATH%\..\Win32BuildSetup\tools\7z\7za
SET TMP_PATH=%CD%\tmp

md %TMP_PATH%
md %CD%

FOR /F "eol=; tokens=1,2" %%f IN (%FILES%) DO (
echo %%f %%g
  IF NOT EXIST %%f (
    %WGET% "%%g/%%f"
  ) ELSE (
    echo Already have %%f
  )

    copy /b "%%f" "%TMP_PATH%"
    del /F /Q "%%f"	
)

cd %TMP_PATH%

FOR /F "eol=; tokens=1,2" %%f IN (%FILES%) DO (
  %ZIP% x %%f
)

FOR /F "tokens=*" %%f IN ('dir /B "*.tar"') DO (
  %ZIP% x -y %%f
)

xcopy boost_1_46_1-headers-win32\* "%LOC_PATH%\" /E /Q /I /Y
xcopy boost_1_46_1-debug-libs-win32\* "%LOC_PATH%\" /E /Q /I /Y
xcopy boost_1_46_1-libs-win32\* "%LOC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%

rmdir %TMP_PATH% /S /Q


:END

