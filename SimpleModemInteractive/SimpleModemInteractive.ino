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

static const uint16_t   CONSOLE_SIZE = 96;
static char             consoleBuf[CONSOLE_SIZE];
static uint16_t         consoleLen = 0;

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

void printHelp()
{
  dbgSerial.println(F("=== SimpleModemInteractive ==="));
  dbgSerial.println(F("Type AT command in monitor console, then press Enter."));
  dbgSerial.println(F("Examples: AT, ATI, AT+CPIN?, AT+CEREG?"));
  dbgSerial.println(F("Local commands: /help, /clear"));
  dbgSerial.println();
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

bool isKnownUrc(const String &line)
{
  return (line == "RDY") ||
         (line == "APP RDY") ||
         line.startsWith("+CPIN:") ||
         line.startsWith("+CEREG:") ||
         line.startsWith("+CGREG:") ||
         line.startsWith("+CREG:") ||
         line.startsWith("+CSCON:") ||
         line.startsWith("+CGEV:") ||
         line.startsWith("+QIURC:") ||
         line.startsWith("+NPSMR:");
}

void modemLineTask(const String &line)
{
  if (line.length() == 0) {
    return;
  }

  if (isKnownUrc(line)) {
    dbgSerial.print(F("[URC] "));
  }
  else {
    dbgSerial.print(F("[MODEM] "));
  }

  dbgSerial.println(line);
}

void clearConsoleInput()
{
  consoleLen = 0;
  consoleBuf[0] = '\0';
}

void sendConsoleCommand()
{
  consoleBuf[consoleLen] = '\0';

  if (consoleLen == 0) {
    return;
  }

  if (strcmp(consoleBuf, "/help") == 0) {
    printHelp();
    return;
  }

  if (strcmp(consoleBuf, "/clear") == 0) {
    dbgSerial.println(F("[SYS] input buffer cleared"));
    return;
  }

  dbgSerial.print(F("[USER][TX] "));
  dbgSerial.println(consoleBuf);

  mySerial.print(consoleBuf);
  mySerial.print("\r\n");
}

void consoleRxTask()
{
  while (dbgSerial.available() > 0) {
    char c = (char)dbgSerial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      sendConsoleCommand();
      clearConsoleInput();
      continue;
    }

    if ((c == '\b') || (c == 0x7F)) {
      if (consoleLen > 0) {
        consoleLen--;
        consoleBuf[consoleLen] = '\0';
      }
      continue;
    }

    if (consoleLen < (CONSOLE_SIZE - 1)) {
      consoleBuf[consoleLen++] = c;
    }
    else {
      dbgSerial.println(F("[CONSOLE] command too long, input cleared"));
      clearConsoleInput();
    }
  }
}

void setup()
{
  mySerial.begin(MODEM_BAUDRATE);
  debugBegin();
  clearConsoleInput();
  printHelp();
}

void loop()
{
  String line = "";

  consoleRxTask();       // read monitor console and send user AT command
  uartRxTask();          // read modem UART
  line = lineParserTask();
  modemLineTask(line);   // print modem response and URC
}
