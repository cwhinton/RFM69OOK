
//
// Simple OOK playground -- receives and dumps received signal and timing
//

#include <RFM69OOK.h>
#include <SPI.h>
#include <RFM69OOKregisters.h>
#include <SimpleFIFO.h>

#define DEBUGGING 1
#define TSIZE 400
#define MAX_0_DUR 100000 // 100 ms
#define MIN_1_DUR 50 // 100 us
#define TOL 50 // +- tolerance
//
// Lifted from acurite.c of rtl_433 project
#define SHORT_LIMIT  220  // short pulse is 220 us + 392 us gap
#define LONG_LIMIT   408  // long pulse is 408 us + 204 us gap
#define SYNC_WIDTH   620  // sync pulse is 620 us + 596 us gap
#define GAP_LIMIT    500  // longest data gap is 392 us, sync gap is 596 us
#define RESET_LIMIT  4000 // packet gap is 2192 us

RFM69OOK radio;

SimpleFIFO<uint8_t, 10> fifo;

volatile uint8_t val = 0;
volatile uint32_t last_uS = 0;
volatile byte bits, lastPinState = 0;
volatile bool gotone = false;
volatile byte state = 0;

void setup() {
  Serial.begin(115200);

//  radio.attachUserInterrupt(interruptHandler);
  radio.initialize();
//  radio.setBandwidth(OOK_BW_10_4);
//  radio.setRSSIThreshold(-70);
  radio.setFixedThreshold(30);
//  radio.setSensitivityBoost(SENSITIVITY_BOOST_HIGH);
  radio.setFrequencyMHz(433.92);
  radio.receiveBegin();
  val = 0;
  last_uS = micros();  // prime it

  Serial.println(F("start"));
}

void loop() {

    interruptHandler();
    
    if (fifo.count() > 0) { // wait for a byte to accumulate
        uint8_t lval = fifo.dequeue();

        Serial.print(micros() / 1000);
        Serial.print(F(": "));
        PrintHex83(&lval,4);
        Serial.println();
    }

}

//
// interruptHander triggered on change (low > high or high < low transition)
void interruptHandler(void) {

  bool currentPinState = radio.poll();            // get current state of pin
  uint32_t current_uS = micros();   // current uSecond counter
  if (last_uS < current_uS) {  // Haven't wrapped around the micros() counter
      uint32_t duration_uS = current_uS - last_uS;

        if (lastPinState != currentPinState) {  // this is for debug when not IRQ driven
#if DEBUGGING > 0
            Serial.println("DEBUG: Pin Change Detected");
            Serial.print("duration_uS: ");
            Serial.println(duration_uS);
#endif          

            if (currentPinState == 1) {  // end of LOW
                if (duration_uS > RESET_LIMIT) {  // We've waited too long, throw out the queue
#if DEBUGGING > 0
                    Serial.print("DEBUG: duration_uS > RESET_LIMIT");
#endif            
                    state = 0;
                    bits = 0;
                    val = 0;
                    fifo.flush();
                } else if (duration_uS > GAP_LIMIT) {  // gap between bytes
#if DEBUGGING > 0
                    Serial.print("DEBUG: duration_uS > GAP_LIMIT");
#endif            
                    fifo.enqueue(val);
                    val = 0;
                    bits = 0;
                }
            } else {  // end of a HIGH
                if (duration_uS > RESET_LIMIT) {  // We've waited too long, throw out the queue
#if DEBUGGING > 0
                    Serial.print("DEBUG: duration_uS > RESET_LIMIT");
#endif            
                    state = 0;
                    bits = 0;
                    val = 0;
                    fifo.flush();
                } else if (duration_uS > SHORT_LIMIT) {  // This was a bit pulse
#if DEBUGGING > 0
                    Serial.print("DEBUG: duration_uS > SHORT_LIMIT");
#endif            
                    val <<= 1;
                    bits++;
                    if (duration_uS > LONG_LIMIT) {  // This was a one pulse
#if DEBUGGING > 0
                        Serial.print("DEBUG: duration_uS > LONG_LIMIT");
#endif            
                        val != 1;
                    }
                } else {  // this was a noise pulse
                    // do nothing
                }
            }
        }
    }
    last_uS = current_uS;
    lastPinState = currentPinState;

}

void PrintHex83(uint8_t *data, uint8_t length) // prints 8-bit data in hex
{
    char tmp[length*2+1];
    byte first ;
    int j=0;
    for (uint8_t i=0; i<length; i++)  {
        first = (data[i] >> 4) | 48;
        if (first > 57) tmp[j] = first + (byte)39;
        else tmp[j] = first ;
        j++;

        first = (data[i] & 0x0F) | 48;
        if (first > 57) tmp[j] = first + (byte)39; 
        else tmp[j] = first;
        j++;
    }
    tmp[length*2] = 0;
    Serial.print(tmp);
}

