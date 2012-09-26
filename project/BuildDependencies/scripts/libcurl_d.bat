@ECHO OFF

SET LOC_PATH="%CD%"
SET FILES="%CD%\libcurl_d.txt"

CALL dlextract.bat libcurl %FILES%

cd %TMP_PATH%

xcopy curl-7.21.6-devel-mingw32\include\curl %CUR_PATH%\include\curl /E /Q /I /Y
xcopy curl-7.21.6-devel-mingw32\lib\*.a %CUR_PATH%\lib /Y

cd %LOC_PATH%
