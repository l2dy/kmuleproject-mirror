@echo off
.\astyle\bin\astyle --style=allman --recursive --exclude=Tools ..\*.cpp ..\*.h
cd ..
del /S *.orig