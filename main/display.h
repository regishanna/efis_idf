/**
 * @file display.h
 * @brief Build and update the EFIS display
 * 
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "lvgl.h"


class display
{

public:

/**
 * @brief Construct a new display object
 * 
 */
display(void);

/**
 * @brief Set roll to display
 * 
 * @param[in] roll_tenths_degrees roll in tenths of degrees
 */
void set_roll(int16_t roll_tenths_degrees);

/**
 * @brief Set pitch to display
 * 
 * @param pitch_tenths_degrees pitch in tenths of degrees
 */
void set_pitch(int16_t pitch_tenths_degrees);


private:

lv_obj_t *m_sky_earth;

};


#endif /* DISPLAY_H */
