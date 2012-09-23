@ECHO OFF

echo Downloading %1
echo --------------

cd %DL_PATH%

FOR /F "eol=; tokens=1,2" %%f IN ('type %2') DO (
  IF NOT EXIST %%f (
    %WGET% "%%g/%%f"
  ) ELSE (
    echo Already have %%f
  )

  copy /b %%f %TMP_PATH%
)

echo Extracting...
echo -------------

cd %TMP_PATH%

FOR /F "eol=; tokens=1,2" %%f IN ('type %2') DO (
  %ZIP% x %%f
)

FOR /F "tokens=*" %%f IN ('dir /B "*.tar"') DO (
  %ZIP% x -y %%f
) 2>nul