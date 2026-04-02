#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#include <unistd.h>

#define I2C_BUS_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_MASTER_FREQ_HZ 100000
#define SLAVE_ADDRESS_LCD 0x27
#define LCD_BACKLIGHT 0x08

static const char* TAG = "I2C_LCD";

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t lcd_dev = NULL;

static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SLAVE_ADDRESS_LCD,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &lcd_dev));

    ESP_LOGI(TAG, "I2C master bus initialized");
    return ESP_OK;
}

static esp_err_t lcd_i2c_write(uint8_t *data, size_t len)
{
    return i2c_master_transmit(lcd_dev, data, len, -1);
}

void lcd_send_data(uint8_t data)
{
    esp_err_t err;
    uint8_t data_u = data & 0xF0;
    uint8_t data_l = (data << 4) & 0xF0;

    uint8_t data_t[4] = {
        data_u | 0b00000101 | LCD_BACKLIGHT,  // EN=1, RS=1
        data_u | 0b00000001 | LCD_BACKLIGHT,  // EN=0, RS=1
        data_l | 0b00000101 | LCD_BACKLIGHT,
        data_l | 0b00000001 | LCD_BACKLIGHT
    };

    err = lcd_i2c_write(data_t, 4);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C data error: %d", err);
    }
}

void lcd_send_cmd(uint8_t cmd)
{
    esp_err_t err;
    uint8_t data_u = cmd & 0xF0;
    uint8_t data_l = (cmd << 4) & 0xF0;
    uint8_t data_t[4] = {
        data_u | 0b00000100 | LCD_BACKLIGHT,  // EN=1, RS=0
        data_u | 0b00000000 | LCD_BACKLIGHT,  // EN=0, RS=0
        data_l | 0b00000100 | LCD_BACKLIGHT,
        data_l | 0b00000000 | LCD_BACKLIGHT
    };

    err = lcd_i2c_write(data_t, 4);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C command error: %d", err);
    }
}

void lcd_init (void)
{
	// 4 bit initialisation
	usleep(50000);  // wait for >40ms
	lcd_send_cmd (0x30);
	usleep(4500);  // wait for >4.1ms
	lcd_send_cmd (0x30);
	usleep(200);  // wait for >100us
	lcd_send_cmd (0x30);
	usleep(200);
	lcd_send_cmd (0x20);  // 4bit mode
	usleep(200);

  // dislay initialisation
	lcd_send_cmd (0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	vTaskDelay(pdMS_TO_TICKS(1));
	lcd_send_cmd (0x08); //Display on/off control --> D=0,C=0, B=0  ---> display off
	vTaskDelay(pdMS_TO_TICKS(1));
	lcd_send_cmd (0x01);  // clear display
	vTaskDelay(pdMS_TO_TICKS(1));
	vTaskDelay(pdMS_TO_TICKS(1));
	lcd_send_cmd (0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	vTaskDelay(pdMS_TO_TICKS(1));;
	lcd_send_cmd (0x0C); //Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
	vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_put_cur(int row, int col)
{
    switch (row)
    {
        case 0:
            col |= 0x80;
            break;
        case 1:
            col |= 0xC0;
            break;
    }

    lcd_send_cmd (col);
}

void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++);
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));;  // minimal 1.5ms
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    lcd_init();
    lcd_clear();

    lcd_put_cur(0, 0);
    lcd_send_string("Hello World!");

    lcd_put_cur(1, 0);
    lcd_send_string("from ESP32");
}