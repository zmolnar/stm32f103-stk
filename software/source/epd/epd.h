/**
 * @file epd.h
 * @brief
 */

#ifndef EPD_H
#define EPD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <stdint.h>
#include <stddef.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  EPD_STAGE_1,
  EPD_STAGE_2,
  EPD_STAGE_3,
  EPD_STAGE_4,
} EPD_Stage_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void EPD_Write(uint8_t cmd, const uint8_t data[], size_t length);
void EPD_Read(uint8_t cmd, uint8_t data[], size_t length);
void EPD_PowerOn(void);
void EPD_Initialize(void);
void EPD_UpdateDisplay(const uint8_t oldimage[128][144/8], const uint8_t newimage[128][144/8]);
void EPD_PartialUpdate(const uint8_t old[128][144 / 8], const uint8_t new[128][144 / 8]);
void EPD_PowerOff(void);

#endif /* EPD_H */

/****************************** END OF FILE **********************************/
