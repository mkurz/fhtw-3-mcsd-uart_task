/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RNG_HandleTypeDef hrng;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
bool done = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_RNG_Init(void);
/* USER CODE BEGIN PFP */
void uart_send_string(char*);
void send_help_msg();
bool allowed_chars(uint8_t);

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
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  uart_send_string("\r\n Welcome!");
  send_help_msg();

  bool print_prompt = true;
  bool wait_for_client_ack = false;
  uint8_t receiveData[1];
  int chars_entered = 0;
  char* chars = malloc(0 * sizeof(*chars)); // dynamically growing buffer for chars entered
  while (1)
  {
    if(print_prompt) {
      uart_send_string("\r\n ~ ");
      print_prompt = false;

      // We are on a new line -> reset the char buffer
      chars_entered = 0;
      chars = realloc(chars, 0 * sizeof(*chars));
    }

    if (HAL_UART_Receive_IT(&huart2, receiveData, 1) == HAL_ERROR) {
      Error_Handler();
    }
    // Wait for data to receive
    while(done == false)
      ;
    // Reset the transmit status
    done = false;

    // Check if the character entered is allowed
    // We don't support every key on the keyboard because some are tricky to handle, e.g. arrow keys are actually a combination of 3 ASCII chars:
    // For example the left arrow key gets send as 27, 91, 68 (ESC, '[', D)
    // I was able to debug that, but also see https://easysavecode.com/ryqtF39y
    // Even more: https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html#user-input
    if(!allowed_chars(receiveData[0])) {
      continue;
    }

    if(receiveData[0] == '\n' || receiveData[0] == '\r') {
      // line feed, carriage return was send (or enter was pressed), time to see what was entered in this line and how to react on it
      if (chars_entered > 0) {
        uart_send_string("\r\n < "); // show that we received data ;)
        uart_send_string(chars);     // also echo the data received

        int lower_bound, upper_bound, check;
        check = sscanf(chars, "#r,%d:%d", &lower_bound, &upper_bound);
        if(check == 2 && lower_bound < upper_bound && lower_bound >= 0) {
          uart_send_string("\r\n > ACK");

          // Generate random number within lower and upper bound
          uint32_t random_number = 0;
          HAL_RNG_GenerateRandomNumber(&hrng, &random_number);
          int num = (random_number % (upper_bound - lower_bound + 1)) + lower_bound;

          char buffer [19]; // 19 is enough to fit INT32_MAX: "\r\n > #a,2147483647\0"
          sprintf(buffer, "\r\n > #a,%d", num);
          uart_send_string(buffer);
          // Now the client can send back ACK as well and the mcu will accept it,
          // instead of replying NACK (which it would normally do, because ACK is handled like any other string usually)
          wait_for_client_ack = true;
        } else if (strcmp(chars, "help") == 0 || strcmp(chars, "?") == 0) {
          send_help_msg();
          wait_for_client_ack = false;
        } else {
          if (strcmp(chars, "ACK") != 0 || !wait_for_client_ack) {
            uart_send_string("\r\n > NACK");
          }
          wait_for_client_ack = false;
        }
      }
      print_prompt = true; // Let's move the prompt to a new line
    } else {
      bool echo_input = false;
      if(receiveData[0] != 127) {
        // Backspace was NOT pressed
        // We need to resize the buffer array so the entered char can be appended to it
        chars = realloc(chars, ++chars_entered * sizeof(*chars) + 1); // "plus one" for '\0' (end of string)
        chars[chars_entered - sizeof(*chars)] = receiveData[0];
        chars[chars_entered] = '\0'; // \0 -> end of string
        echo_input = true;
      } else if(chars_entered > 0) {
        // Backspace was pressed and characters exist
        // We need to resize the buffer array so the removed char is not contained anymore
        // FYI: free() isn't needed, realloc() takes care of freeing memory
        chars = realloc(chars, --chars_entered * sizeof(*chars)); // remove the last character
        echo_input = true;
      }

      if (echo_input) {
        uart_send_string((char[]){receiveData[0], '\0'});
      }
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  // set transmit status to done
  done = true;
}

/**
  * @brief  Helper method to send a help info message via UART
  * @retval None
  */
void send_help_msg() {
  uart_send_string("\r\n To generate a random value enter following command:\r\n");
  uart_send_string(" #r,<lower_bound>:<upper_bound>\\n\r\n");
  uart_send_string(" lower_bound and upper_bound have to be unsigned integers (>= 0).\r\n");
  uart_send_string(" lower_bound has to be smaller than upper_bound!\r\n");
  uart_send_string(" Instead of \\n you can also use \\r. With PuTTY\r\n");
  // See https://z49x2vmq.github.io/2017/11/12/putty-cr-lf-en/
  uart_send_string(" * to send \\r (carriage return) hit enter or ctrl+m\r\n * to send \\n (line feed) use ctrl+j\r\n");
  uart_send_string(" Special keys (like arrow keys, escape, f-keys, etc.) are NOT supported!\r\n");
  uart_send_string(" To display this message again enter 'help' or '?'");
}

/**
  * @brief  Helper method to send a given string (char array) via UART
  * @param  msg: The message string to send
  * @retval None
  */
void uart_send_string(char* msg) {
  if (HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 1000) == HAL_ERROR) {
    Error_Handler();
  }
}

/**
  * @brief  Checks if a given character is allowed to be processed further
  * @param  character: The character to check
  * @retval If the character is allowed (true) or not (false)
  */
bool allowed_chars(uint8_t character) {
  if(32 <= character && character <= 126) {
    // ASCII 32 to 126 are printable characters, so they are OK:
    // https://en.wikipedia.org/wiki/ASCII#Printable_characters
    return true;
  }

  // Other allowed characters besides the one which are printable
  uint8_t allowed[] = {
    127, // backspace
    13,  // \r (carriage return)
    10   // \n (line feed)
  };
  size_t n = sizeof(allowed) / sizeof(uint8_t);
  for(int i = 0; i < n; i++) {
    if (allowed[i] == character) {
      return true;
    }
  }
  return false;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
