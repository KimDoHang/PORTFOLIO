<h1 style="display:inline">Remote Procedure Call 관련 폴더</h1> 👉 RPC 컴파일러 및 Template 파일과 이를 이용하여 구현한 싱글 스레드 채팅 서버

![Image](https://github.com/user-attachments/assets/cd95f6d9-996e-47cf-823a-ce83f61a611d)

## 📂 폴더 및 파일 설명
  ### 📄 MarkinosC
 👉 RPC 컴파일러 및 Templates
 - Markinos.exe 및 Templates을 이용하여 MIDL 파일로부터 원하는 h, cpp 파일 생성 가능
 - Markinosexe.bat을 통해서 exe 파일 생성 가능
### 📄 RPC_ChatServer
 👉 Markinos RPC 컴파일러를 활용하여 구현한 싱글 채팅 서버 
 (항목별 설명은 우측 참고  [하위 폴더의 README로 이동](../SingleChatServer/ReadMe.md))
- `AuthSolo` : Solo Content 상속을 통해 Redis와의 인증 처리 절차 수행
- `ChatSolo` : Solo Content 상속을 통해 채팅 서버 로직을 Lock을 잡고 Session마다 병렬적으로 처리
### 📄 02_MonitorClient
 👉  중앙 모니터링 서버로 모니터링 데이터 전송을 위한 클라이언트
### 📄 03_Utils
 👉  Redis 및 SQL 유틸 함수들

## ⚙️ Config 설정
- `MultiChatServer.cnf` : 멀티채팅서버 관련 수치 항목 설정 파일
