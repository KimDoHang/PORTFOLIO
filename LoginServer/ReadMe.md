<h1 style="display:inline">로그인 서버</h1> 👉 클라이언트 로그인 처리 및 레디스 토큰 저장 

![Image](https://github.com/user-attachments/assets/83a6f2c0-c3a6-40a4-85cc-1171aa6f4af5)

## 📂 폴더 및 파일 설명
  ### 📄 00_Main 
 👉 로그인 서버 관련 Data 및 프료토콜 구현

### 📄 01_LoginServer
 👉 Solo Content 상속 받아 IOCP Thread를 이용해 Session마다 병렬적으로 처리
- `AuthServer` : Solo Content 상속을 통해 구현
### 📄 02_MonitorClient
 👉  중앙 모니터링 서버로 모니터링 데이터 전송을 위한 클라이언트

### 📄 03_Utils
 👉  Redis 및 SQL 유틸 함수들

## ⚙️ Config 설정
- `LoginServer.cnf` : Login 서버 관련 스레드 및 유저수 등 설정 파일
- `DBServer.cnf` : DB로 접근하여 인증 코드 검사 (클라이언트가 외부 Platform로 부터 왔다고 가정)

