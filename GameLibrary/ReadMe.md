<h1 style="display:inline">게임 라이브러리</h1> 👉 Lan, Net Client 및 Lan, Net Server 기능들 포함

## 📂 폴더 및 파일 설명
  ### 📄 LanNetwork 
 👉 LAN 내부 통신시 사용하는 클라 및 서버
- `LanClient` : 암호화가 없는 클라이언트
- `LanServer` : 암호화가 없는 서버
- `LanSendPacket, LanRecvPacket, LanSerializeBuffer` : 직렬화 버퍼 관련 파일들

### 📄 NetNetwork 
 👉 외부와 통신시 사용하는 클라 및 서버
- `NetClient` : 암호화가 있는 클라이언트
- `NetServer` : 암호화가 있는 서버
- `NetSendPacket, NetRecvPacket, NetSerializeBuffer` : 직렬화 버퍼 관련 파일들
### 📄 Thread
 👉  Solo, Group 콘텐츠 로직 기능 제공, 
- `SoloInstance` : 혼자 동작하는 Session 기능 제공 파일
- `GroupInstance` : 여러 Session이 존재하는 프레임 기능 제공 파일
### 📄 Memory
 👉  TLS Memory Pool List, ARR 방식제공
- `LockFreeObjectPoolTLS` : Arr 방식의 TLS Memory Pool
- `LockFreeObjectPoolTLSList` : List 방식의 TLS Memory Pool (Memory 사용량 감소)

### 📄 DataStructure
 👉  LockFree 알고리즘을 이용한 Stack, Queue 제공
- `LockFreeQueueStatic` : Queue의 노드 풀을 공용으로 사용하는 Queue
- `LockFreeQueue, Stack` : LockFree 알고리즘의 Queue, Stack 파일

### 📄 Utils
 👉  모니터링 및 추가 기능들 제공
- `CpuUsage` : 서버 및 프로세스별 CPU 사용률
- `PerformanceMonitor` : PDH를 통해 프로세스 관련 여러 데이터 제공 파일
- `Profiler` : 성능 측정을 위한 파일
- `TextParser` : WCHAR, CHAR 관련 텍스트 파서

## ⚙️ Config 설정
👉 `Values.h` 직렬화 버퍼 및 파일 내부 관련 초깃값 설정 가능

