
REM ���� ���丮 ���� �� ������ ���丮�� �̵� (pushd), %~dp0 ���� ���� ���� ��ġ ������ ���丮 ��θ� �ǹ�
pushd %~dp0 
REM pyinstaller : python ������ ���� ���Ϸ� ����� ����, --onefile : ���� .exe ���Ϸ� ���巷�ش�.
pyinstaller --onefile MarkinosC.py


MOVE .\dist\MarkinosC.exe .\Markinos.exe


REM RD -> ���丮 ����, DEL -> File ����, /S -> ���� ���� �� ���丮 ����, /Q -> ����� �Է� X, /F -> �б����뵵 ����
@RD /S /Q .\build
@RD /S /Q .\dist
DEL /S /F /Q .\MarkinosC.spec

