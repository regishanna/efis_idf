#include "driver/gpio.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_io_additions.h"

#include "lv_demos.h"

#include <stdio.h>


// LCD spec
// --------

#define LCD_H_RES               (480)
#define LCD_V_RES               (480)
#define LCD_BIT_PER_PIXEL       (16)
#define RGB_BIT_PER_PIXEL       (16)
#define RGB_DATA_WIDTH          (16)
#define RGB_BOUNCE_BUFFER_HEIGHT 20
#define RGB_BOUNCE_BUFFER_SIZE  (LCD_H_RES * RGB_BOUNCE_BUFFER_HEIGHT)

#define LCD_IO_SPI_CS           (GPIO_NUM_48)
#define LCD_IO_SPI_SCL          (GPIO_NUM_47)
#define LCD_IO_SPI_SDA          (GPIO_NUM_21)

#define LCD_IO_RGB_DISP         (-1)             // not used
#define LCD_IO_RGB_VSYNC        (GPIO_NUM_1)
#define LCD_IO_RGB_HSYNC        (GPIO_NUM_2)
#define LCD_IO_RGB_DE           (GPIO_NUM_41)
#define LCD_IO_RGB_PCLK         (GPIO_NUM_42)

#define LCD_IO_RGB_DATA15       (GPIO_NUM_13)   // Red
#define LCD_IO_RGB_DATA14       (GPIO_NUM_12)
#define LCD_IO_RGB_DATA13       (GPIO_NUM_11)
#define LCD_IO_RGB_DATA12       (GPIO_NUM_10)
#define LCD_IO_RGB_DATA11       (GPIO_NUM_9)

#define LCD_IO_RGB_DATA10       (GPIO_NUM_46)   // Green
#define LCD_IO_RGB_DATA9        (GPIO_NUM_3)
#define LCD_IO_RGB_DATA8        (GPIO_NUM_8)
#define LCD_IO_RGB_DATA7        (GPIO_NUM_18)
#define LCD_IO_RGB_DATA6        (GPIO_NUM_17)
#define LCD_IO_RGB_DATA5        (GPIO_NUM_16)

#define LCD_IO_RGB_DATA4        (GPIO_NUM_15)   // Blue
#define LCD_IO_RGB_DATA3        (GPIO_NUM_7)
#define LCD_IO_RGB_DATA2        (GPIO_NUM_6)
#define LCD_IO_RGB_DATA1        (GPIO_NUM_5)
#define LCD_IO_RGB_DATA0        (GPIO_NUM_4)

#define LCD_IO_SPI_CS           (GPIO_NUM_48)
#define LCD_IO_SPI_SCL          (GPIO_NUM_47)
#define LCD_IO_SPI_SDA          (GPIO_NUM_21)

#define LCD_IO_RST              (GPIO_NUM_40)


// LCD settings
// -----------
#define LCD_BUFFER_NUMS 2


static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}

    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t []){0x08}, 1, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t []){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t []){0x10, 0x0C}, 2, 0},
    {0xC2, (uint8_t []){0x31, 0x0A}, 2, 0},
    {0xC3, (uint8_t []){0x02, 0x00, 0x00}, 3, 0},   // PCLK N
    {0xCC, (uint8_t []){0x10}, 1, 0},
    {0xCD, (uint8_t []){0x08}, 1, 0},

    // Gamma settings
    {0xB0, (uint8_t []){0x40, 0x0E, 0x58, 0x0E, 0x12, 0x08, 0x0C, 0x09, 0x09, 0x27, 0x07, 0x18, 0x15, 0x78, 0x26, 0xC7}, 16, 0},
    {0xB1, (uint8_t []){0x40, 0x13, 0x5B, 0x0D, 0x11, 0x06, 0x0A, 0x08, 0x08, 0x26, 0x03, 0x13, 0x12, 0x79, 0x28, 0xC9}, 16, 0},

    // Power Control
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t []){0x6D}, 1, 0},
    {0xB1, (uint8_t []){0x38}, 1, 0},
    {0xB2, (uint8_t []){0x81}, 1, 0},
    {0xB3, (uint8_t []){0x80}, 1, 0},
    {0xB5, (uint8_t []){0x4E}, 1, 0},
    {0xB7, (uint8_t []){0x85}, 1, 0},
    {0xB8, (uint8_t []){0x20}, 1, 0},
    {0xC1, (uint8_t []){0x78}, 1, 0},
    {0xC2, (uint8_t []){0x78}, 1, 0},
    {0xD0, (uint8_t []){0x88}, 1, 0},

    // GIP settings
    {0xE0, (uint8_t []){0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (uint8_t []){0x06, 0x30, 0x08, 0x30, 0x05, 0x30, 0x07, 0x30, 0x00, 0x33, 0x33}, 11, 0},
    {0xE2, (uint8_t []){0x11, 0x11, 0x33, 0x33, 0xF4, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t []){0x0D, 0xF5, 0x30, 0xF0, 0x0F, 0xF7, 0x30, 0xF0, 0x09, 0xF1, 0x30, 0xF0, 0x0B, 0xF3, 0x30, 0xF0}, 16, 0},
    {0xE6, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t []){0x0C, 0xF4, 0x30, 0xF0, 0x0E, 0xF6, 0x30, 0xF0, 0x08, 0xF0, 0x30, 0xF0, 0x0A, 0xF2, 0x30, 0xF0}, 16, 0},
    {0xE9, (uint8_t []){0x36, 0x01}, 2, 0},     // en +
    {0xEB, (uint8_t []){0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40}, 7, 0},
    {0xED, (uint8_t []){0xFF, 0x45, 0x67, 0xFA, 0x01, 0x2B, 0xCF, 0xFF, 0xFF, 0xFC, 0xB2, 0x10, 0xAF, 0x76, 0x54, 0xFF}, 16, 0},
    {0xEF, (uint8_t []){0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0},   // en +

    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x29, (uint8_t []){0x00}, 0, 25},
//    {0x35, (uint8_t []){0x00}, 1, 0},
};    
    
    
void init_graphics(void)
{
    // Install 3-wire SPI panel IO
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = LCD_IO_SPI_CS,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = LCD_IO_SPI_SCL,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = LCD_IO_SPI_SDA,
        .io_expander = NULL,
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    // Install ST7701 panel driver
    esp_lcd_panel_handle_t lcd_handle = NULL;
    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .psram_trans_align = 64,
        .data_width = RGB_DATA_WIDTH,
        .bits_per_pixel = RGB_BIT_PER_PIXEL,
        .de_gpio_num = LCD_IO_RGB_DE,
        .pclk_gpio_num = LCD_IO_RGB_PCLK,
        .vsync_gpio_num = LCD_IO_RGB_VSYNC,
        .hsync_gpio_num = LCD_IO_RGB_HSYNC,
        .disp_gpio_num = LCD_IO_RGB_DISP,
        .data_gpio_nums = {
            LCD_IO_RGB_DATA0,
            LCD_IO_RGB_DATA1,
            LCD_IO_RGB_DATA2,
            LCD_IO_RGB_DATA3,
            LCD_IO_RGB_DATA4,
            LCD_IO_RGB_DATA5,
            LCD_IO_RGB_DATA6,
            LCD_IO_RGB_DATA7,
            LCD_IO_RGB_DATA8,
            LCD_IO_RGB_DATA9,
            LCD_IO_RGB_DATA10,
            LCD_IO_RGB_DATA11,
            LCD_IO_RGB_DATA12,
            LCD_IO_RGB_DATA13,
            LCD_IO_RGB_DATA14,
            LCD_IO_RGB_DATA15,
        },
        .timings = {
            .pclk_hz = 16 * 1000 * 1000,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 8,
            .hsync_back_porch = 56,
            .hsync_front_porch = 20,
            .vsync_pulse_width = 10,
            .vsync_back_porch = 16,
            .vsync_front_porch = 12,
            .flags.pclk_active_neg = true,
        },
        .flags.fb_in_psram = 1,
        .num_fbs = LCD_BUFFER_NUMS,
        .bounce_buffer_size_px = RGB_BOUNCE_BUFFER_SIZE,
    };
    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .auto_del_panel_io = 0,         /**
                                             * Set to 1 if panel IO is no longer needed after LCD initialization.
                                             * If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
                                             * Please set it to 1 to release the pins.
                                             */
            .mirror_by_cmd = 1,             // Set to 0 if `auto_del_panel_io` is enabled
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_IO_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));

    // Initialize LVGL port
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = lcd_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES,
        .double_buffer = 0,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
            .direct_mode = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = false,
#endif
        }
    };
    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = false,
            .avoid_tearing = true,
        }
    };
    assert(lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg) != NULL);
}


void app_main(void)
{
    init_graphics();

    assert(lvgl_port_lock(0) == true);
    lv_demo_music();
    lvgl_port_unlock();
}
