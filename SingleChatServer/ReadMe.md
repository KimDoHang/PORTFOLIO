<h1 style="display:inline">싱글 채팅 서버</h1> 👉 Group Content를 상속받아 Frame마다 모든 Session들의 채팅 로직을 하나의 스레드에서 처리

![Image](https://github.com/user-attachments/assets/0564cad8-3610-443c-98ae-1489c825e05b)

## 📂 폴더 및 파일 설명
  ### 📄 00_Main 
 👉 채팅 서버 관련 데이터들 폴더
### 📄 01_ChatServer
 👉 Group Content를 상속하여 구현한 채팅 서버 로직처리 부분
- `ChatGroup` : Group Content 상속을 통해 모든 인증 및 채팅 관련 로직을 싱글 스레드로 처리
### 📄 02_MonitorClient
 👉  중앙 모니터링 서버로 모니터링 데이터 전송을 위한 클라이언트
### 📄 03_Utils
 👉  Redis 및 SQL 유틸 함수들

## ⚙️ Config 설정
👉 `ChatServer.cnf` : 싱글 채팅서버 관련 수치 항목 설정 파일
