@ECHO OFF
ECHO Running AMeDAS Converter for %USERNAME% on %DATE% %TIME%

AMeDAS_Converter.exe HAKUBA 1590

:: wait for the user to press a key before closing the window
pause
