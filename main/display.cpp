#include "display.h"

#include "esp_lvgl_port.h"


display::display(void)
{
    assert(lvgl_port_lock(0) == true);


    // Sky and Earth container
    static lv_style_t style_container;
    lv_style_init(&style_container);
    lv_style_set_border_width(&style_container, 0);
    lv_style_set_radius(&style_container, 0);

    m_sky_earth = lv_obj_create(lv_scr_act());
    lv_obj_add_style(m_sky_earth, &style_container, 0);
    lv_obj_set_size(m_sky_earth, lv_pct(200), lv_pct(200));
    lv_obj_align(m_sky_earth, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_transform_pivot_x(m_sky_earth, lv_pct(50), 0);
    lv_obj_set_style_transform_pivot_y(m_sky_earth, lv_pct(50), 0);


    // Sky
    static lv_style_t style_sky;
    lv_style_init(&style_sky);
    lv_style_set_bg_color(&style_sky, lv_palette_main(LV_PALETTE_LIGHT_BLUE));

    lv_obj_t *sky;
    sky = lv_obj_create(m_sky_earth);
    lv_obj_add_style(sky, &style_container, 0);
    lv_obj_add_style(sky, &style_sky, 0);
    lv_obj_set_size(sky, lv_pct(100), lv_pct(50));
    lv_obj_align(sky, LV_ALIGN_TOP_MID, 0, 0);


    // Earth
    static lv_style_t style_earth;
    lv_style_init(&style_earth);
    lv_style_set_bg_color(&style_earth, lv_palette_main(LV_PALETTE_BROWN));

    lv_obj_t *earth;
    earth = lv_obj_create(m_sky_earth);
    lv_obj_add_style(earth, &style_container, 0);
    lv_obj_add_style(earth, &style_earth, 0);
    lv_obj_set_size(earth, lv_pct(100), lv_pct(50));
    lv_obj_align(earth, LV_ALIGN_BOTTOM_MID, 0, 0);


    lvgl_port_unlock();
}


void display::set_roll(int16_t roll_tenths_degrees)
{
    //assert(lvgl_port_lock(0) == true);

    lv_obj_set_style_transform_angle(m_sky_earth, roll_tenths_degrees, 0);

    //lvgl_port_unlock();
}


void display::set_pitch(int16_t pitch_tenths_degrees)
{
    //assert(lvgl_port_lock(0) == true);

    lv_obj_set_style_translate_y(m_sky_earth, pitch_tenths_degrees, 0);

    //lvgl_port_unlock();
}
