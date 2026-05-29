# Arduino Uno Sample Code for Quectel Modem

這個專案目前提供一組 Arduino 範例，用來示範 Quectel 通訊模組的 UART 接收、AT command 發送、回覆解析、URC 分流，以及基礎 cellular networking 狀態機。範例刻意從簡單到進階排列，方便先確認硬體通訊，再逐步加入 command runner、timeout/retry 與連網流程。

## 目前內容

| 檔案 | 說明 |
| --- | --- |
| `SimpleReadModem/SimpleReadModem.ino` | 透過 UART 接收 modem 回應，使用 ring buffer 暫存資料，並依 CR/LF 分行輸出至 debug console。 |
| `SimpleModemInteractive/SimpleModemInteractive.ino` | 從 monitor console 手動輸入 AT command 並送給 modem，同時顯示 response 與 URC。 |
| `SimpleModemATcmdLoop/SimpleModemATcmdLoop.ino` | 每秒輪流送出 AT command list 中的一筆命令，並輸出 modem 回覆、結果碼與 timeout。 |
| `SimpleModemNetworking/SimpleModemNetworking.ino` | 使用非阻塞 cellular 狀態機管理 Quectel BC66 類 NB-IoT 模組的 SIM、註冊、PDP 與 network ready 流程。 |
| `docs/PROJECT_PLAN.md` | 未來範例系列、文件架構、測試方向與 Quectel AT command skill 的規劃。 |

## 支援板子與序列埠

目前程式依編譯環境切換序列埠設定：

| 環境 | Modem UART | Debug UART |
| --- | --- | --- |
| `ARDUINO_UNO_Q` | `Serial1` | `Monitor` |
| `ARDUINO_ARCH_AVR` | `SoftwareSerial` on RX 12 / TX 13 | `Serial` |

在傳統 Arduino UNO / AVR 開發板上，硬體 UART `Serial` 通常已經和板上的 USB serial 轉換晶片連接，用來負責程式上傳與電腦端 debug console。如果直接把同一組硬體 UART 接到 Quectel 模組，容易造成上傳、監看 log、modem 通訊互相干擾。

因此本範例在 `ARDUINO_ARCH_AVR` 環境下採用 `SoftwareSerial` library，預設使用 pin 12 作為 RX、pin 13 作為 TX 連接 Quectel 模組，並保留硬體 `Serial` 給 USB debug 使用。

> 請依實際接線、板子與 modem baud rate 調整程式中的 UART 設定。

## 目前範例狀態

| 範例 | 狀態 | 重點能力 | 適合用途 |
| --- | --- | --- | --- |
| `SimpleReadModem` | 基礎完成 | UART ring buffer、CR/LF line parser、modem log 輸出 | 第一次確認接線、baud rate、modem 是否有輸出。 |
| `SimpleModemInteractive` | 基礎完成 | monitor console 輸入、手動 AT command、常見 URC 標示 | 人工探索 AT command、收集 response/URC log。 |
| `SimpleModemATcmdLoop` | 基礎完成 | command list 輪詢、final result 判斷、timeout | 驗證自動送出 AT command 與簡單回覆流程。 |
| `SimpleModemNetworking` | 雛形完成 | cellular 狀態機、command runner、timeout/retry、URC router、PDP 啟用 | 驗證 BC66 類 NB-IoT 模組的基本連網流程。 |

目前各範例共用的設計方向：

- 使用固定大小 ring buffer 接收 UART byte stream。
- 將 modem 回應依 `\r\n` 分行解析。
- 盡量維持非阻塞 loop，讓後續狀態機、timeout 與 retry 可以穩定運作。
- 使用 `OK`、`ERROR`、`+CME ERROR`、`+CMS ERROR` 判斷 AT command final result。
- 逐步分離 command response 與 URC，避免非同步訊息干擾目前正在等待的 command。
- 在 line buffer 過長時輸出前 16 bytes 的 HEX 資訊，方便初步偵錯。

## 範例列表

### SimpleReadModem

`SimpleReadModem` 是最小接收範例，只負責從 Quectel modem UART 讀取資料、放入 ring buffer、依 CR/LF 切成單行，再輸出到 debug console。這個範例適合用來確認硬體接線、baud rate、電源與 modem 是否有正常吐出 response 或 URC。

詳細流程請見 [SimpleReadModem/README.md](SimpleReadModem/README.md)。

### SimpleModemInteractive

`SimpleModemInteractive` 從 `SimpleReadModem` 衍生，加入 monitor console 輸入功能。使用者可以手動輸入 `AT`、`ATI`、`AT+CPIN?` 等命令，程式會補上 CR/LF 後送給 modem，並持續顯示 modem response 與常見 URC。這個範例適合用來人工探索 AT command、收集 modem log、驗證後續狀態機流程。

詳細流程請見 [SimpleModemInteractive/README.md](SimpleModemInteractive/README.md)。

### SimpleModemATcmdLoop

`SimpleModemATcmdLoop` 在接收解析基礎上加入 AT command list。程式每秒從清單中取出一筆命令送給 modem，依序輪流執行 `AT`、`ATI`、`AT+CPIN?`，並等待 `OK`、`ERROR`、`+CME ERROR`、`+CMS ERROR` 或 timeout。這個範例適合用來驗證基本 AT command 發送流程，並作為後續 command queue 或狀態機的起點。

詳細流程請見 [SimpleModemATcmdLoop/README.md](SimpleModemATcmdLoop/README.md)。

### SimpleModemNetworking

`SimpleModemNetworking` 進一步加入 cellular networking manager，示範如何用非阻塞狀態機完成 `AT` 同步、`ATE0`、`AT+CMEE=2`、模組資訊查詢、SIM 檢查、訊號查詢、EPS registration、PDP context 設定、packet attach、PDP activation 與 IP 查詢。這個範例目前以 Quectel BC66 類 NB-IoT 模組流程為主要參考，networking management 相關 AT command 仍需依模組系列、SIM 卡、營運商、APN、註冊型態與實際網路環境調整。

詳細流程請見 [SimpleModemNetworking/README.md](SimpleModemNetworking/README.md)。

## 快速開始

1. 使用 Arduino IDE 或 Arduino CLI 開啟其中一個 sketch 目錄，例如 `SimpleReadModem/SimpleReadModem.ino`。
2. 確認板子類型與編譯巨集符合目標硬體。
3. 將 Quectel modem 的 UART TX/RX/GND 接至對應腳位。
4. 依目標選擇範例：

| 目標 | 建議範例 |
| --- | --- |
| 只確認 modem 有沒有吐資料 | `SimpleReadModem` |
| 手動輸入 AT command 測試 | `SimpleModemInteractive` |
| 自動輪詢少量 AT command | `SimpleModemATcmdLoop` |
| 跑 SIM、註冊與 PDP 連網流程 | `SimpleModemNetworking` |

5. 確認 modem UART baud rate，必要時修改：

```cpp
mySerial.begin(115200);
```

部分較新的範例使用常數設定：

```cpp
static const uint32_t MODEM_BAUDRATE = 115200;
```

6. 上傳程式後開啟 debug console，觀察輸出格式：

```text
[MODEM] OK
[MODEM] +CSQ: 20,99
```

若使用 `SimpleModemNetworking`，請先依 SIM 卡與營運商修改 APN：

```cpp
static const char PDP_APN[] = "internet";
```

`SimpleModemNetworking` 中的註冊、attach、PDP activation 與 IP 查詢流程是目前的基礎範例，不代表所有 Quectel 模組或所有營運商都能直接套用。實作前請對照目標模組的 AT Command Manual，調整 network registration、PDP context、packet service 與 socket/data service 相關命令。

## 建議的下一步

這個 repo 適合逐步整理成一系列 Quectel AT command 應用範例，例如：

- 基礎通訊：`AT`、`ATE0`、`ATI`、`+GMR`
- SIM 與網路註冊：`+CPIN?`、`+CSQ`、`+CREG?`、`+CEREG?`
- PDP context 與資料連線：`+CGDCONT`、`+CGATT`、`+QIACT`
- TCP/UDP/MQTT/HTTP 應用
- GNSS、SMS、低功耗、韌體版本查詢與錯誤診斷
- 將目前重複的 ring buffer、line parser、command runner 與 URC router 抽成共用 library

詳細規劃請見 [docs/PROJECT_PLAN.md](docs/PROJECT_PLAN.md)。

## Quectel AT Command Skill 建議

建議未來建立一個專門的 Codex skill，用來累積 Quectel AT command 知識。這個 skill 可以協助使用者：

- 根據模組型號與應用情境選擇正確 AT command 流程。
- 產生 Arduino / MCU 端的非阻塞狀態機範例。
- 解析常見 URC、錯誤碼與 modem 回應。
- 建議 timeout、重試、電源控制與網路註冊檢查策略。
- 將官方 AT command 文件轉換成可重用的開發 recipe。

這個 skill 建議在範例累積到 3 到 5 個穩定主題後開始建立，避免太早把尚未驗證的流程固定下來。
