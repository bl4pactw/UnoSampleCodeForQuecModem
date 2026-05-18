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

static const uint16_t   RB_SIZE = 256;
volatile uint8_t        ringBuf[RB_SIZE];
volatile uint16_t       ringBufHead = 0;
volatile uint16_t       ringBufTail = 0;

static const uint16_t   LINE_SIZE = 256;
static char             lineBuf[LINE_SIZE];
static uint16_t         lineLen = 0;

// push one byte into ring buffer
bool ringBufPush(uint8_t c) 
{
  uint16_t next = (ringBufHead + 1) % RB_SIZE;
  if (next == ringBufTail) {
    return false; // full
  }
  ringBuf[ringBufHead] = c;
  ringBufHead = next;
  return true;
}

// pop one byte from ring buffer
bool ringBufPop(uint8_t *c) 
{

  if (ringBufHead == ringBufTail) {
    return false; // empty
  }

  *c = ringBuf[ringBufTail];
  ringBufTail = (ringBufTail + 1) % RB_SIZE;
  return true;
}

// collect bytes from SoftwareSerial into ring buffer
void uartRxTask() 
{
  while (mySerial.available() > 0) {
    uint8_t c = (uint8_t)mySerial.read();

    if (!ringBufPush(c)) {
      dbgSerial.println(F("[RB] overflow"));
    }
  }

}

// parse bytes from ring buffer into line buffer
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

      // only dump the first 16Bytes for check. 打印前16位元確認
      for(uint16_t i=0; i<16; i++) {
        if((uint8_t)lineBuf[i] < 0x10) dbgSerial.print("0");
        dbgSerial.print((uint8_t)lineBuf[i], HEX);
        dbgSerial.print(" ");
      }
      
      dbgSerial.println();
      lineLen = 0;      
    }

  } /* end of while (ringBufPop(&c)) */
  return "";
}

void setup() 
{
  mySerial.begin(115200);     // modem baudrate，改成你的實際值
  dbgSerial.begin();
}

void loop() 
{
  String atrsp = "";
  uartRxTask();      // 持續收 UART
  atrsp = lineParserTask();  // 做行分段解析

  // 這裡可以放其他非阻塞工作
  // 例如狀態機、LED heartbeat、timeout 檢查等
  if (atrsp.length() > 0) {
    dbgSerial.print(F("[MODEM] "));
    dbgSerial.println(atrsp);
  }
}
