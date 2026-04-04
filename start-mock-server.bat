@echo off
echo Building frontend...
cd frontend
call npm run build
cd ..

echo.
echo Starting mock server...
echo.
node mock-server.js
