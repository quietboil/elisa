#ifndef __BLINK_H
#define __BLINK_H

/**
 * \brief Initializes and starts LED blinking
 */
void blink_init(void);

/**
 * \brief Stops LED blinking.
 * \note  LED will be off while it is not blinking.
 */
void blink_stop(void);

/**
 * \brief Restarts LED blinking
 */
void blink_restart(void);

/**
 * \brief Changes mask that defines the blinking pattern.
 *
 *        LED is on when all masked bits are 0.
 * 
 * \param new_mask New 8-bit mask.
 * 
 */
void blink_set_mask(uint8_t new_mask);

#endif