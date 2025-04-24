![Image](https://github.com/user-attachments/assets/b4c8bed5-644a-4807-8448-805d594f9e26)

## 📂 주요 폴더 설명

# 👉 서버
- `Login Server` : 로그인 서버
- `SingleChatServer` : 싱글 스레드 채팅 서버
- `MultiChatServer` : 멀티 스레드 채팅 서버
- `MonitorServer` : 주앙 모니터링 서버

# 👉 클라이언트
- `DummyClient` : 로그인, 채팅 서버 더미
- `MonitorClient` : 서버 리소스 관련 모니터링 GUI 클라이언트
- `MonitorDistributionClient` : 서버 섹터 분포 모니터링 GUI 클라이언트
  
# 👉 게임 라이브러리
- 게임 서버 및 클라 라이브러리 관련 Lib 파일 (Lan, Net Client / Lan, Net Server) 포함, LockFree 알고리즘 자료구조 및 TLS Memory Pool 포함
  
# 👉 Redis
- `MyRedis` : 직접 구현한 인메모리 DB 서버, 클라, Utils
- `MyRedis를 이용한 LoginChatServer` : MyRedis를 이용하여 구현한 로그인 및 채팅 서버
  
# 👉 RPC 툴 및 서버
- RPC 컴파일러 및 이를 이용한 채팅 서버

