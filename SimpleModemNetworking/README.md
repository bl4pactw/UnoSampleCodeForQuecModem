# SimpleModemNetworking

這個範例基於 `SimpleModemATcmdLoop` 的 UART 接收、line parser 與 AT command 發送概念，進一步加入 cellular networking manager。它示範如何用非阻塞狀態機管理 Quectel BC66 類 NB-IoT 模組的基本連網流程，並把 URC 與 command response 分流處理。

## 範例目標

- 初始化 modem 並同步 AT command 通訊。
- 查詢 SIM、訊號與 EPS registration 狀態。
- 啟用 registration / RRC 相關 URC。
- 設定 PDP context、attach packet service、啟用 PDP。
- 建立 URC router，避免非同步 URC 干擾目前正在等待的 command response。

## 主要流程

```text
BOOT
  -> SYNC_AT
  -> BASIC_CONFIG
  -> QUERY_INFO
  -> CHECK_SIM
  -> CHECK_SIGNAL
  -> CONFIG_REG_URC
  -> WAIT_REGISTER
  -> CONFIG_PDP
  -> ATTACH_PACKET
  -> ACTIVATE_PDP
  -> NETWORK_READY
```

若任一重要步驟失敗，狀態機會進入 `CELL_ERROR_RECOVERY`，等待一段時間後重新從 `CELL_BOOT` 開始。

## 使用的 AT Command

| 階段 | Command | 用途 |
| --- | --- | --- |
| 同步 | `AT` | 確認 UART 與 modem AT parser 正常。 |
| 基本設定 | `ATE0` | 關閉 echo，降低 response parser 複雜度。 |
| 基本設定 | `AT+CMEE=2` | 啟用詳細錯誤訊息。 |
| 模組資訊 | `ATI` | 查詢模組識別資訊。 |
| 模組資訊 | `AT+CGMR` | 查詢韌體版本。 |
| SIM | `AT+CPIN?` | 確認 SIM 是否 ready。 |
| 訊號 | `AT+CSQ` | 查詢基本訊號品質。 |
| 訊號 | `AT+CESQ` | 查詢延伸訊號品質。 |
| 註冊 | `AT+CEREG=2` | 啟用 EPS registration URC 與較詳細結果。 |
| 註冊 | `AT+CSCON=1` | 啟用 signaling connection 狀態 URC。 |
| 註冊 | `AT+CEREG?` | 輪詢 EPS registration 狀態。 |
| PDP | `AT+CGDCONT=1,"IP","<APN>"` | 設定 PDP context。 |
| Packet service | `AT+CGATT?` | 查詢 packet service attach 狀態。 |
| Packet service | `AT+CGATT=1` | 要求 attach packet service。 |
| PDP | `AT+CGACT=1,1` | 啟用 PDP context。 |
| PDP | `AT+CGPADDR=1` | 查詢 PDP 位址，作為 network ready 判斷。 |

> `PDP_APN` 預設為 `internet`，請依 SIM 卡與營運商需求修改。

## URC 處理機制

每一行 modem response 都會先進入 `dispatchModemLine()`，再交給：

```text
handleUrc()
handleCommandLine()
```

URC router 目前處理：

| URC | 處理 |
| --- | --- |
| `RDY` / `APP RDY` | 標記 modem ready。 |
| `+CPIN:` | 更新 SIM ready 狀態。 |
| `+CEREG:` | 更新 registration 狀態，`stat=1` 或 `stat=5` 視為 registered。 |
| `+CSCON:` | 輸出 RRC connected / idle 狀態。 |
| `+CGEV:` | 偵測 PDP context 事件，若出現 deactivation 會清除 PDP active 狀態。 |
| `+QIURC:` | 保留給 Quectel socket / packet service 非同步事件。 |
| `+NPSMR:` | 保留給 PSM 狀態變化。 |

設計重點是：URC 可能在等待某個 command response 時插入，因此不能只靠「是否正在等待 command」來決定要不要解析 URC。

## Command Runner

`sendAtCommand()` 會記錄：

- 目前 command 字串。
- 預期 response prefix，例如 `+CPIN:`、`+CEREG:`。
- timeout。
- retry 次數。
- 是否已看到預期 response line。

`commandTimeoutTask()` 負責 timeout 與 retry。`handleCommandLine()` 則負責偵測 final result：

```text
OK
ERROR
+CME ERROR
+CMS ERROR
```

只有在收到 `OK` 且已看過預期 prefix 時，該 command 才會被視為成功。對於只需要 `OK` 的 command，預期 prefix 可以設為 `NULL`。

## 預期輸出

```text
[CELL][STATE] 0
[CELL][STATE] 1
[ATCMD][TX #1] AT
[MODEM] OK
[ATCMD][RESULT #1] AT -> OK
[ATCMD][TX #4] AT+CPIN?
[MODEM] +CPIN: READY
[URC][SIM] +CPIN: READY
[MODEM] OK
[ATCMD][RESULT #4] AT+CPIN? -> OK
[URC][REG] stat=1
[CELL] network ready
```

## 可調整項目

- `PDP_APN`: SIM 卡使用的 APN。
- `MODEM_BAUDRATE`: modem UART baud rate。
- `CMD_SIZE`: command buffer 大小。
- `RB_SIZE` / `LINE_SIZE`: UART ring buffer 與單行 parser buffer。
- 各 state 裡的 timeout / retry 次數。

## 注意事項

- BC66 相關命令仍應以實際 AT Command Manual 為準；不同韌體或營運商環境可能需要調整 PDP 與 attach 流程。
- `AT+COPS=?` 這類掃描命令可能耗時很久，不建議放在一般連網流程中。
- 若加入 socket、UDP、MQTT、CoAP 等應用層命令，建議延伸 `+QIURC:` handler，將連線關閉、收到資料、PDP deactivation 等事件納入狀態機。
