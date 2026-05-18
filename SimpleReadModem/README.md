# SimpleReadModem

這個範例示範如何從 Quectel 通訊模組讀取 UART 回覆，並把 modem 傳回來的 byte stream 轉成一行一行的文字輸出到 debug console。它不主動送出 AT command，主要用來確認硬體接線、UART baud rate、modem 是否正常開機，以及是否能看到 modem 主動輸出的 URC 或其他回覆。

## 適用情境

- 第一次確認 Arduino 和 Quectel modem 的 UART 是否接通。
- 檢查 modem 是否有主動輸出 boot message 或 URC。
- 在加入 AT command 發送邏輯前，先驗證接收與分行解析是否穩定。
- 作為後續 parser、狀態機、command queue 範例的接收基礎。

## UART 設定

| 編譯環境 | Modem UART | Debug UART |
| --- | --- | --- |
| `ARDUINO_UNO_Q` | `Serial1` | `Monitor` |
| `ARDUINO_ARCH_AVR` | `SoftwareSerial` RX 12 / TX 13 | `Serial` |

在 Arduino UNO / AVR 板子上，硬體 `Serial` 通常連到 USB serial 轉換晶片，因此本範例使用 `SoftwareSerial` 連接 modem，保留 `Serial` 給電腦端 debug console。

## 程式流程

1. `setup()` 初始化 modem UART 與 debug console。
2. `loop()` 每次執行先呼叫 `uartRxTask()`。
3. `uartRxTask()` 持續讀取 `mySerial.available()` 中的資料。
4. 每讀到一個 byte，就透過 `ringBufPush()` 放入 ring buffer。
5. 如果 ring buffer 已滿，輸出 `[RB] overflow`。
6. `lineParserTask()` 從 ring buffer 取出 byte。
7. 遇到 `\r` 會略過。
8. 遇到 `\n` 且目前 line buffer 有內容時，回傳一行完整 modem response。
9. `loop()` 收到非空字串後，以 `[MODEM]` 前綴印出。
10. 如果單行資料超過 `LINE_SIZE`，程式會輸出前 16 bytes 的 HEX 方便檢查。

## 預期輸出

當 modem 有回覆或 URC 時，debug console 會看到類似：

```text
[MODEM] RDY
[MODEM] APP RDY
[MODEM] +CPIN: READY
```

實際輸出會依 Quectel 模組型號、韌體版本、開機狀態與 SIM 狀態而不同。

## 可調整項目

- `mySerial.begin(115200)`: modem UART baud rate。
- `SoftwareSerial _mySerial(12, 13)`: AVR UNO 使用的 RX/TX 腳位。
- `RB_SIZE`: ring buffer 大小。
- `LINE_SIZE`: 單行 response buffer 大小。

## 注意事項

- 這個範例不會主動送出 `AT`，如果 modem 沒有主動輸出資料，debug console 可能暫時沒有任何訊息。
- 若要測試主動發送 AT command，請使用 `SimpleModemATcmdLoop`。
- Quectel modem 通常需要穩定電源與足夠瞬間電流，UART 正常但 modem 不回應時，請優先檢查供電、PWRKEY、RESET、GND 與天線/SIM 狀態。
