pushd %~dp0

IF ERRORLEVEL 1 PAUSE
Markinos.exe --path=./SingleChat.MIDL --output=../RPC_ChatServer/ --client=SingleChat_Client --server=SingleChat_Server --packet=SingleChat_Packet

echo  �α׸� Ȯ���ϼ���
PAUSE