<h1 style="display:inline">Remote Procedure Call 관련 폴더</h1> 👉 RPC 컴파일러 및 Template 파일과 이를 이용하여 구현한 싱글 스레드 채팅 서버

![Image](https://github.com/user-attachments/assets/cd95f6d9-996e-47cf-823a-ce83f61a611d)

## 📂 폴더 및 파일 설명
  ### 📄 MarkinosC
 👉 RPC 컴파일러 및 Templates
 - Markinos.exe 및 Templates을 이용하여 MIDL 파일로부터 원하는 h, cpp 파일 생성 가능
 - Markinosexe.bat을 통해서 exe 파일 생성 가능
### 📄 RPC_ChatServer/Markinos
 👉 Markinos RPC 컴파일러를 활용하여 구현한 싱글 채팅 서버 
 ([싱글 스레드 채팅 서버 구조는 해당 링크 참고](../SingleChatServer/ReadMe.md))
- `MIDL.bat` : Markinos.exe 관련 설정 실행 bat
- `SingleChat.MIDL` : 싱글 채팅 스레드 구현을 위한 IDL 파일
