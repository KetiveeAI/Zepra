@echo off
echo Starting KetiveeSearch Backend and Zepra Browser...
echo.

REM Start the search backend
echo [1/2] Starting KetiveeSearch Backend...
cd ketiveeserchengin\backend
start "KetiveeSearch Backend" cmd /k "npm start"
cd ..\..

REM Wait a moment for the backend to start
timeout /t 3 /nobreak > nul

REM Start the browser
echo [2/2] Starting Zepra Browser...
cd build
start "Zepra Browser" cmd /k "bin\zepra.exe"
cd ..

echo.
echo Both services are starting...
echo - Search Backend: http://localhost:6329
echo - Browser: Running from build\bin\zepra.exe
echo.
echo Press any key to exit this launcher...
pause > nul 