#include "display.h"

#include "esp_lvgl_port.h"

#include <math.h>


#define FULL_HEIGHT_PITCH_D10 500   // Full height pitch in tenths of degrees


display::display(void)
{
    assert(lvgl_port_lock(0) == true);

    // Precalculate sine and cosine tables
    for (int i = 0; i < 3600; i++) {
        double angle_rad = static_cast<double>(i) * M_PI / 180.0 / 10.0;
        m_sin_table[i] = static_cast<int16_t>(sin(angle_rad) * 1024.0);
        m_cos_table[i] = static_cast<int16_t>(cos(angle_rad) * 1024.0);
    }

    // Create parts of the display
    m_static_horizon_img = create_static_horizon_img();
    m_horizon_canvas = create_horizon_canvas();

    // Set the initial attitude
    set_attitude(0, 0);

    lvgl_port_unlock();
}


lv_img_dsc_t *display::create_static_horizon_img(void)
{
    double ratio = 2.0;     // Ratio between size of horizon canvas and screen size
    lv_obj_t *screen = lv_scr_act();
    int32_t width = static_cast<int32_t>(static_cast<double>(lv_obj_get_width(screen)) * ratio);
    int32_t height = static_cast<int32_t>(static_cast<double>(lv_obj_get_height(screen)) * ratio);

    lv_draw_buf_t *draw_buf = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_RGB565, 0);
    assert(draw_buf != NULL);

    lv_obj_t *canvas = lv_canvas_create(NULL);
    assert(canvas != NULL);
    lv_canvas_set_draw_buf(canvas, draw_buf);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    // Draw earth and sky
    {
        // Caracteristics of rectangles
        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.radius = 0;
        rect_dsc.border_width = 0;

        // Draw the sky
        rect_dsc.bg_color = lv_palette_main(LV_PALETTE_LIGHT_BLUE);
        lv_area_t coords_sky = {0, 0, width - 1, (height / 2) - 1};
        lv_draw_rect(&layer, &rect_dsc, &coords_sky);

        // Draw the earth
        rect_dsc.bg_color = lv_palette_main(LV_PALETTE_BROWN);
        lv_area_t coords_earth = {0, height / 2, width - 1, height - 1};
        lv_draw_rect(&layer, &rect_dsc, &coords_earth);
    }

    // Draw pitch lines
    {
        lv_draw_rect_dsc_t line_dsc;
        lv_draw_rect_dsc_init(&line_dsc);
        line_dsc.bg_color = lv_color_white();
    
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = lv_color_white();
        label_dsc.text_local = 1;
        label_dsc.font = &lv_font_montserrat_18;

        int32_t font_half_height = lv_font_get_line_height(label_dsc.font) / 2;
        int32_t label_x_spacing = 15;   // Spacing between the pitch line and the label text
    
        const int cx = width / 2;
        const int cy = height / 2;
        const int pitch_step_d10 = 25; // Pitch step in tenths of degrees
        const int pitch_step_px = pitch_step_d10 * lv_obj_get_height(screen) / FULL_HEIGHT_PITCH_D10;
        const int max_pitch_d10 = 500;
    
        for (int angle_d10 = -max_pitch_d10; angle_d10 <= max_pitch_d10; angle_d10 += pitch_step_d10) {
            int y = cy - (angle_d10 * pitch_step_px) / pitch_step_d10;
            if (y < 0 || y >= height) continue;
    
            int len = (angle_d10 % (4 * pitch_step_d10) == 0) ? 40 :
                (angle_d10 % (2 * pitch_step_d10) == 0) ? 20 : 10;
    
            // Lines (not for 0 pitch)
            if (angle_d10 != 0) {
                lv_area_t coords_line = {cx - len, y - 1, cx + len, y};
                lv_draw_rect(&layer, &line_dsc, &coords_line);
            }

            // Numbers (only for multiples of 4 pitch steps but not 0)
            if ((angle_d10 != 0) && (angle_d10 % (4 * pitch_step_d10) == 0)) {
                char txt[8];
                snprintf(txt, sizeof(txt), "%d", abs(angle_d10) / 10);
                label_dsc.text = txt;
                
                // Right number
                label_dsc.align = LV_TEXT_ALIGN_LEFT;
                lv_area_t coords_text_right = {cx + len + label_x_spacing, y - font_half_height, width, y + font_half_height};
                lv_draw_label(&layer, &label_dsc, &coords_text_right);

                // Left number
                label_dsc.align = LV_TEXT_ALIGN_RIGHT;
                lv_area_t coords_text_left = {0, y - font_half_height, cx - len - label_x_spacing, y + font_half_height};
                lv_draw_label(&layer, &label_dsc, &coords_text_left);
            }
        }
    }

    lv_canvas_finish_layer(canvas, &layer);

    return lv_canvas_get_image(canvas);
}


void display::render_horizon(lv_obj_t *canvas, const lv_img_dsc_t *src_img, int16_t roll_d10, int16_t pitch_d10) const
{
    assert(lv_canvas_get_image(canvas)->header.cf == LV_COLOR_FORMAT_RGB565);
    const int canvas_w = lv_obj_get_width(canvas);
    const int canvas_h = lv_obj_get_height(canvas);
    const int cx = canvas_w / 2;
    const int cy = canvas_h / 2;

    assert(src_img->header.cf == LV_COLOR_FORMAT_RGB565);
    const int img_w = src_img->header.w;
    const int img_h = src_img->header.h;
    const int sx = img_w / 2;
    const int sy = img_h / 2;

    // Convert pitch from tenths of degrees to pixels
    const int pitch_px = pitch_d10 * canvas_h / FULL_HEIGHT_PITCH_D10;

    // Get source and destination buffers
    lv_color16_t *dst_buf = (lv_color16_t *)lv_canvas_get_buf(canvas);
    lv_color16_t *src_buf = (lv_color16_t *)src_img->data;

    // Angle in [0..3599]
    int a = roll_d10 % 3600;
    if (a < 0) a += 3600;

    int16_t cos_a = m_cos_table[a];
    int16_t sin_a = m_sin_table[a];

    for (int y = 0; y < canvas_h; y++) {
        int dy = y - cy;

        for (int x = 0; x < canvas_w; x++) {
            int dx = x - cx;

            // Roll rotation
            int u = ((cos_a * dx) + (sin_a * dy)) >> 10;
            int v = ((-sin_a * dx) + (cos_a * dy)) >> 10;

            // Pitch translation
            v += pitch_px;

            int src_x = sx + u;
            int src_y = sy + v;

            if ((src_x >= 0) && (src_x < img_w) && (src_y >= 0) && (src_y < img_h)) {
                dst_buf[(y * canvas_w) + x] = src_buf[(src_y * img_w) + src_x];
            }
            else {
                dst_buf[(y * canvas_w) + x] = {
                    .blue = 0,
                    .green = 0,
                    .red = 0
                };
            }
        }
    }

    // Mark the canvas as invalid to redrawn its area
    lv_obj_invalidate(canvas);
}


lv_obj_t *display::create_horizon_canvas(void)
{
    lv_obj_t *screen = lv_scr_act();
    int32_t width = lv_obj_get_width(screen);
    int32_t height = lv_obj_get_height(screen);

    lv_draw_buf_t *draw_buf = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_RGB565, 0);
    assert(draw_buf != NULL);

    lv_obj_t *canvas = lv_canvas_create(screen);
    assert(canvas != NULL);
    lv_canvas_set_draw_buf(canvas, draw_buf);

    lv_obj_center(canvas);
    lv_obj_update_layout(canvas);   // To update the size of the canvas
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    return canvas;
}


void display::set_attitude(int16_t roll_d10, int16_t pitch_d10)
{
    //assert(lvgl_port_lock(0) == true);

    render_horizon(m_horizon_canvas, m_static_horizon_img, roll_d10, pitch_d10);

    //lvgl_port_unlock();
}
