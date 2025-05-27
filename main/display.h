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
 * @brief Set the attitude of the aircraft
 * 
 * @param roll_d10 roll in tenths of degrees 
 * @param pitch_d10 pitch in tenths of degrees 
 */
void set_attitude(int16_t roll_d10, int16_t pitch_d10);


private:

lv_img_dsc_t *m_static_horizon_img;     // Static horizon image to be used for rendering
lv_obj_t *m_horizon_canvas;             // Canvas to render the horizon

int16_t m_sin_table[3600];      // Precalculated sine/cosine tables * 1024 on 3600 values ​​(0.1 degree step)
int16_t m_cos_table[3600];

/**
 * @brief Create the static horizon image which is used to draw the horizon
 * 
 * @return static horizon image
 */
static lv_img_dsc_t *create_static_horizon_img(void);

/**
 * @brief Rotate (roll) and vertically translate (pitch) a static horizon image to display it on a canvas
 * 
 * @param canvas canvas to render the horizon
 * @param src_img source image of a static horizon
 * @param angle_d10 roll angle in tenths of degrees
 * @param pitch_px pitch in tenths of degrees
 */
void render_horizon(lv_obj_t *canvas, const lv_img_dsc_t *src_img, int16_t roll_d10, int16_t pitch_d10) const;

/**
 * @brief Create a canvas for the horizon
 * 
 * @return the created canvas
 */
static lv_obj_t *create_horizon_canvas(void);


};


#endif /* DISPLAY_H */
