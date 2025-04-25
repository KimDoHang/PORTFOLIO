<h1 style="display:inline">MyRedis 서버</h1> 👉 토큰 서버로 사용하기 위해 직접 구현한 인 메모리 DB 서버, 클라, Util 클래스

### ◆ 실행 화면

![image](https://github.com/user-attachments/assets/59522ba2-a22d-4db0-a41e-2ed795d55bbc)
### ◆ 스레드 구조
![Image](https://github.com/user-attachments/assets/507380fc-6c8e-4890-b891-ef11fd2f6613)
## 📂 폴더 및 파일 설명
  ### 📄 00_Main 
 👉 인 메모리 DB 관련 데이터 파일들

### 📄 01_Server
 👉 인 메모리 DB 서버 Arr, Hash 두가지 방식으로 구현
- `RedisServer` : Solo Content 상속을 통해 구현
### 📄 02_Client
 👉  MyRedis 서버와의 통신을 위해 LanClient를 상속받아 구현, 사용을 용이하게 하기 위한 Util 함수 클래스 포함

## ⚙️ Config 설정
👉 `RedisServer.cnf` : MyRedis 서버 관련 설정 파일



