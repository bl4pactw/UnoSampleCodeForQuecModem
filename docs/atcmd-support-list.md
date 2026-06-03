## AI 生成

根據您提供的三份 Quectel 模組 AT 指令手冊，我整理了以下指令支援對照表。
表格將指令分為 **產品資訊與通用指令**、**硬體與平台指令**、**網路註冊與服務**、**SIM 卡相關**、**簡訊服務 (SMS)** 以及 **通訊協定相關** 六大類別。

請注意，`V` 表示該模組手冊中明確記載支援，`-` 表示在提供的摘錄中未提及或不支援。

### Quectel 模組 AT 指令支援對照表

#### 1. 產品資訊與通用指令
| AT 指令 | BC660K-GL / BC950K-GL | BC66 / BC66-NA | BG95 / BG77 / BG600L |
| :--- | :---: | :---: | :---: |
| **ATI** (顯示產品資訊) | V | V | V |
| **AT+CGMI / AT+GMI** (製造商識別) | V | V | V |
| **AT+CGMM / AT+GMM** (型號識別) | V | V | V |
| **AT+CGMR / AT+GMR** (韌體版本) | V | V | V |
| **AT+CGSN / AT+GSN** (IMEI 查詢) | V | V | V |
| **ATE** (設定回聲模式) | V | V | V |
| **AT+IPR** (設定串列埠速率) | V | V | V |
| **AT&W** (儲存設定至 NVRAM) | - | V | V |
| **AT&F** (恢復出廠設定) | - | - | V |
| **AT+CFUN** (設定模組功能) | V | V | V |
| **AT+CMEE** (錯誤訊息格式) | V | V | V |

#### 2. 硬體與平台相關指令
| AT 指令 | BC660K-GL / BC950K-GL | BC66 / BC66-NA | BG95 / BG77 / BG600L |
| :--- | :---: | :---: | :---: |
| **AT+CBC** (電池/電壓查詢) | V | V | V |
| **AT+QADC** (讀取 ADC 數值) | V | V | V |
| **AT+QRST** (模組重啟) | V | V | - |
| **AT+QSCLK** (休眠模式配置) | - | V | V |
| **AT+QTEMP** (讀取溫度) | - | - | V |
| **AT+QVBATT** (配置電壓閾值) | - | V | V |
| **AT+CCLK** (設定/查詢時鐘) | - | V | V |

#### 3. 網路註冊與運營商指令
| AT 指令 | BC660K-GL / BC950K-GL | BC66 / BC66-NA | BG95 / BG77 / BG600L |
| :--- | :---: | :---: | :---: |
| **AT+CGATT** (PS 附著/分離) | V | V | V |
| **AT+CREG** (網路註冊狀態) | V | - | V |
| **AT+CEREG** (EPS 網路註冊狀態) | - | V | V |
| **AT+COPS** (運營商選擇) | V | V | V |
| **AT+CSQ** (訊號品質查詢) | V | V | V |
| **AT+QENG** (工程模式查詢) | V | V | - |
| **AT+QBAND** (取得/設定頻段) | V | V | - |
| **AT+QCSEARFCN** (清除 EARFCN 列表) | V | V | - |
| **AT+CCIOTOPT** (CIoT 優化配置) | V | V | - |

#### 4. (U)SIM 卡相關指令
| AT 指令 | BC660K-GL / BC950K-GL | BC66 / BC66-NA | BG95 / BG77 / BG600L |
| :--- | :---: | :---: | :---: |
| **AT+CPIN** (輸入 PIN 碼) | V | V | V |
| **AT+CPINR** (剩餘重試次數) | V | V | V |
| **AT+CIMI** (查詢 IMSI) | V | V | V |
| **AT+QCCID** (查詢 ICCID) | V | V | V |
| **AT+CLCK** (設施鎖定) | V | V | V |
| **AT+CPWD** (更改密碼) | V | V | V |
| **AT+CRSM** (限制性存取 SIM) | V | V | V |

#### 5. 簡訊服務 (SMS) 指令
| AT 指令 | BC660K-GL / BC950K-GL | BC66 / BC66-NA | BG95 / BG77 / BG600L |
| :--- | :---: | :---: | :---: |
| **AT+CMGF** (簡訊格式) | V | - | V |
| **AT+CMGS** (傳送簡訊) | - | - | V |
| **AT+CMGL** (列出簡訊) | - | - | V |
| **AT+CMGR** (讀取簡訊) | - | - | V |
| **AT+CMGD** (刪除簡訊) | - | - | V |
| **AT+CPMS** (簡訊儲存空間) | - | - | V |

#### 6. 協定與擴充功能指令
| 類別 | BC660K-GL / BC950K-GL | BC66 / BC66-NA | BG95 / BG77 / BG600L |
| :--- | :---: | :---: | :---: |
| **TCP/IP** | V (如 AT+QIOPEN) | V (如 AT+QIOPEN) | V (如 AT+QIOPEN) |
| **MQTT** | V (如 AT+QMTPUB) | V (如 AT+QMTPUB) | V (如 AT+QMTPUB) |
| **DFOTA** | V (AT+QFOTADL) | V (AT+QFOTADL) | V (AT+QFOTADL) |
| **LwM2M** | - | V (如 AT+QLWREG) | V (如 AT+QLWSVC) |
| **SSL** | - | V (如 AT+QSSLSEND) | V (如 AT+QSSLOPEN) |
| **GNSS** | - | - | V (如 AT+QGPS) |
| **HTTP(S)** | - | - | V (如 AT+QHTTPURL) |

**補充說明：**
*   **系統配置指令 (`AT+QCFG`)：** 三個系列均支援此指令，但其子參數 (如 "NBcategory", "band", "nwscanmode") 在各模組間有顯著差異。
*   **BG95 系列：** 由於支援 eMTC 與 NB-IoT 雙模，其網路指令 (如 `AT+COPS`) 包含更多關於技術選擇 (`<Act>`) 的參數。
*   **BC660K 系列：** 特別強調對 3GPP R14 協定的支援，例如 `AT+QR14FEATURE`。
