#include "display.h"

#include "driver/gpio.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_log.h"

#include <stdio.h>


static const char *TAG = "EFIS main";


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
// ------------
#define LCD_BUFFER_NUMS 2


static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}

    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t []){0x08}, 1, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t []){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t []){0x10, 0x0C}, 2, 0},
    {0xC2, (uint8_t []){0x31, 0x00}, 2, 0},
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
};    
    
    
static void init_graphics(void)
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

    esp_lcd_rgb_panel_config_t rgb_config = {};

    rgb_config.clk_src = LCD_CLK_SRC_DEFAULT;
    rgb_config.psram_trans_align = 64;
    rgb_config.data_width = RGB_DATA_WIDTH;
    rgb_config.bits_per_pixel = RGB_BIT_PER_PIXEL;
    rgb_config.num_fbs = LCD_BUFFER_NUMS;
    rgb_config.bounce_buffer_size_px = RGB_BOUNCE_BUFFER_SIZE;
    rgb_config.de_gpio_num = LCD_IO_RGB_DE;
    rgb_config.pclk_gpio_num = LCD_IO_RGB_PCLK;
    rgb_config.vsync_gpio_num = LCD_IO_RGB_VSYNC;
    rgb_config.hsync_gpio_num = LCD_IO_RGB_HSYNC;
    rgb_config.disp_gpio_num = LCD_IO_RGB_DISP;

    rgb_config.data_gpio_nums[0] = LCD_IO_RGB_DATA0;
    rgb_config.data_gpio_nums[1] = LCD_IO_RGB_DATA1;
    rgb_config.data_gpio_nums[2] = LCD_IO_RGB_DATA2;
    rgb_config.data_gpio_nums[3] = LCD_IO_RGB_DATA3;
    rgb_config.data_gpio_nums[4] = LCD_IO_RGB_DATA4;
    rgb_config.data_gpio_nums[5] = LCD_IO_RGB_DATA5;
    rgb_config.data_gpio_nums[6] = LCD_IO_RGB_DATA6;
    rgb_config.data_gpio_nums[7] = LCD_IO_RGB_DATA7;
    rgb_config.data_gpio_nums[8] = LCD_IO_RGB_DATA8;
    rgb_config.data_gpio_nums[9] = LCD_IO_RGB_DATA9;
    rgb_config.data_gpio_nums[10] = LCD_IO_RGB_DATA10;
    rgb_config.data_gpio_nums[11] = LCD_IO_RGB_DATA11;
    rgb_config.data_gpio_nums[12] = LCD_IO_RGB_DATA12;
    rgb_config.data_gpio_nums[13] = LCD_IO_RGB_DATA13;
    rgb_config.data_gpio_nums[14] = LCD_IO_RGB_DATA14;
    rgb_config.data_gpio_nums[15] = LCD_IO_RGB_DATA15;

    rgb_config.timings.pclk_hz = 16 * 1000 * 1000;
    rgb_config.timings.h_res = LCD_H_RES;
    rgb_config.timings.v_res = LCD_V_RES;
    rgb_config.timings.hsync_pulse_width = 10;
    rgb_config.timings.hsync_back_porch = 10;
    rgb_config.timings.hsync_front_porch = 20;
    rgb_config.timings.vsync_pulse_width = 10;
    rgb_config.timings.vsync_back_porch = 16;
    rgb_config.timings.vsync_front_porch = 12;
    rgb_config.timings.flags.pclk_active_neg = 1;

    rgb_config.flags.fb_in_psram = 1;

    st7701_vendor_config_t vendor_config = {};
    vendor_config.rgb_config = &rgb_config;
    vendor_config.init_cmds = lcd_init_cmds,
    vendor_config.init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
    vendor_config.flags.auto_del_panel_io = 0;  /**
                                                 * Set to 1 if panel IO is no longer needed after LCD initialization.
                                                 * If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
                                                 * Please set it to 1 to release the pins.
                                                 */
    vendor_config.flags.mirror_by_cmd = 1;      // Set to 0 if `auto_del_panel_io` is enabled

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_IO_RST;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = LCD_BIT_PER_PIXEL;
    panel_config.vendor_config = &vendor_config;

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));

    // Initialize LVGL port
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // Add LCD screen
    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.panel_handle = lcd_handle;
    disp_cfg.buffer_size = LCD_H_RES * LCD_V_RES;
    disp_cfg.double_buffer = 0;
    disp_cfg.hres = LCD_H_RES;
    disp_cfg.vres = LCD_V_RES;
    disp_cfg.monochrome = false;
#if LVGL_VERSION_MAJOR >= 9
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
#endif
    disp_cfg.flags.direct_mode = true;

    lvgl_port_display_rgb_cfg_t rgb_cfg = {};
    rgb_cfg.flags.avoid_tearing = true;

    assert(lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg) != NULL);
}


#define ROLL_STEP 100
#define PITCH_STEP 10

static void my_timer(lv_timer_t * timer)
{
    display *disp = static_cast<display *>(lv_timer_get_user_data(timer));

    static int16_t roll = 0;
    static int16_t roll_step = ROLL_STEP;
    static int16_t pitch = 0;
    static int16_t pitch_step = PITCH_STEP;

    disp->set_attitude(roll, pitch);

    if (roll >= 900) {
        roll_step = -ROLL_STEP;
    } else if (roll <= -900) {
        roll_step = ROLL_STEP;
    }
    roll += roll_step;

    if (pitch >= 200) {
        pitch_step = -PITCH_STEP;
    } else if (pitch <= -200) {
        pitch_step = PITCH_STEP;
    }
    pitch += pitch_step;
}


extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Main task priority = %d", uxTaskPriorityGet(NULL));

    init_graphics();

    display *disp = new display();

#if 1
    lv_timer_create(my_timer, 30, disp);
#endif

}
