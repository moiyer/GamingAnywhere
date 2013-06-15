@echo off
@REM
@set GADEPS=..\deps.win32
echo Create directories in %GADEPS%
@REM mkdir %GADEPS%
mkdir %GADEPS%\bin
mkdir %GADEPS%\lib
mkdir %GADEPS%\include
mkdir %GADEPS%\include\SDL
mkdir %GADEPS%\include\live555
@REM
echo Installing stdint headers ...
bin\7za x -y msinttypes-r26.zip *.h
move /y *.h %GADEPS%\include
@REM
echo Installing ffmpeg ...
@REM set FFMPEG=20121010-git-1a104bf
@REM set FFMPEG=20121114-git-2f74f8d
@REM set FFMPEG=1.1.1
set FFMPEG=1.2
bin\7za x -y ffmpeg-%FFMPEG%-win32-shared.7z
move /y ffmpeg-%FFMPEG%-win32-shared\bin\* %GADEPS%\bin\
rmdir /s /q ffmpeg-%FFMPEG%-win32-shared
@REM
bin\7za x -y ffmpeg-%FFMPEG%-win32-dev.7z
move /y ffmpeg-%FFMPEG%-win32-dev\lib\*.lib %GADEPS%\lib\
xcopy /e /q /h /r /y ffmpeg-%FFMPEG%-win32-dev\include\* %GADEPS%\include\
rmdir /s /q ffmpeg-%FFMPEG%-win32-dev
@REM
echo Installing SDL ...
@REM set SDL=20130130
set SDL=20130219
bin\7za x -y SDL-devel-%SDL%-VC.zip
move /y SDL-%SDL%\lib\x86\*.dll %GADEPS%\bin\
move /y SDL-%SDL%\include\*.h %GADEPS%\include\SDL\
move /y SDL-%SDL%\lib\x86\*.lib %GADEPS%\lib\
rmdir /s /q SDL-%SDL%
@REM
echo Installing pthreads ...
bin\7za x pthreads-w32-2-9-1-release.zip Pre-built.2
move /y Pre-built.2\dll\x86\pthread*.dll %GADEPS%\bin\
move /y Pre-built.2\include\*.h %GADEPS%\include\
move /y Pre-built.2\lib\x86\*.lib %GADEPS%\lib\
rmdir /s /q Pre-built.2
@REM
echo Installing live555 ...
bin\7za x live.2012.05.17-bin.zip
move /y live555\include\*.* %GADEPS%\include\live555\
move /y live555\lib\*.lib %GADEPS%\lib\
rmdir /s /q live555
@REM
echo Installing detour library ...
bin\7za x detour.7z
move /y detour\*.h %GADEPS%\include\
move /y detour\*.lib %GADEPS%\lib\
move /y detour\*.dll %GADEPS%\bin\
rmdir /s /q detour
@REM
echo Installation finished
pause
