# SimpleModemATcmdLoop

這個範例在 `SimpleReadModem` 的 UART 接收與分行解析基礎上，加入 AT command list 輪詢任務。程式每秒從清單中取出一筆 AT command 發送給 Quectel modem，並等待最終結果碼或 timeout。

目前預設輪流發送：

```text
AT
ATI
AT+CPIN?
```

## 適用情境

- 驗證 MCU 可以主動送出 AT command 給 Quectel modem。
- 確認 modem 基本通訊、模組資訊與 SIM 狀態。
- 示範非阻塞方式管理定時發送、等待回覆與 timeout。
- 作為後續 command queue、初始化流程、網路註冊流程的雛形。

## UART 設定

| 編譯環境 | Modem UART | Debug UART |
| --- | --- | --- |
| `ARDUINO_UNO_Q` | `Serial1` | `Monitor` |
| `ARDUINO_ARCH_AVR` | `SoftwareSerial` RX 12 / TX 13 | `Serial` |

在 Arduino UNO / AVR 板子上，硬體 `Serial` 通常連到 USB serial 轉換晶片，因此本範例使用 `SoftwareSerial` 連接 modem，保留 `Serial` 給電腦端 debug console。

## Command List

AT command 清單定義在：

```cpp
static const char* const atCmdLst[] = {
  "AT",
  "ATI",
  "AT+CPIN?",
};
```

新增命令時，只需要在 `atCmdLst` 中增加字串，`ATCMD_COUNT` 會自動依清單長度計算。程式會使用 `atCmdIndex` 輪流指向下一筆命令，使用 `atCmdSeq` 作為發送序號，兩者彼此獨立。

## 程式流程

1. `setup()` 初始化 modem UART 與 debug console。
2. `loop()` 持續呼叫 `uartRxTask()` 接收 modem UART 資料。
3. `uartTxTask()` 檢查是否正在等待上一筆 command 的結果。
4. 如果正在等待，且超過 `ATCMD_TIMEOUT_MS`，輸出 timeout 並釋放等待狀態。
5. 如果沒有等待回覆，且距離上次發送已超過 `ATCMD_INTERVAL_MS`，從 `atCmdLst` 取出下一筆命令。
6. 若命令不是空字串，透過 `mySerial.println(cmd)` 發送給 modem。
7. `lineParserTask()` 從 ring buffer 解析出完整一行 response。
8. `modemRspTask()` 將每一行 response 以 `[MODEM]` 前綴輸出。
9. 當 response 是 `OK`、`ERROR`、`+CME ERROR` 或 `+CMS ERROR` 時，視為該筆 command 的最終結果。
10. 輸出 `[ATCMD][RESULT #n] command -> result`，然後進入下一輪等待。

## 預期輸出

debug console 會看到類似：

```text
17:05:58.309 -> [MODEM] AT
17:05:58.309 -> [MODEM] OK
17:05:58.309 -> [ATCMD][RESULT #1681] AT -> OK
17:05:59.260 -> [ATCMD][TX #1682] ATI
17:05:59.340 -> [MODEM] ATI
17:05:59.340 -> [MODEM] Quectel_Ltd
17:05:59.340 -> [MODEM] Quectel_BC66
17:05:59.340 -> [MODEM] Revision: BC66NBR01A10_BETA0617
17:05:59.340 -> [MODEM] OK
17:05:59.340 -> [ATCMD][RESULT #1682] ATI -> OK
17:06:00.261 -> [ATCMD][TX #1683] AT+CPIN?
17:06:00.296 -> [MODEM] AT+CPIN?
17:06:00.296 -> [MODEM] ERROR
17:06:00.296 -> [ATCMD][RESULT #1683] AT+CPIN? -> ERROR
17:06:01.315 -> [ATCMD][TX #1684] AT
17:06:01.315 -> [MODEM] AT
17:06:01.315 -> [MODEM] OK
17:06:01.315 -> [ATCMD][RESULT #1684] AT -> OK
17:06:02.264 -> [ATCMD][TX #1685] ATI
17:06:02.334 -> [MODEM] ATI
17:06:02.334 -> [MODEM] Quectel_Ltd
17:06:02.334 -> [MODEM] Quectel_BC66
17:06:02.334 -> [MODEM] Revision: BC66NBR01A10_BETA0617
17:06:02.334 -> [MODEM] OK
17:06:02.334 -> [ATCMD][RESULT #1685] ATI -> OK
17:06:03.284 -> [ATCMD][TX #1686] AT+CPIN?
17:06:03.320 -> [MODEM] AT+CPIN?
17:06:03.320 -> [MODEM] ERROR
17:06:03.320 -> [ATCMD][RESULT #1686] AT+CPIN? -> ERROR
```

如果 modem 沒有在 timeout 內回覆最終結果碼，會看到：

```text
[ATCMD][TIMEOUT #3] AT+CPIN? no final response
```

## 可調整項目

- `ATCMD_INTERVAL_MS`: 每筆 command 的發送間隔，目前為 `1000 ms`。
- `ATCMD_TIMEOUT_MS`: 等待最終結果碼的 timeout，目前為 `1000 ms`。
- `atCmdLst`: 要輪流發送的 AT command 清單。
- `mySerial.begin(115200)`: modem UART baud rate。
- `RB_SIZE` / `LINE_SIZE`: 接收 buffer 與單行 response buffer 大小。

## 注意事項

- 目前範例以 `OK`、`ERROR`、`+CME ERROR`、`+CMS ERROR` 作為 command 結束判斷。
- Quectel 部分命令可能會有多行 response，程式會持續列印每一行，直到遇到最終結果碼。
- 如果加入需要較長處理時間的命令，例如網路註冊、PDP 啟用或 scan 類命令，請調大 `ATCMD_TIMEOUT_MS`。
- 這個範例仍是簡化版輪詢，不會根據上一筆 command 結果決定下一步；若要做正式初始化流程，建議再擴充成狀態機。
