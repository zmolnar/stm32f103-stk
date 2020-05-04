/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palClearPad(GPIOC, GPIOC_LED);
    chThdSleepMilliseconds(100);
    palSetPad(GPIOC, GPIOC_LED);
    chThdSleepMilliseconds(100);
  }
}

const SPIConfig epdSpiConfig = {
    .end_cb = NULL,
    .ssport = GPIOB,
    .sspad  = GPIOB_SPI2_NSS,
    .cr1    = (((0x2 << 3) & SPI_CR1_BR) | SPI_CR1_MSTR),
};

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  spiObjectInit(&SPID2);
  spiStart(&SPID2, &epdSpiConfig);

  // Power ON
  palSetPad(GPIOB, GPIOB_EPD_VCC_EN);
  chThdSleepMilliseconds(100);

  palClearPad(GPIOB, GPIOB_EPD_VCC_EN);
  spiUnselect(&SPID2);
  palSetPad(GPIOA, GPIOA_EPD_RESET);
  chThdSleepMilliseconds(5);
  
  palClearPad(GPIOA, GPIOA_EPD_RESET);
  chThdSleepMilliseconds(5);

  palSetPad(GPIOA, GPIOA_EPD_RESET);
  chThdSleepMilliseconds(5);

  // Initialize
  bool busy = true;
  while(busy) {
    busy = PAL_HIGH == palReadPad(GPIOA, GPIOA_EPD_BUSY);
    chThdSleepMilliseconds(1);
  }

  // Read COG ID
  uint8_t cmd[] = {0x71};
  uint8_t id = 0;
  spiSelect(&SPID2);
  spiSend(&SPID2, 1, cmd);
  spiReceive(&SPID2, 1, &id);
  spiUnselect(&SPID2);

  if (0x12 == id) {
    chThdSleepMilliseconds(100);
  } else {
    chThdSleepMilliseconds(200);
  }


  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    chThdSleepMilliseconds(500);
  }
}
