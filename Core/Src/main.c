/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32_METEO_STATION - environmental weather station on
  *                   NUCLEO-L152RE + X-NUCLEO-IKS01A3.
  *
  *                   I2C1 (PB8/PB9)   -> HTS221, LPS22HH, STTS751
  *                   USART2 (PA2/PA3) -> printf at 115200 8N1
  *                   TIM6 IRQ         -> 1 Hz sample tick
  *                   EXTI13 (PC13)    -> mode toggle (LIVE / FROZEN)
  *                   ADC1 IN0 (PA0)   -> potentiometer = alarm threshold
  *                   GPIO LEDs        -> status, error, mode, alarm, heartbeat
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "weather.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUTTON_DEBOUNCE_MS  50u
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static volatile uint8_t  g_sample_flag    = 0;       /* set by TIM6 IRQ */
static volatile uint8_t  g_ask_city       = 1;       /* set by EXTI13 + at boot */
static volatile uint8_t  g_city_ready     = 0;       /* set by UART RX IRQ on Enter */
static volatile uint32_t g_last_btn_tick  = 0;       /* button debounce */

/* En pause tant que l'utilisateur n'a pas saisi de ville.
 * 1 au boot et a chaque appui B1, repasse a 0 quand la ville est validee. */
static volatile uint8_t  g_paused = 1;

static WeatherSample g_sample = {0};

static char     g_city[WEATHER_CITY_LEN] = "Inconnue";
static char     g_rx_buf[WEATHER_CITY_LEN];
static uint8_t  g_rx_idx = 0;
static uint8_t  g_rx_char = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  /* stdout en mode non-buffer : le prompt sans \n part immediatement. */
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("\r\n=== STM32 METEO STATION boot ===\r\n");
  printf("Board: NUCLEO-L152RE + X-NUCLEO-IKS01A3\r\n");
  printf("UART2 115200 8N1, TIM6 1 Hz, EXTI13 = changer de ville.\r\n");

  if (Weather_InitSensors() != 0) {
      printf("[WARN] Un ou plusieurs capteurs IKS01A3 KO.\r\n");
      HAL_GPIO_WritePin(L1_GPIO_Port, L1_Pin, GPIO_PIN_SET);
  } else {
      printf("Capteurs HTS221 + LPS22HH + STTS751 prets.\r\n");
  }

  /* Demarre TIM6 (1 Hz sampling) et l'ecoute UART en interruption. */
  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK) {
      printf("[ERR] Demarrage TIM6 echec.\r\n");
      Error_Handler();
  }
  HAL_UART_Receive_IT(&huart2, &g_rx_char, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* --- Demande de ville (au boot ou apres appui B1) --- */
      if (g_ask_city) {
          g_ask_city = 0;
          g_rx_idx   = 0;
          g_paused   = 1;            /* on suspend la telemetrie */
          printf("\r\nEntrez la ville : ");
      }

      /* --- Une ligne a ete recue sur l'UART --- */
      if (g_city_ready) {
          g_city_ready = 0;
          strncpy(g_city, g_rx_buf, sizeof(g_city) - 1);
          g_city[sizeof(g_city) - 1] = '\0';
          printf("\r\nVille selectionnee : %s\r\n", g_city);
          g_paused = 0;              /* on reprend les mesures */
          g_sample_flag = 0;         /* on jette le tick en attente eventuel */
      }

      /* --- Tick periodique TIM6 (1 Hz) --- */
      if (g_sample_flag && !g_paused) {
          g_sample_flag = 0;

          HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);   /* heartbeat */

          (void)Weather_ReadSensors(&g_sample);
          Weather_PrintUart(&huart2, g_city, &g_sample);
          Weather_UpdateLeds(&g_sample);
      } else if (g_paused) {
          /* tant qu'on attend la ville, on ignore les ticks accumules */
          g_sample_flag = 0;
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 31999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, L0_Pin|L1_Pin|L2_Pin|L3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : L0_Pin L1_Pin L2_Pin L3_Pin */
  GPIO_InitStruct.Pin = L0_Pin|L1_Pin|L2_Pin|L3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* printf retargeting -> USART2 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

/* TIM6 period elapsed -- sampling tick. ISR reste minuscule. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        g_sample_flag = 1;
    }
}

/* EXTI13 -- Bouton bleu B1. Anti-rebond logiciel puis demande de ville. */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin) {
        uint32_t now = HAL_GetTick();
        if ((now - g_last_btn_tick) >= BUTTON_DEBOUNCE_MS) {
            g_last_btn_tick = now;
            g_ask_city      = 1;
            g_rx_idx        = 0;  /* reset le buffer en cours */
        }
    }
}

/* Ecriture directe sur le registre TX, sans passer par la HAL.
 * Evite tout conflit de state machine avec HAL_UART_Receive_IT. */
static inline void uart_tx_byte(uint8_t b)
{
    while ((USART2->SR & USART_SR_TXE) == 0u) { }
    USART2->DR = b;
}

/* UART RX -- un caractere a la fois.
 * CR ou LF -> ligne terminee.
 * Autres caracteres imprimables -> ajout + echo. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        uint8_t c = g_rx_char;

        if (c == '\r' || c == '\n') {
            uart_tx_byte('\r');
            uart_tx_byte('\n');
            g_rx_buf[g_rx_idx] = '\0';
            if (g_rx_idx > 0) {
                g_city_ready = 1;
            }
        } else if (c >= 0x20 && c < 0x7F && g_rx_idx < (sizeof(g_rx_buf) - 1)) {
            g_rx_buf[g_rx_idx++] = (char)c;
            uart_tx_byte(c);
        }

        /* re-armer la reception */
        HAL_UART_Receive_IT(&huart2, &g_rx_char, 1);
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1) {
      /* lock up; pulse L1 if we ever recover */
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  (void)file; (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
