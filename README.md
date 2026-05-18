# Uno Sample Code for Quectel Modem

這個專案目前提供一個 Arduino 範例，用來讀取 Quectel 通訊模組透過 UART 回傳的 AT command 訊息。現階段程式碼刻意保持簡單，重點放在穩定接收 modem 回應、分行解析資料，以及建立後續 AT command 範例系列的基礎。

## 目前內容

| 檔案 | 說明 |
| --- | --- |
| `SimpleReadModem.ino` | 透過 UART 接收 modem 回應，使用 ring buffer 暫存資料，並依 CR/LF 分行輸出至 debug console。 |
| `docs/PROJECT_PLAN.md` | 未來範例系列、文件架構、測試方向與 Quectel AT command skill 的規劃。 |

## 支援板子與序列埠

目前程式依編譯環境切換序列埠設定：

| 環境 | Modem UART | Debug UART |
| --- | --- | --- |
| `ARDUINO_UNO_Q` | `Serial1` | `Monitor` |
| `ARDUINO_ARCH_AVR` | `SoftwareSerial` on RX 12 / TX 13 | `Serial` |

> 請依實際接線、板子與 modem baud rate 調整程式中的 UART 設定。

## 範例功能

- 使用固定大小 ring buffer 接收 UART byte stream。
- 將 modem 回應依 `\r\n` 分行解析。
- 避免在主循環中長時間阻塞，保留未來加入狀態機與 timeout 檢查的空間。
- 在 line buffer 過長時輸出前 16 bytes 的 HEX 資訊，方便初步偵錯。

## 快速開始

1. 使用 Arduino IDE 或 Arduino CLI 開啟 `SimpleReadModem.ino`。
2. 確認板子類型與編譯巨集符合目標硬體。
3. 將 Quectel modem 的 UART TX/RX/GND 接至對應腳位。
4. 確認 modem UART baud rate，必要時修改：

```cpp
mySerial.begin(115200);
```

5. 上傳程式後開啟 debug console，觀察輸出格式：

```text
[MODEM] OK
[MODEM] +CSQ: 20,99
```

## 建議的下一步

這個 repo 適合逐步整理成一系列 Quectel AT command 應用範例，例如：

- 基礎通訊：`AT`、`ATE0`、`ATI`、`+GMR`
- SIM 與網路註冊：`+CPIN?`、`+CSQ`、`+CREG?`、`+CEREG?`
- PDP context 與資料連線：`+CGDCONT`、`+CGATT`、`+QIACT`
- TCP/UDP/MQTT/HTTP 應用
- GNSS、SMS、低功耗、韌體版本查詢與錯誤診斷

詳細規劃請見 [docs/PROJECT_PLAN.md](docs/PROJECT_PLAN.md)。

## Quectel AT Command Skill 建議

建議未來建立一個專門的 Codex skill，用來沉澱 Quectel AT command 的開發知識。這個 skill 可以協助使用者：

- 根據模組型號與應用情境選擇正確 AT command 流程。
- 產生 Arduino / MCU 端的非阻塞狀態機範例。
- 解析常見 URC、錯誤碼與 modem 回應。
- 建議 timeout、重試、電源控制與網路註冊檢查策略。
- 將官方 AT command 文件轉換成可重用的開發 recipe。

這個 skill 建議在範例累積到 3 到 5 個穩定主題後開始建立，避免太早把尚未驗證的流程固定下來。
