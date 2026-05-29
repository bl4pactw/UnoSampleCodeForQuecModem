#include <Arduino.h>

#if defined(ARDUINO_UNO_Q)
#include <Arduino_RouterBridge.h>

#define mySerial        Serial1
#define dbgSerial       Monitor
#endif

#if defined(ARDUINO_ARCH_AVR)
#include <SoftwareSerial.h>

#define mySerial        _mySerial
#define dbgSerial       Serial
SoftwareSerial          _mySerial(12, 13);   // RX, TX
#endif

static const uint32_t   MODEM_BAUDRATE = 115200;
static const uint32_t   DEBUG_BAUDRATE = 115200;

static const uint16_t   RB_SIZE = 256;
volatile uint8_t        ringBuf[RB_SIZE];
volatile uint16_t       ringBufHead = 0;
volatile uint16_t       ringBufTail = 0;

static const uint16_t   LINE_SIZE = 256;
static char             lineBuf[LINE_SIZE];
static uint16_t         lineLen = 0;

static const uint16_t   CMD_SIZE = 96;
static char             currentCmd[CMD_SIZE];
static char             expectPrefix[24];
static uint32_t         cmdSentMs = 0;
static uint32_t         cmdSeq = 0;
static uint32_t         cmdTimeoutMs = 1000;
static uint8_t          cmdRetryLeft = 0;
static bool             waitingCmdRsp = false;
static bool             expectedLineSeen = false;

static const char       PDP_APN[] = "internet";   // 改成 SIM 卡所需 APN

enum CellularState {
  CELL_BOOT,            /* 0 */
  CELL_SYNC_AT,         /* 1 */
  CELL_BASIC_CONFIG,    /* 2 */
  CELL_QUERY_INFO,      /* 3 */
  CELL_CHECK_SIM,       /* 4 */
  CELL_CHECK_SIGNAL,    /* 5 */
  CELL_CONFIG_REG_URC,  /* 6 */
  CELL_WAIT_REGISTER,   /* 7 */
  CELL_CONFIG_PDP,      /* 8 */
  CELL_ATTACH_PACKET,   /* 9 */
  CELL_ACTIVATE_PDP,    /* 10 */
  CELL_NETWORK_READY,   /* 11 */
  CELL_ERROR_RECOVERY   /* 12 */
};

static CellularState    cellState = CELL_BOOT;
static uint8_t          stateStep = 0;
static uint32_t         stateEnterMs = 0;
static uint32_t         lastStatusPollMs = 0;
static bool             commandDone = false;
static bool             commandOk = false;

static bool             modemReady = false;
static bool             simReady = false;
static bool             registeredNetwork = false;
static bool             packetAttached = false;
static bool             pdpActive = false;
static int              ceregStat = -1;

bool ringBufPush(uint8_t c)
{
  uint16_t next = (ringBufHead + 1) % RB_SIZE;
  if (next == ringBufTail) {
    return false;
  }
  ringBuf[ringBufHead] = c;
  ringBufHead = next;
  return true;
}

bool ringBufPop(uint8_t *c)
{
  if (ringBufHead == ringBufTail) {
    return false;
  }

  *c = ringBuf[ringBufTail];
  ringBufTail = (ringBufTail + 1) % RB_SIZE;
  return true;
}

void debugBegin()
{
#if defined(ARDUINO_UNO_Q)
  dbgSerial.begin();
#else
  dbgSerial.begin(DEBUG_BAUDRATE);
#endif
}

void uartRxTask()
{
  while (mySerial.available() > 0) {
    uint8_t c = (uint8_t)mySerial.read();
    if (!ringBufPush(c)) {
      dbgSerial.println(F("[RB] overflow"));
    }
  }
}

String lineParserTask()
{
  uint8_t c;

  while (ringBufPop(&c)) {
    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      if (lineLen > 0) {
        lineBuf[lineLen] = '\0';
        lineLen = 0;
        return lineBuf;
      }
      continue;
    }

    if (lineLen < (LINE_SIZE - 1)) {
      lineBuf[lineLen++] = (char)c;
    }
    else {
      lineBuf[lineLen] = '\0';
      dbgSerial.print(F("[LINE] too long, HEX data: "));
      for (uint16_t i = 0; i < 16; i++) {
        if ((uint8_t)lineBuf[i] < 0x10) dbgSerial.print("0");
        dbgSerial.print((uint8_t)lineBuf[i], HEX);
        dbgSerial.print(" ");
      }
      dbgSerial.println();
      lineLen = 0;
    }
  }

  return "";
}

bool startsWithText(const String &line, const char *prefix)
{
  return (prefix != NULL) && (prefix[0] != '\0') && line.startsWith(prefix);
}

bool isFinalResult(const String &line)
{
  return (line == "OK") ||
         (line == "ERROR") ||
         line.startsWith("+CME ERROR") ||
         line.startsWith("+CMS ERROR");
}

int parseNumberAfterColon(const String &line)
{
  int colon = line.indexOf(':');
  if (colon < 0) {
    return -1;
  }
  return line.substring(colon + 1).toInt();
}

int parseSecondNumber(const String &line)
{
  int comma = line.indexOf(',');
  if (comma < 0) {
    return -1;
  }
  return line.substring(comma + 1).toInt();
}

int parseCeregStat(const String &line)
{
  int stat = parseSecondNumber(line);
  if (stat >= 0) {
    return stat;
  }
  return parseNumberAfterColon(line);
}

void enterState(CellularState nextState)
{
  cellState = nextState;
  stateStep = 0;
  stateEnterMs = millis();

  dbgSerial.print(F("[CELL][STATE] "));
  dbgSerial.println((int)cellState);
}

bool sendAtCommand(const char *cmd, const char *expect, uint32_t timeoutMs, uint8_t retryMax)
{
  if (waitingCmdRsp) {
    return false;
  }

  strncpy(currentCmd, cmd, CMD_SIZE - 1);
  currentCmd[CMD_SIZE - 1] = '\0';

  if (expect == NULL) {
    expectPrefix[0] = '\0';
  }
  else {
    strncpy(expectPrefix, expect, sizeof(expectPrefix) - 1);
    expectPrefix[sizeof(expectPrefix) - 1] = '\0';
  }

  cmdSeq++;
  cmdTimeoutMs = timeoutMs;
  cmdRetryLeft = retryMax;
  cmdSentMs = millis();
  waitingCmdRsp = true;
  expectedLineSeen = (expectPrefix[0] == '\0');
  commandDone = false;
  commandOk = false;

  dbgSerial.print(F("[ATCMD][TX #"));
  dbgSerial.print(cmdSeq);
  dbgSerial.print(F("] "));
  dbgSerial.println(currentCmd);

  mySerial.println(currentCmd);
  return true;
}

void retryCurrentCommand()
{
  cmdSentMs = millis();
  expectedLineSeen = (expectPrefix[0] == '\0');

  dbgSerial.print(F("[ATCMD][RETRY #"));
  dbgSerial.print(cmdSeq);
  dbgSerial.print(F("] "));
  dbgSerial.println(currentCmd);

  mySerial.println(currentCmd);
}

void commandTimeoutTask()
{
  if (!waitingCmdRsp) {
    return;
  }

  if ((uint32_t)(millis() - cmdSentMs) < cmdTimeoutMs) {
    return;
  }

  if (cmdRetryLeft > 0) {
    cmdRetryLeft--;
    retryCurrentCommand();
    return;
  }

  dbgSerial.print(F("[ATCMD][TIMEOUT #"));
  dbgSerial.print(cmdSeq);
  dbgSerial.print(F("] "));
  dbgSerial.println(currentCmd);

  waitingCmdRsp = false;
  commandDone = true;
  commandOk = false;
}

bool handleUrc(const String &line)
{
  if ((line == "RDY") || (line == "APP RDY")) {
    modemReady = true;
    dbgSerial.print(F("[URC][BOOT] "));
    dbgSerial.println(line);
    return true;
  }

  if (line.startsWith("+CPIN:")) {
    simReady = (line.indexOf("READY") >= 0);
    dbgSerial.print(F("[URC][SIM] "));
    dbgSerial.println(line);
    return !waitingCmdRsp;
  }

  if (line.startsWith("+CEREG:")) {
    ceregStat = parseCeregStat(line);
    registeredNetwork = (ceregStat == 1) || (ceregStat == 5);
    dbgSerial.print(F("[URC][REG] stat="));
    dbgSerial.println(ceregStat);
    return !waitingCmdRsp;
  }

  if (line.startsWith("+CSCON:")) {
    dbgSerial.print(F("[URC][RRC] "));
    dbgSerial.println(line);
    return true;
  }

  if (line.startsWith("+CGEV:")) {
    if (line.indexOf("DEACT") >= 0) {
      pdpActive = false;
    }
    dbgSerial.print(F("[URC][PDP] "));
    dbgSerial.println(line);
    return true;
  }

  if (line.startsWith("+QIURC:")) {
    dbgSerial.print(F("[URC][QIURC] "));
    dbgSerial.println(line);
    return true;
  }

  if (line.startsWith("+NPSMR:")) {
    dbgSerial.print(F("[URC][PSM] "));
    dbgSerial.println(line);
    return true;
  }

  return false;
}

void handleCommandLine(const String &line)
{
  if (!waitingCmdRsp) {
    return;
  }

  if (startsWithText(line, expectPrefix)) {
    expectedLineSeen = true;

    if (line.startsWith("+CPIN:")) {
      simReady = (line.indexOf("READY") >= 0);
    }
    else if (line.startsWith("+CEREG:")) {
      ceregStat = parseCeregStat(line);
      registeredNetwork = (ceregStat == 1) || (ceregStat == 5);
    }
    else if (line.startsWith("+CGATT:")) {
      packetAttached = (line.indexOf(": 1") >= 0);
    }
    else if (line.startsWith("+CGPADDR:")) {
      pdpActive = (line.indexOf(".") >= 0);
    }
  }

  if (!isFinalResult(line)) {
    return;
  }

  waitingCmdRsp = false;
  commandDone = true;
  commandOk = (line == "OK") && expectedLineSeen;

  dbgSerial.print(F("[ATCMD][RESULT #"));
  dbgSerial.print(cmdSeq);
  dbgSerial.print(F("] "));
  dbgSerial.print(currentCmd);
  dbgSerial.print(F(" -> "));
  dbgSerial.println(line);
}

void dispatchModemLine(const String &line)
{
  if (line.length() == 0) {
    return;
  }

  dbgSerial.print(F("[MODEM] "));
  dbgSerial.println(line);

  bool urcHandled = handleUrc(line);
  if (urcHandled && !waitingCmdRsp) {
    return;
  }

  handleCommandLine(line);
}

bool commandFinished(bool *ok)
{
  if (!commandDone) {
    return false;
  }

  *ok = commandOk;
  commandDone = false;
  return true;
}

void cellularTask()
{
  bool ok = false;
  char cmd[CMD_SIZE];

  switch (cellState) {
    case CELL_BOOT:
      if (modemReady || ((uint32_t)(millis() - stateEnterMs) > 3000)) {
        enterState(CELL_SYNC_AT);
      }
      break;

    case CELL_SYNC_AT:
      if (stateStep == 0) {
        sendAtCommand("AT", NULL, 1000, 5);
        stateStep = 1;
      }
      else if (commandFinished(&ok)) {
        enterState(ok ? CELL_BASIC_CONFIG : CELL_ERROR_RECOVERY);
      }
      break;

    case CELL_BASIC_CONFIG:
      if (stateStep == 0) {
        sendAtCommand("ATE0", NULL, 1000, 2);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        if (!ok) enterState(CELL_ERROR_RECOVERY);
        else {
          sendAtCommand("AT+CMEE=2", NULL, 1000, 2);
          stateStep = 2;
        }
      }
      else if (stateStep == 2 && commandFinished(&ok)) {
        enterState(ok ? CELL_QUERY_INFO : CELL_ERROR_RECOVERY);
      }
      break;

    case CELL_QUERY_INFO:
      if (stateStep == 0) {
        sendAtCommand("ATI", NULL, 2000, 1);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        sendAtCommand("AT+CGMR", NULL, 2000, 1);
        stateStep = 2;
      }
      else if (stateStep == 2 && commandFinished(&ok)) {
        enterState(CELL_CHECK_SIM);
      }
      break;

    case CELL_CHECK_SIM:
      if (stateStep == 0) {
        sendAtCommand("AT+CPIN?", "+CPIN:", 3000, 10);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        if (ok && simReady) enterState(CELL_CHECK_SIGNAL);
        else enterState(CELL_ERROR_RECOVERY);
      }
      break;

    case CELL_CHECK_SIGNAL:
      if (stateStep == 0) {
        sendAtCommand("AT+CSQ", "+CSQ:", 2000, 2);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        sendAtCommand("AT+CESQ", "+CESQ:", 2000, 1);
        stateStep = 2;
      }
      else if (stateStep == 2 && commandFinished(&ok)) {
        enterState(CELL_CONFIG_REG_URC);
      }
      break;

    case CELL_CONFIG_REG_URC:
      if (stateStep == 0) {
        sendAtCommand("AT+CEREG=2", NULL, 1000, 2);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        sendAtCommand("AT+CSCON=1", NULL, 1000, 1);
        stateStep = 2;
      }
      else if (stateStep == 2 && commandFinished(&ok)) {
        enterState(CELL_WAIT_REGISTER);
      }
      break;

    case CELL_WAIT_REGISTER:
      if (registeredNetwork) {
        enterState(CELL_CONFIG_PDP);
        break;
      }

      if (!waitingCmdRsp && ((uint32_t)(millis() - lastStatusPollMs) >= 3000)) {
        lastStatusPollMs = millis();
        sendAtCommand("AT+CEREG?", "+CEREG:", 3000, 1);
      }

      if (commandFinished(&ok) && registeredNetwork) {
        enterState(CELL_CONFIG_PDP);
      }
      break;

    case CELL_CONFIG_PDP:
      if (stateStep == 0) {
        snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", PDP_APN);
        sendAtCommand(cmd, NULL, 3000, 2);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        enterState(ok ? CELL_ATTACH_PACKET : CELL_ERROR_RECOVERY);
      }
      break;

    case CELL_ATTACH_PACKET:
      if (stateStep == 0) {
        sendAtCommand("AT+CGATT?", "+CGATT:", 3000, 2);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        if (packetAttached) {
          enterState(CELL_ACTIVATE_PDP);
        }
        else {
          sendAtCommand("AT+CGATT=1", NULL, 10000, 3);
          stateStep = 2;
        }
      }
      else if (stateStep == 2 && commandFinished(&ok)) {
        packetAttached = ok;
        enterState(ok ? CELL_ACTIVATE_PDP : CELL_ERROR_RECOVERY);
      }
      break;

    case CELL_ACTIVATE_PDP:
      if (stateStep == 0) {
        sendAtCommand("AT+CGACT=1,1", NULL, 10000, 2);
        stateStep = 1;
      }
      else if (stateStep == 1 && commandFinished(&ok)) {
        sendAtCommand("AT+CGPADDR=1", "+CGPADDR:", 3000, 2);
        stateStep = 2;
      }
      else if (stateStep == 2 && commandFinished(&ok)) {
        enterState((ok && pdpActive) ? CELL_NETWORK_READY : CELL_ERROR_RECOVERY);
      }
      break;

    case CELL_NETWORK_READY:
      if ((uint32_t)(millis() - lastStatusPollMs) >= 10000) {
        lastStatusPollMs = millis();
        dbgSerial.println(F("[CELL] network ready"));
      }
      break;

    case CELL_ERROR_RECOVERY:
      if (!waitingCmdRsp && ((uint32_t)(millis() - stateEnterMs) > 5000)) {
        modemReady = false;
        simReady = false;
        registeredNetwork = false;
        packetAttached = false;
        pdpActive = false;
        enterState(CELL_BOOT);
      }
      break;
  }
}

void setup()
{
  mySerial.begin(MODEM_BAUDRATE);
  debugBegin();
  enterState(CELL_BOOT);
}

void loop()
{
  String line = "";

  uartRxTask();
  commandTimeoutTask();
  cellularTask();

  line = lineParserTask();
  dispatchModemLine(line);
}
