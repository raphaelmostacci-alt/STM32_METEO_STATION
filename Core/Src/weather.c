/**
  ******************************************************************************
  * @file    weather.c
  * @brief   Weather station sensor + LED + UART logic, on top of IKS01A3 BSP.
  ******************************************************************************
  */
#include "weather.h"

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "iks01a3_env_sensors.h"

#define WEATHER_HUM_SENSOR   IKS01A3_HTS221_0
#define WEATHER_PRESS_SENSOR IKS01A3_LPS22HH_0

#ifdef IKS01A3_STTS751_0
  #define WEATHER_TEMP_SENSOR  IKS01A3_STTS751_0
#else
  #define WEATHER_TEMP_SENSOR  IKS01A3_HTS221_0
#endif

static uint8_t s_hum_ok   = 0;
static uint8_t s_press_ok = 0;
static uint8_t s_temp_ok  = 0;

int Weather_InitSensors(void)
{
    int rc = 0;

    if (IKS01A3_ENV_SENSOR_Init(WEATHER_HUM_SENSOR, ENV_HUMIDITY) == BSP_ERROR_NONE &&
        IKS01A3_ENV_SENSOR_Enable(WEATHER_HUM_SENSOR, ENV_HUMIDITY) == BSP_ERROR_NONE) {
        s_hum_ok = 1;
    } else {
        rc = -1;
    }

    if (IKS01A3_ENV_SENSOR_Init(WEATHER_PRESS_SENSOR, ENV_PRESSURE) == BSP_ERROR_NONE &&
        IKS01A3_ENV_SENSOR_Enable(WEATHER_PRESS_SENSOR, ENV_PRESSURE) == BSP_ERROR_NONE) {
        s_press_ok = 1;
    } else {
        rc = -1;
    }

    if (IKS01A3_ENV_SENSOR_Init(WEATHER_TEMP_SENSOR, ENV_TEMPERATURE) == BSP_ERROR_NONE &&
        IKS01A3_ENV_SENSOR_Enable(WEATHER_TEMP_SENSOR, ENV_TEMPERATURE) == BSP_ERROR_NONE) {
        s_temp_ok = 1;
    } else {
        rc = -1;
    }

    return rc;
}

int Weather_ReadSensors(WeatherSample *out)
{
    if (out == NULL) return -1;

    float v = 0.0f;
    uint8_t any_err = 0;

    out->temperature_c = 0.0f;
    out->humidity_pct  = 0.0f;
    out->pressure_hpa  = 0.0f;

    if (s_temp_ok &&
        IKS01A3_ENV_SENSOR_GetValue(WEATHER_TEMP_SENSOR, ENV_TEMPERATURE, &v) == BSP_ERROR_NONE) {
        out->temperature_c = v;
    } else {
        any_err = 1;
    }

    if (s_hum_ok &&
        IKS01A3_ENV_SENSOR_GetValue(WEATHER_HUM_SENSOR, ENV_HUMIDITY, &v) == BSP_ERROR_NONE) {
        out->humidity_pct = v;
    } else {
        any_err = 1;
    }

    if (s_press_ok &&
        IKS01A3_ENV_SENSOR_GetValue(WEATHER_PRESS_SENSOR, ENV_PRESSURE, &v) == BSP_ERROR_NONE) {
        out->pressure_hpa = v;
    } else {
        any_err = 1;
    }

    out->valid = any_err ? 0u : 1u;
    return any_err ? -1 : 0;
}

/* L0 = sample valid, L1 = sensor error */
void Weather_UpdateLeds(const WeatherSample *s)
{
    GPIO_PinState ok  = (s && s->valid)  ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState err = (s && !s->valid) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(L0_GPIO_Port, L0_Pin, ok);
    HAL_GPIO_WritePin(L1_GPIO_Port, L1_Pin, err);
}

void Weather_PrintUart(UART_HandleTypeDef *huart,
                       const char *city,
                       const WeatherSample *s)
{
    static char buf[128];

    int n = snprintf(buf, sizeof(buf),
                     "METEO Ville=%s T=%.2f C RH=%.2f %% P=%.1f hPa%s\r\n",
                     (city && city[0]) ? city : "Inconnue",
                     s->temperature_c,
                     s->humidity_pct,
                     s->pressure_hpa,
                     s->valid ? "" : " [SENSOR_ERR]");
    if (n < 0) return;
    if (n > (int)sizeof(buf)) n = (int)sizeof(buf);
    HAL_UART_Transmit(huart, (uint8_t *)buf, (uint16_t)n, 100u);
}
