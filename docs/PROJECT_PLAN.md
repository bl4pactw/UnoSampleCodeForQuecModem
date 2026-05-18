# Quectel AT Command 範例系列計劃書

## 目標

本專案目標是從一個簡單的 Arduino UART 讀取範例，逐步發展成 Quectel 通訊模組 AT command 應用範例集。每個範例都應該能單獨編譯、清楚展示一個開發主題，並能作為實務專案的起點。

## 現況分析

目前 repo 只有一個 Arduino sketch：

- `SimpleReadModem.ino`

程式已具備後續範例會需要的幾個基礎能力：

- 跨 Arduino UNO Q 與 AVR 環境切換 UART 實作。
- 使用 ring buffer 接收 modem UART 資料。
- 將 byte stream 轉為 line-based modem response。
- 主循環維持非阻塞風格，適合後續加入 AT command 狀態機。
- 對過長 line 做基本保護與 HEX dump。

目前仍缺少的能力：

- AT command 發送佇列。
- 指令 timeout 與 retry 機制。
- `OK`、`ERROR`、`+CME ERROR`、`+CMS ERROR` 等結果碼分類。
- URC 與 command response 的分流處理。
- 模組初始化流程，例如關閉 echo、查詢 SIM、查詢註冊狀態。
- 範例命名規範、資料夾結構與 README 索引。

## 建議專案結構

初期可以維持 Arduino sketch 為主，等範例數量增加後再整理成以下結構：

```text
UnoSampleCodeForQuecModem/
├── README.md
├── docs/
│   ├── PROJECT_PLAN.md
│   ├── HARDWARE_SETUP.md
│   └── AT_COMMAND_NOTES.md
├── examples/
│   ├── 00_simple_read_modem/
│   ├── 01_basic_at_test/
│   ├── 02_sim_and_signal/
│   ├── 03_network_registration/
│   ├── 04_pdp_context/
│   ├── 05_tcp_client/
│   ├── 06_mqtt_client/
│   └── 07_http_client/
└── src/
    ├── QuecAtClient/
    └── QuecAtParser/
```

短期內不一定要馬上拆 `src/` library；可以等至少 2 到 3 個範例重複出現相同 parser、timeout、command queue 邏輯後再抽出共用層。

## 範例規劃

### Phase 1: 基礎通訊

目的：確認 MCU 與 modem 的 UART 通訊可靠。

- `00_simple_read_modem`: 保留目前接收與分行解析能力。
- `01_basic_at_test`: 送出 `AT`、`ATE0`、`ATI`、`AT+GMR`，確認基本回應。
- `02_response_parser`: 示範如何辨識 `OK`、`ERROR` 與 multiline response。

### Phase 2: SIM 與網路狀態

目的：建立連網前的檢查流程。

- `03_sim_status`: 查詢 `AT+CPIN?`。
- `04_signal_quality`: 查詢 `AT+CSQ` 並轉換成可讀訊號狀態。
- `05_network_registration`: 查詢 `AT+CREG?`、`AT+CEREG?`、`AT+CGREG?`。

### Phase 3: Packet Data

目的：建立蜂巢式資料連線。

- `06_pdp_context`: 設定 APN 與 `AT+CGDCONT`。
- `07_attach_and_activate`: 處理 `AT+CGATT?`、`AT+QIACT`、`AT+QIACT?`。
- `08_ip_status`: 讀取 IP、檢查 bearer 狀態。

### Phase 4: Internet 應用

目的：展示常見上層協定。

- `09_tcp_client`: TCP connect、send、receive、close。
- `10_udp_client`: UDP send/receive。
- `11_http_get_post`: HTTP GET/POST。
- `12_mqtt_publish_subscribe`: MQTT connect、publish、subscribe。

### Phase 5: 實務可靠性

目的：補上真實產品會遇到的狀況。

- modem power on/off 與 reset 腳位控制。
- boot ready 與 URC 等待流程。
- timeout、retry、backoff。
- 網路斷線重連。
- `+CME ERROR` 與 `+CMS ERROR` 診斷。
- 低功耗模式與喚醒流程。

## 程式設計原則

- 所有範例預設使用非阻塞 loop。
- AT command 流程使用明確狀態機，不使用大量 `delay()`。
- 每個範例只聚焦一個主題，避免把所有功能堆在同一個 sketch。
- parser 應能分辨 command response 與 URC。
- timeout 與 retry 應集中管理，避免散落在各範例。
- 所有 modem 特定指令都要標註適用模組系列或需查官方文件確認。

## 文件規劃

建議逐步補齊：

- `README.md`: 專案入口、硬體需求、範例索引。
- `docs/HARDWARE_SETUP.md`: 接線、電源、level shifting、baud rate 注意事項。
- `docs/AT_COMMAND_NOTES.md`: 常用 AT command、回應格式、錯誤碼整理。
- 每個 `examples/*/README.md`: 該範例目的、流程圖、使用指令、預期輸出。

## 測試與驗證

Arduino 與 modem 專案很難完全自動化測試，但仍可建立幾個驗證層級：

- 編譯測試：確保每個 example 都能在目標 board profile 下編譯。
- parser 單元測試：把 AT response 字串餵給 parser，驗證狀態判斷。
- 手動測試紀錄：記錄模組型號、韌體版本、SIM、電信商、測試日期。
- 實機 smoke test：至少驗證 `AT`、`ATE0`、`CPIN`、`CSQ`、註冊狀態。

## Quectel AT Command Skill 規劃

建議建立 skill，但不建議在第一個 sketch 時就定稿。比較好的時機是：

- 已完成 3 到 5 個穩定範例。
- 已整理出常見 AT command 流程。
- 已確認至少一個 Quectel 模組系列的實測行為。
- 已有錯誤處理與 timeout 策略的共識。

### Skill 可以解決的問題

- 協助使用者依模組型號、網路制式與應用場景產生 AT command 流程。
- 產生 Arduino / MCU 非阻塞狀態機範例。
- 解析 modem log，指出卡在哪個階段。
- 將 `+CME ERROR`、`+CMS ERROR`、URC 與常見 response 轉成診斷建議。
- 提醒硬體注意事項，例如供電、UART level、PWRKEY、RESET、天線與 SIM。

### Skill 建議內容

```text
quectel-at-command/
├── SKILL.md
├── references/
│   ├── common-command-flows.md
│   ├── response-patterns.md
│   ├── error-handling.md
│   └── hardware-checklist.md
└── scripts/
    └── parse_modem_log.py
```

`SKILL.md` 可以先定義工作流程：

1. 詢問或辨識模組型號、韌體版本、介面與應用目標。
2. 先確認硬體與 UART 通訊。
3. 再確認 SIM、訊號、註冊狀態。
4. 最後進入 PDP context 與應用層協定。
5. 遇到錯誤時先分類 response、URC、timeout 或硬體問題。

### Skill 風險

- Quectel 不同系列指令相近但不完全一致，skill 必須要求使用者提供模組型號。
- 官方文件版本會變動，skill 應避免聲稱所有指令通用。
- 網路註冊與 PDP 問題常受 SIM、電信商、頻段、天線與供電影響，不能只從程式判斷。

## 短期工作清單

- [ ] 將目前 `SimpleReadModem.ino` 移到 `examples/00_simple_read_modem/`。
- [ ] 新增 `01_basic_at_test`，示範送出 `AT` 並等待 `OK`。
- [ ] 建立共用的 response parser 雛形。
- [ ] 補上硬體接線文件。
- [ ] 建立手動測試紀錄格式。
- [ ] 收集第一批 Quectel 模組型號與 AT command 文件來源。

## 建議里程碑

| 里程碑 | 交付內容 |
| --- | --- |
| M1 | README、計劃書、目前 simple read 範例整理完成。 |
| M2 | 基礎 AT command 發送、timeout、OK/ERROR parser 完成。 |
| M3 | SIM、訊號、網路註冊範例完成。 |
| M4 | PDP context 與 TCP/HTTP/MQTT 其中一個應用範例完成。 |
| M5 | 建立 Quectel AT command skill 初版。 |
