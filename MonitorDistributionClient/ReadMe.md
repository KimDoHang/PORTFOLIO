<h1 style="display:inline">모니터링 섹터 분포 클라이언트</h1> 👉 서버의 섹터 분포 모니터링 클라이언트

![Image](https://github.com/user-attachments/assets/1d262975-c1a3-433d-a6be-055524ed7afe)
## 📂 폴더 및 파일 설명
 - `MonitorClass` : NetClient를 상속받아 구현

## ⚙️ Config 설정
📄 `MonitorDistributionClient.cnf` : 모니터링 분포 클라 cnf 설정
- `IP, PORT` : 중앙 모니터링 서버 기준으로 설정
- `ServerNo` : 관측하고 싶은 ServerNo 설정 (ChatServer의 경우 2)
- `Congetstion Level` : 섹터당 혼잡도 별 클라이언트 수 설정

