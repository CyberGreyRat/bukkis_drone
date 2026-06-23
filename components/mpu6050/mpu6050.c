#include <math.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_timer.h" // WICHTIG: Für die genaue Zeitmessung (dt)
#include "mpu6050.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000 // Auf 400kHz (Fast Mode) erhöht für schnellere Sensor-Updates
#define I2C_MASTER_TIMEOUT_MS 1000

#define MPU6050_ADDR 0x68
#define MPU6050_PWR_MGMT_1_REG 0x6B
#define MPU6050_CONFIG_REG 0x1A       // Register für den digitalen Low-Pass-Filter (DLPF)
#define MPU6050_GYRO_CONFIG_REG 0x1B  // Gyro Konfiguration
#define MPU6050_ACCEL_XOUT_H_REG 0x3B // Start-Register für alle Daten (Accel + Temp + Gyro)

static const char *TAG = "MPU6050";

// Globale Variablen für den Filter (müssen sich ihren Wert zwischen den Aufrufen merken)
static float current_angle_pitch = 0.0f;
static float current_angle_roll = 0.0f;
static uint64_t last_read_time = 0; // Speichert den Zeitpunkt der letzten Messung in Mikrosekunden

esp_err_t mpu6050_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK)
        return err;

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK)
        return err;

    // 1. Sensor aufwecken (Sleep-Mode deaktivieren)
    uint8_t write_buf_pwr[2] = {MPU6050_PWR_MGMT_1_REG, 0x00};
    err = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf_pwr, 2, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Fehler beim Aufwecken!");
        return err;
    }

    // 2. Hardware Low-Pass-Filter (DLPF) aktivieren!
    // Wert 0x03 bedeutet: Filter bei ~42Hz. Blockt hochfrequente Motorvibrationen ab.
    uint8_t write_buf_dlpf[2] = {MPU6050_CONFIG_REG, 0x03};
    err = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf_dlpf, 2, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Fehler beim DLPF setzen!");
        return err;
    }

    // 3. Gyroskop auf +/- 500 Grad/Sekunde einstellen (bessere Auflösung für Drohnen)
    uint8_t write_buf_gyro[2] = {MPU6050_GYRO_CONFIG_REG, 0x08};
    err = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf_gyro, 2, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    if (err == ESP_OK)
    {
        last_read_time = esp_timer_get_time(); // Startzeitpunkt setzen
        ESP_LOGI(TAG, "Initialisiert, aufgeweckt und Filter aktiviert (400kHz, DLPF 42Hz).");
    }
    return err;
}

// Sensor auslesen und mit Komplementärfilter (Sensor Fusion) berechnen
esp_err_t mpu6050_read_angles(mpu6050_angles_t *angles)
{
    uint8_t reg = MPU6050_ACCEL_XOUT_H_REG;
    uint8_t data[14]; // Wir brauchen jetzt 14 Bytes: 6 für Accel, 2 für Temp, 6 für Gyro

    // Alle Daten in einem Rutsch lesen (das ist extrem effizient!)
    esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg, 1, data, 14, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    if (err == ESP_OK)
    {
        // --- 1. Zeitdifferenz (dt) berechnen ---
        uint64_t current_time = esp_timer_get_time();
        // Zeit in Sekunden umrechnen (esp_timer gibt Mikrosekunden zurück)
        float dt = (float)(current_time - last_read_time) / 1000000.0f;
        last_read_time = current_time;

        // Sicherheits-Check: Wenn dt zu groß (z.B. beim ersten Aufruf), verwerfe die Messung kurz
        if (dt > 0.5f)
            dt = 0.01f;

        // --- 2. Rohdaten zusammensetzen ---
        int16_t accel_x = (data[0] << 8) | data[1];
        int16_t accel_y = (data[2] << 8) | data[3];
        int16_t accel_z = (data[4] << 8) | data[5];
        // data[6] und data[7] sind die Temperatur, die ignorieren wir hier
        int16_t gyro_x_raw = (data[8] << 8) | data[9];
        int16_t gyro_y_raw = (data[10] << 8) | data[11];
        // gyro_z ignorieren wir für Pitch und Roll, wird nur für Gier (Yaw) gebraucht

        // --- 3. Rohdaten skalieren ---
        // Da wir das Gyro in init() auf +/- 500 deg/s gesetzt haben, müssen wir durch 65.5 teilen
        float gyro_x = gyro_x_raw / 65.5f;
        float gyro_y = gyro_y_raw / 65.5f;

        // --- 4. Winkel aus dem Beschleunigungssensor (Accel) berechnen ---
        float pitch_acc = atan2((float)accel_y, (float)accel_z) * 180.0f / M_PI;
        float roll_acc = atan2(-(float)accel_x, sqrt((float)accel_y * accel_y + (float)accel_z * accel_z)) * 180.0f / M_PI;

        // --- 5. DER KOMPLEMENTÄR-FILTER (Sensor Fusion) ---
        // Alpha-Wert: 0.98 vertraut zu 98% dem Gyro (schnell) und zu 2% dem Accel (absolut, aber zittrig)
        float alpha = 0.98f;

        current_angle_pitch = alpha * (current_angle_pitch + gyro_x * dt) + (1.0f - alpha) * pitch_acc;
        current_angle_roll = alpha * (current_angle_roll + gyro_y * dt) + (1.0f - alpha) * roll_acc;

        // --- 6. Werte in die Struktur schreiben ---
        angles->pitch = -current_angle_roll;
        angles->roll = -current_angle_pitch;
    }

    return err;
}