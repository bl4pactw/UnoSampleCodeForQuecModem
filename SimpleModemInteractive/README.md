# SimpleModemInteractive

這個範例從 `SimpleReadModem` 衍生而來，保留 UART ring buffer 與 line parser，並加入 monitor console 輸入功能。使用者可以直接在 Arduino Serial Monitor 或 UNO Q Monitor 中輸入 AT command，程式會補上 `\r\n` 後送給 Quectel modem，同時持續顯示 modem response 與 URC。

## 適用情境

- 手動測試 Quectel modem AT command。
- 快速確認 SIM、訊號、註冊、PDP 或 socket command 行為。
- 收集 modem response / URC log。
- 在正式寫狀態機前，先互動式驗證 command 流程。

## UART 設定

| 編譯環境 | Modem UART | Debug / Monitor Console |
| --- | --- | --- |
| `ARDUINO_UNO_Q` | `Serial1` | `Monitor` |
| `ARDUINO_ARCH_AVR` | `SoftwareSerial` RX 12 / TX 13 | `Serial` |

在 Arduino UNO / AVR 板子上，硬體 `Serial` 通常連到 USB serial 轉換晶片，因此本範例使用 `SoftwareSerial` 連接 modem，保留 `Serial` 給使用者輸入 AT command 與查看 log。

## 使用方式

1. 開啟 `SimpleModemInteractive/SimpleModemInteractive.ino`。
2. 確認 modem UART baud rate，必要時修改 `MODEM_BAUDRATE`。
3. 上傳程式。
4. 開啟 monitor console。
5. 將 line ending 設為 `Newline` 或 `Both NL & CR`。
6. 輸入 AT command，例如：

```text
AT
ATI
AT+CPIN?
AT+CEREG?
```

程式會自動將使用者輸入補上 `\r\n` 後送到 modem。

## 程式流程

1. `setup()` 初始化 modem UART、monitor console，並顯示簡短 help。
2. `consoleRxTask()` 從 monitor console 讀取使用者輸入。
3. 使用者按下 Enter 後，`sendConsoleCommand()` 將完整字串送到 modem。
4. `uartRxTask()` 持續讀取 modem UART 資料並放入 ring buffer。
5. `lineParserTask()` 將 modem byte stream 依 CR/LF 切成單行。
6. `modemLineTask()` 判斷是否為常見 URC，並加上 `[URC]` 或 `[MODEM]` 前綴輸出。

## Console 指令

| 指令 | 說明 |
| --- | --- |
| `/help` | 重新顯示 help。 |
| `/clear` | 清空目前輸入 buffer。 |

其他輸入都會被視為 AT command，直接送到 modem。

## URC 顯示

目前會將以下常見非同步訊息標示為 `[URC]`：

- `RDY`
- `APP RDY`
- `+CPIN:`
- `+CEREG:`
- `+CGREG:`
- `+CREG:`
- `+CSCON:`
- `+CGEV:`
- `+QIURC:`
- `+NPSMR:`

未列出的行仍會以 `[MODEM]` 顯示，避免漏掉任何 modem 輸出。

## 預期輸出

```text
[USER][TX] AT
[MODEM] OK
[USER][TX] AT+CPIN?
[MODEM] +CPIN: READY
[MODEM] OK
[URC] +CEREG: 2,1
```

## 可調整項目

- `MODEM_BAUDRATE`: modem UART baud rate。
- `DEBUG_BAUDRATE`: AVR debug console baud rate。
- `CONSOLE_SIZE`: 使用者輸入 command buffer 大小。
- `RB_SIZE`: modem UART ring buffer 大小。
- `LINE_SIZE`: modem 單行 response buffer 大小。

## 注意事項

- 這個範例是互動式工具，不會自動判斷 command 是否完成，也不會 retry。
- 如果要自動化輪詢 command list，請參考 `SimpleModemATcmdLoop`。
- 如果要做完整 cellular networking 狀態機與 URC 分流，請參考 `SimpleModemNetworking`。
