<h1 style="display:inline">중앙 모니터링 서버</h1> 👉 모든 서버로부터 모니터링 항목 수집 및 DB 저장, 클라이언트들에게 관련 항목 전송 
![image](https://github.com/user-attachments/assets/a7720344-9604-4f5b-b301-6dd58d93ce99)

## 📂 폴더 및 파일 설명
  ### 📄 00_ManagerServer 
 👉 Lan으로부터 내부 서버들 모니터링 항목 수정 및 메모리, DB 저장
- `LanMonitor` : Solo Content를 상속하여 구현
### 📄 01_MontiorClientServer
 👉 모니터링 클라이언트들에게 모니터링 항목 전송
- `MonitorSolo` : Solo Content 상속을 통해 구현
### 📄 02_MonitorDistributionServer
 👉 모니터링 분포 클라이언트들에게 모니터링 항목 전송
### 📄 03_MonitorObjects
 👉 각 서버별 모니터링 항목 클래스
### 📄 04_Utils
 👉 SQL 관련 Util 함수
## ⚙️ Config 설정
- `DBServer.cnf` : DB 관련 정보 설정

- `LanMonitor.cnf` : Lan 내부 모니터링 서버 IP, PORT 및 관련 정보 설정

- `NetMonitor.cnf` : 모니터링 클라이언트 서버 IP, PORT 및 관련 정보 설정

- `MonitorGUI_Server.cnf` : 모니터링 분포 클라이언트 서버 IP, PORT 및 관련 정보 설정


