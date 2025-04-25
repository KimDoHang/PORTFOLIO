<h1 style="display:inline">멀티 채팅 서버</h1> 👉 Solo Content를 상속받아 Lock을 잡고 Session별 로직을 처리하는 형식의 스레드 구조 채팅 서버

![Image](https://github.com/user-attachments/assets/fbbeaeff-49a8-474c-9afd-5763a33c10a2)
## 📂 폴더 및 파일 설명
  ### 📄 00_Main 
 👉 채팅 서버 관련 데이터들 폴더
### 📄 01_ChatServer
 👉 Solo Content를 상속하여 구현한 채팅 서버 로직처리 부분
- `AuthSolo` : Solo Content 상속을 통해 Redis와의 인증 처리 절차 수행
- `ChatSolo` : Solo Content 상속을 통해 채팅 서버 로직을 Lock을 잡고 Session마다 병렬적으로 처리
### 📄 02_MonitorClient
 👉  중앙 모니터링 서버로 모니터링 데이터 전송을 위한 클라이언트
### 📄 03_Utils
 👉  Redis 및 SQL 유틸 함수들

## ⚙️ Config 설정
- `MultiChatServer.cnf` : 멀티채팅서버 관련 수치 항목 설정 파일
