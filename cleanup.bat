@echo off

del Die.msix
del Die.pfx
RMDIR /S /q "obj/"
RMDIR /S /q "Die/Die"
pause
echo Press Enter to exit
