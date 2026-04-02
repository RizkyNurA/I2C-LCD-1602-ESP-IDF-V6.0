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

void lcd_send_data_lawas(uint8_t data)
{
    esp_err_t err;
    uint8_t data_u = data & 0xF0;
    uint8_t data_l = (data << 4) & 0xF0;

    uint8_t data_t[4] = {
        data_u | 0b00001101 | LCD_BACKLIGHT,  // EN=1, RS=1
        data_u | 0b00001001 | LCD_BACKLIGHT,  // EN=0, RS=1
        data_l | 0b00001101 | LCD_BACKLIGHT,
        data_l | 0b00001001 | LCD_BACKLIGHT
    };

    err = lcd_i2c_write(data_t, 4);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C data error: %d", err);
    }
}

void lcd_send_data(uint8_t data)
{
    uint8_t data_u = data & 0xF0;
    uint8_t data_l = (data << 4) & 0xF0;

    uint8_t data_t[4] = {
        data_u | 0b00001101 | LCD_BACKLIGHT,  // RS=1
        data_u | 0b00001001 | LCD_BACKLIGHT,
        data_l | 0b00001101 | LCD_BACKLIGHT,
        data_l | 0b00001001 | LCD_BACKLIGHT
    };

    for (int i = 0; i < 4; i++) {
        lcd_i2c_write(&data_t[i], 1);
        esp_rom_delay_us(200);
    }
}

void lcd_send_cmd_lawas(uint8_t cmd)
{
    esp_err_t err;
    uint8_t data_u = cmd & 0xF0;
    uint8_t data_l = (cmd << 4) & 0xF0;
    uint8_t data_t[4] = {
        data_u | 0b00001100 | LCD_BACKLIGHT,  // EN=1, RS=0
        data_u | 0b00001000 | LCD_BACKLIGHT,  // EN=0, RS=0
        data_l | 0b00001100 | LCD_BACKLIGHT,
        data_l | 0b00001000 | LCD_BACKLIGHT
    };

    err = lcd_i2c_write(data_t, 4);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C command error: %d", err);
    }
}

void lcd_send_cmd(uint8_t cmd)
{
    uint8_t data_u = cmd & 0xF0;
    uint8_t data_l = (cmd << 4) & 0xF0;

    uint8_t data_t[4] = { 
        data_u | 0b00001100 | LCD_BACKLIGHT,  // EN=1
        data_u | 0b00001000 | LCD_BACKLIGHT,  // EN=0
        data_l | 0b00001100 | LCD_BACKLIGHT,
        data_l | 0b00001000 | LCD_BACKLIGHT
    };

    for (int i = 0; i < 4; i++) {
        lcd_i2c_write(&data_t[i], 1);
        esp_rom_delay_us(50);
    }
}

void lcd_init_lawas_1 (void)
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

void lcd_init_lawas(void)
{
    vTaskDelay(pdMS_TO_TICKS(50)); // >40ms

    // Force 8-bit mode 3x
    lcd_send_cmd(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Switch ke 4-bit
    lcd_send_cmd(0x20);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Sekarang baru aman pakai normal command
    lcd_send_cmd(0x28); // 4-bit, 2 line
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x08); // display off
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x01); // clear
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x06); // entry mode
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x0C); // display on
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd_send_nibble_lawas(uint8_t nibble)
{
    uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT;

    uint8_t seq[2] = {
        data | 0b00001100, // EN=1
        data | 0b00001000  // EN=0
    };

    lcd_i2c_write(seq, 2);
}

void lcd_send_nibble(uint8_t nibble)
{
    uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT;

    uint8_t seq[2] = {
        data | 0b00000100, // EN=1
        data | 0b00000000  // EN=0
    };

    lcd_i2c_write(seq, 2);
    esp_rom_delay_us(50);
}

void lcd_init_new(void)
{
    vTaskDelay(pdMS_TO_TICKS(50)); // >40ms

    // INIT HARUS pakai nibble langsung
    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    // masuk 4-bit
    lcd_send_nibble(0x20);
    vTaskDelay(pdMS_TO_TICKS(5));

    // baru pakai normal command
    lcd_send_cmd(0x28);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x08);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x06);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x0C);
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x20); // masuk 4-bit
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x28);
    lcd_send_cmd(0x08);
    lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
    lcd_send_cmd(0x06);
    lcd_send_cmd(0x0C);
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

void lcd_send_int(int num)
{
    char buf[12]; // cukup untuk int32
    int i = 0;

    if (num == 0) {
        lcd_send_data('0');
        return;
    }

    if (num < 0) {
        lcd_send_data('-');
        num = -num;
    }

    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    // reverse
    for (int j = i - 1; j >= 0; j--) {
        lcd_send_data(buf[j]);
    }
}

void lcd_send_float(float num, int decimal_places)
{
    if (num < 0) {
        lcd_send_data('-');
        num = -num;
    }

    int int_part = (int)num;
    float frac = num - int_part;

    lcd_send_int(int_part);
    lcd_send_data('.');

    for (int i = 0; i < decimal_places; i++) {
        frac *= 10;
        int digit = (int)frac;
        lcd_send_data(digit + '0');
        frac -= digit;
    }
}

void lcd_clear_row(int row)
{
    lcd_put_cur(row, 0);
    for (int i = 0; i < 16; i++) {
        lcd_send_data(' ');
    }
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));;  // minimal 1.5ms
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());

    int a = -100;
    float b = -3.145678;
    int counter = 0;
    lcd_init();
    lcd_clear();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_put_cur(0, 0);
    lcd_send_string("HELLO ");
    lcd_send_int(a);
    lcd_put_cur(1, 0);
    lcd_send_string("WORLD ");
    lcd_send_float(b, 2);

    while (1) {
        lcd_clear();
        lcd_put_cur(0, 0);
        lcd_send_string("Count: ");
        lcd_send_int(counter);
        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
