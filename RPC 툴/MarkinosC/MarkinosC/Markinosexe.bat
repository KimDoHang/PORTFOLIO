
REM 현재 디렉토리 저장 후 지정된 디렉토리로 이동 (pushd), %~dp0 현재 실행 중인 배치 파일의 디렉토리 경로를 의미
pushd %~dp0 
REM pyinstaller : python 파잃을 실행 파일로 만드는 도구, --onefile : 단일 .exe 파일로 만드렁준다.
pyinstaller --onefile MarkinosC.py


MOVE .\dist\MarkinosC.exe .\Markinos.exe


REM RD -> 디렉토리 삭제, DEL -> File 삭제, /S -> 하위 파일 및 디렉토리 삭제, /Q -> 사용자 입력 X, /F -> 읽기전용도 삭제
@RD /S /Q .\build
@RD /S /Q .\dist
DEL /S /F /Q .\MarkinosC.spec

