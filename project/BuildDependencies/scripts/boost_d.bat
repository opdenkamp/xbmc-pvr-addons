@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\boost_d.txt

CALL dlextract.bat boost %FILES%

cd %TMP_PATH%

xcopy boost_1_46_1-headers-win32\* "%CUR_PATH%\include\" /E /Q /I /Y
xcopy boost_1_46_1-debug-libs-win32\* "%CUR_PATH%\" /E /Q /I /Y
xcopy boost_1_46_1-libs-win32\* "%CUR_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
