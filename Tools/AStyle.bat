@echo off
.\astyle\bin\astyle --style=allman --keep-one-line-blocks --keep-one-line-statements --indent-switches --pad-header --unpad-paren --recursive --exclude=Tools ..\*.cpp ..\*.h
cd ..
del /S *.orig