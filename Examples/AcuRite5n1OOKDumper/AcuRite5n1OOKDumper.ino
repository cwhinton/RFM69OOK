//
// Simple OOK playground -- receives and dumps received signal and timing
//

#include <RFM69OOK.h>
#include <SPI.h>
#include <RFM69OOKregisters.h>
#include <SimpleFIFO.h>


#define TSIZE 400
#define MAX_0_DUR 100000 // 100 ms
#define MIN_1_DUR 50 // 100 us
#define TOL 50 // +- tolerance
//
// Lifted from acurite.c of rtl_433 project
#define SHORT_LIMIT    = 220  // short pulse is 220 us + 392 us gap
#define LONG_LIMIT     = 408  // long pulse is 408 us + 204 us gap
#define SYNC_WIDTH     = 620  // sync pulse is 620 us + 596 us gap
#define GAP_LIMIT      = 500  // longest data gap is 392 us, sync gap is 596 us
#define RESET_LIMIT    = 4000 // packet gap is 2192 us

RFM69OOK radio;

SimpleFIFO<uint32_t, 10> fifo;

volatile uint32_t val, last_uS = 0;
volatile byte bits, lastPinState = 0;
volatile bool gotone = false;
volatile byte state = 0;

unsigned long last_uS = 0; // 
uint16_t tt[TSIZE];

byte s0 = 0;
byte pos = 0;
bool restart = true;

void setup() {
  Serial.begin(115200);

  radio.initialize();
//  radio.setBandwidth(OOK_BW_10_4);
//  radio.setRSSIThreshold(-70);
  radio.setFixedThreshold(30);
//  radio.setSensitivityBoost(SENSITIVITY_BOOST_HIGH);
  radio.setFrequencyMHz(433.9);
  radio.receiveBegin();

  Serial.println(F("start"));
}

void loop() {

  if (restart) {
    while(!radio.poll());
    last_uS = micros();
    s0 = 1;
    pos = 0;
    restart = false;
    return;
  }

  if (pos >= TSIZE || s0 == 0 && d > MAX_0_DUR) {
    Serial.print("time: ");
    Serial.println(micros() / 1000);
    s = 1;
    for (int i = 0; i < pos; i++) {
      Serial.print(i);
      Serial.print(F(": "));
      Serial.print(s);
      Serial.print(' ');
      Serial.println(tt[i]);
      s = !s;
    }
    restart = true;
  }

}

//
// interruptHander triggered on change (low > high or high < low transition)
void interruptHandler(void) {

  bool currentPinState = radio.poll();            // get current state of pin
  uint32_t current_uS = micros();   // current uSecond counter
  if (last_us < current_us) {  // Haven't wrapped around the micros() counter
      uint32_t duration_uS = current_uS - last_uS;

        if (lastPinState != currentPinState) {  // this seems like an illogical check by definition

            if (currentPinState == 1) {  // end of LOW
                if (duration_us > RESET_LIMIT) {  // We've waited too long, throw out the queue
                    state = 0;
                    bits = 0;
                    val = 0;
                    fifo.flush();
                } else if (duration_uS > GAP_LIMIT) {  // gap between bytes
                    fifo.enqueue(val);
                    val = 0;
                    bits = 0;
                }
            } else {  // end of a HIGH
                if (duration_us > RESET_LIMIT) {  // We've waited too long, throw out the queue
                    state = 0;
                    bits = 0;
                    val = 0;
                    fifo.flush();
                } else if (duration_us > SHORT_LIMIT) {  // This was a bit pulse
                    val <<= 1;
                    bits++;
                    if (duration_uS > LONG_LIMIT) {  // This was a one pulse
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


