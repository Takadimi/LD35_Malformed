@echo off

SET source=..\src\*.c*
SET cust_include=..\include
SET libs=..\lib\glfw3dll.lib opengl32.lib ..\lib\irrKlang.lib

pushd bin
cl /nologo -FC -Zi /EHsc /Feld35 %source% -I%cust_include% /link %libs% 
popd