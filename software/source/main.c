

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

static THD_FUNCTION(Thread1, arg)
{
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

void EPD_Write(SPIDriver *pspi, uint8_t cmd, const uint8_t data[], size_t length)
{
  uint8_t cmdheader  = 0x70;
  uint8_t dataheader = 0x72;

  spiSelect(pspi);
  spiSend(pspi, 1, &cmdheader);
  spiSend(pspi, 1, &cmd);
  spiUnselect(pspi);

  //chThdSleepMicroseconds(1);

  spiSelect(pspi);
  spiSend(pspi, 1, &dataheader);
  spiSend(pspi, length, data);
  spiUnselect(pspi);
}

void EPD_Read(SPIDriver *pspi, uint8_t cmd, uint8_t data[], size_t length)
{
  uint8_t cmdheader  = 0x70;
  uint8_t dataheader = 0x73;

  spiSelect(pspi);
  spiSend(pspi, 1, &cmdheader);
  spiSend(pspi, 1, &cmd);
  spiUnselect(pspi);

  chThdSleepMicroseconds(1);

  spiSelect(pspi);
  spiSend(pspi, 1, &dataheader);
  spiReceive(pspi, length, data);
  spiUnselect(pspi);
}

void EPD_PowerOn(void)
{
  // Power ON
  palClearPad(GPIOB, GPIOB_EPD_DISCHARGE);
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

  while (busy) {
    busy = PAL_HIGH == palReadPad(GPIOA, GPIOA_EPD_BUSY);
    chThdSleepMilliseconds(1);
  }

  // Read COG ID
  uint8_t rid = 0x71;
  uint8_t id = 0;

  spiSelect(&SPID2);
  spiSend(&SPID2, 1, &rid);
  spiReceive(&SPID2, 1, &id);
  spiUnselect(&SPID2);

  if (0x12 != id) {
    chSysHalt("wrong chip-id");
  }

  uint8_t cmd;
  uint8_t wrdata[10];
  uint8_t rddata[10];

  // Disable OE
  cmd       = 0x02;
  wrdata[0] = 0x40;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  // Breakage detection
  cmd = 0x0f;
  EPD_Read(&SPID2, cmd, rddata, 1);

  if (0x80 != (rddata[0] & 0x80))
    chSysHalt("panel broken");

  // Power saving mode
  cmd       = 0x0b;
  wrdata[0] = 0x02;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  // Channel select
  // (0x0000,0003,FC00,00FF)
  cmd       = 0x01;
  wrdata[0] = 0x00;
  wrdata[1] = 0x00;
  wrdata[2] = 0x00;
  wrdata[3] = 0x03;
  wrdata[4] = 0xfc;
  wrdata[5] = 0x00;
  wrdata[6] = 0x00;
  wrdata[7] = 0xff;
  EPD_Write(&SPID2, cmd, wrdata, 8);

  // High power mode
  cmd       = 0x07;
  wrdata[0] = 0xd1;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  // Power setting
  cmd       = 0x08;
  wrdata[0] = 0x02;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  // Set VCOM level
  cmd       = 0x09;
  wrdata[0] = 0xc2;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  // Power setting
  cmd       = 0x04;
  wrdata[0] = 0x03;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  // Driver latch on
  cmd       = 0x03;
  wrdata[0] = 0x01;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  // Driver latch off
  cmd       = 0x03;
  wrdata[0] = 0x00;
  EPD_Write(&SPID2, cmd, wrdata, 1);

  chThdSleepMilliseconds(5);

  bool ready = false;
  size_t i   = 0;

  for (i = 1; !ready; ++i) {
    if (4 <= i)
      chSysHalt("charge pump failed");

    // Start charge pump positive voltage
    cmd       = 0x05;
    wrdata[0] = 0x01;
    EPD_Write(&SPID2, cmd, wrdata, 1);

    chThdSleepMilliseconds(170);

    // Start charge pump negative voltage
    cmd       = 0x05;
    wrdata[0] = 0x03;
    EPD_Write(&SPID2, cmd, wrdata, 1);

    chThdSleepMicroseconds(110);

    // Set charge pump VCOM on
    cmd       = 0x05;
    wrdata[0] = 0x0f;
    EPD_Write(&SPID2, cmd, wrdata, 1);

    chThdSleepMilliseconds(60);

    cmd = 0x0f;
    EPD_Read(&SPID2, cmd, rddata, 1);

    if (0x40 == (rddata[0] & 0x40))
      ready = true;
  }

  // Output enable to disable
  cmd       = 0x02;
  wrdata[0] = 0x06;
  EPD_Write(&SPID2, cmd, wrdata, 1);
}

/*
 * Application entry point.
 */
int main(void)
{
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *	 and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *	 RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  spiObjectInit(&SPID2);
  spiStart(&SPID2, &epdSpiConfig);

  EPD_PowerOn();

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    chThdSleepMilliseconds(500);
  }
}
