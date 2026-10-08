@echo off
setlocal
cd /d "%~dp0"

echo ========================================================
echo    Launching Multithreaded File Search Engine GUI
echo ========================================================
echo.

python -m streamlit run app.py

pause
