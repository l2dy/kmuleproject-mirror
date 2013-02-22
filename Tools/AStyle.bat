@echo off
.\astyle\bin\astyle --style=allman --pad-header --unpad-paren --recursive --exclude=Tools ..\*.cpp ..\*.h
cd ..
del /S *.orig