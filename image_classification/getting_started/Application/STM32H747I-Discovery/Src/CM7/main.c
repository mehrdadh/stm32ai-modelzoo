/**
 ******************************************************************************
 * @file    main.c
 * @author  MCD Application Team
 * @brief   This is the main program for the application running on Cortex-M7
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_display.h"
#include "app_camera.h"
#include "app_network.h"

/* Global variables ----------------------------------------------------------*/

/*Application context*/
AppConfig_TypeDef App_Config;

/*Table of classes for the NN model*/
CLASSES_TABLE;

/****************************************************************************/
/***********************Application Buffers definition***********************/
/****************************************************************************/

/***Buffer to store the camera captured frame***/
#if defined ( __ICCARM__ )
 #pragma location="CapturedImage_Buffer"
 #pragma data_alignment=32
#elif defined ( __CC_ARM )
 __attribute__((section(".CapturedImage_Buffer"), zero_init))
 __attribute__ ((aligned (32)))
#elif defined ( __GNUC__ )
 __attribute__((section(".CapturedImage_Buffer")))
 __attribute__ ((aligned (32)))
#else
 #error Unknown compiler
#endif
uint8_t CapturedImage_Buffer[CAM_FRAME_BUFFER_SIZE + 32 - (CAM_FRAME_BUFFER_SIZE%32)];

/***Buffer to store the rescaled frame***/
#if defined ( __ICCARM__ )
 #pragma location="RescaledImage_Buffer"
 #pragma data_alignment=32
#elif defined ( __CC_ARM )
 __attribute__((section(".RescaledImage_Buffer"), zero_init))
 __attribute__ ((aligned (32)))
#elif defined ( __GNUC__ )
 __attribute__((section(".RescaledImage_Buffer")))
 __attribute__ ((aligned (32)))
#else
 #error Unknown compiler
#endif
uint8_t RescaledImage_Buffer[RESCALED_FRAME_BUFFER_SIZE + 32 - (RESCALED_FRAME_BUFFER_SIZE%32)];

 /***Buffer to store the NN input frame***/
 #if defined ( __ICCARM__ )
  #pragma location="NN_InputImage_Buffer"
  #pragma data_alignment=32
 #elif defined ( __CC_ARM )
  __attribute__((section(".NN_InputImage_Buffer"), zero_init))
  __attribute__ ((aligned (32)))
 #elif defined ( __GNUC__ )
  __attribute__((section(".NN_InputImage_Buffer")))
  __attribute__ ((aligned (32)))
 #else
  #error Unknown compiler
 #endif
#ifdef AI_NETWORK_INPUTS_IN_ACTIVATIONS
 uint8_t *NN_InputImage_Buffer=NULL;
#else 
 uint8_t NN_InputImage_Buffer[AI_INPUT_BUFFER_SIZE + 32 - (AI_INPUT_BUFFER_SIZE%32)];
#endif 
 
 /***Buffer to store the NN ouput data***/
 #if defined ( __ICCARM__ )
  #pragma location="NN_OutputData_Buffer"
  #pragma data_alignment=32
 #elif defined ( __CC_ARM )
  __attribute__((section(".NN_OutputData_Buffer"), zero_init))
  __attribute__ ((aligned (32)))
 #elif defined ( __GNUC__ )
  __attribute__((section(".NN_OutputData_Buffer")))
  __attribute__ ((aligned (32)))
 #else
  #error Unknown compiler
 #endif
#ifdef AI_NETWORK_OUTPUTS_IN_ACTIVATIONS
 uint8_t *NN_OutputData_Buffer=NULL;
#else 
 uint8_t NN_OutputData_Buffer[AI_OUTPUT_BUFFER_SIZE + 32 - (AI_OUTPUT_BUFFER_SIZE%32)]= {0};
#endif 
  
 /***Buffer to store the NN Activation data***/
 #if defined ( __ICCARM__ )
  #pragma location="NN_Activation_Buffer"
  #pragma data_alignment=32
 #elif defined ( __CC_ARM )
  __attribute__((section(".NN_Activation_Buffer"), zero_init))
  __attribute__ ((aligned (32)))
 #elif defined ( __GNUC__ )
  __attribute__((section(".NN_Activation_Buffer")))
  __attribute__ ((aligned (32)))
 #else
  #error Unknown compiler
 #endif
 uint8_t NN_Activation_Buffer[AI_ACTIVATION_BUFFER_SIZE + 32 - (AI_ACTIVATION_BUFFER_SIZE%32)];
 

/*
 * LCD display buffers in external SDRAM
 * When double framebuffer technique is used, it is recommended to have these
 * buffers in two separate banks.
 * AN4861: 4.5.3 - Optimizing the LTDC framebuffer fetching from SDRAM.
 */
#if defined(__ICCARM__)
#pragma location = "Lcd_Display"
#pragma data_alignment=32
#elif defined(__CC_ARM)
__attribute__((section(".Lcd_Display"), zero_init))
__attribute__ ((aligned (32)))
#elif defined(__GNUC__)
__attribute__((section(".Lcd_Display")))
__attribute__ ((aligned (32)))
#else
#error Unknown compiler
#endif
uint8_t lcd_display_global_memory[SDRAM_BANK_SIZE + LCD_FRAME_BUFFER_SIZE];


uint8_t pixel_conv_lut[256];


/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
static void MPU_Config(void);
static void Software_Init(AppConfig_TypeDef *);
static void Hardware_Init(AppConfig_TypeDef *);

/**
* @brief  Application entry point
* @param  None
* @retval None
*/
int main(void)
{
  /* System Init, System clock, voltage scaling and L1-Cache configuration are done by CPU1 (Cortex-M7)
  in the meantime Domain D2 is put in STOP mode(Cortex-M4 in deep-sleep) */
  
  /* Configure the MPU attributes */
  MPU_Config();
  
  /* Enable the CPU Cache */
  CPU_CACHE_Enable();
  
  /* Initialize the HAL Library */
  HAL_Init();
  
  /* Configure the system clock to 400 MHz */
  SystemClock_Config();
  
  /* Enable CRC HW IP block */
  __HAL_RCC_CRC_CLK_ENABLE();
  
  /* Perfom SW configuration related to the application  */
  Software_Init(&App_Config);
  
  /* Perfom HW configuration (display, camera) related to the application  */
  Hardware_Init(&App_Config);
  
  /* Initialize the Neural Network library  */
  Network_Init(&App_Config);
  
  /* Display welcome message */
  Display_WelcomeScreen(&App_Config);
  
  while(1)
  {
    /**************************************************************************/
    /**************Wait for the next frame to be ready for processing**********/
    /**************************************************************************/ 
    Camera_GetNextReadyFrame(&App_Config);
    
    /**************************************************************************/
    /********************Dispaly camera frame into LCD display*****************/
    /**************************************************************************/ 
    Display_CameraPreview(&App_Config);
    
    /**************************************************************************/
    /**************************Run Frame Preprocessing*************************/
    /**************************************************************************/  
    Network_Preprocess(&App_Config);
    
    /**************************************************************************/
    /***Launch camera capture of next frame in // of current frame inference***/
    /**************************************************************************/ 
    Camera_StartNewFrameAcquisition(&App_Config);
    
    /**************************************************************************/
    /****************************Run NN Inference******************************/
    /**************************************************************************/
    Network_Inference(&App_Config);
    
    /**************************************************************************/
    /*************************Run post process operations**********************/
    /**************************************************************************/    
    Network_Postprocess(&App_Config);  
    
    /**************************************************************************/
    /*****************Display Inference output results and FPS*****************/
    /**************************************************************************/ 
    Display_NetworkOutput(&App_Config);
  }
  
  /* End of program */
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief Initializes the application context structure
 * @param App_Config_Ptr pointer to application context
 */
static void Software_Init(AppConfig_TypeDef *App_Config_Ptr)
{
  App_Config_Ptr->mirror_flip=CAMERA_MIRRORFLIP_FLIP;
  App_Config_Ptr->new_frame_ready=0;
 
  App_Config_Ptr->lcd_sync=0;
  
  App_Config_Ptr->lut=pixel_conv_lut;
  
  App_Config_Ptr->nn_input_type= QUANT_INPUT_TYPE;
  App_Config_Ptr->nn_output_type= QUANT_OUTPUT_TYPE;
  
  /*{scale,zero-point} related to the NN model training*/
  App_Config_Ptr->nn_input_norm_scale=PP_SCALE;
  App_Config_Ptr->nn_input_norm_zp=PP_OFFSET;
  
  App_Config_Ptr->nn_output_labels=classes_table;
  
  /*Preproc*/
#if PP_COLOR_MODE == RGB_FORMAT
  App_Config_Ptr->red_blue_swap = 1; /* See UM2611 section 3.2.6 Pixel data order */
#else
  App_Config_Ptr->red_blue_swap = 0;
#endif
  
#if PP_COLOR_MODE == GRAYSCALE_FORMAT
  App_Config_Ptr->PixelFormatConv=SW_PFC;
#else
  App_Config_Ptr->PixelFormatConv=HW_PFC;
#endif
  
  /*Memory buffer init*/
  App_Config_Ptr->nn_input_buffer = NN_InputImage_Buffer; 
  App_Config_Ptr->nn_output_buffer=NN_OutputData_Buffer;
  App_Config_Ptr->camera_capture_buffer = CapturedImage_Buffer;
  App_Config_Ptr->rescaled_image_buffer = RescaledImage_Buffer;
  App_Config_Ptr->activation_buffer = NN_Activation_Buffer;
  App_Config_Ptr->lcd_frame_read_buff=lcd_display_global_memory;
  App_Config_Ptr->lcd_frame_write_buff=lcd_display_global_memory + SDRAM_BANK_SIZE;
}

/**
 * @brief Initializes the WH peripherals
 * @param App_Config_Ptr pointer to application context
 */
static void Hardware_Init(AppConfig_TypeDef *App_Config_Ptr)
{
  /*LEDs Init*/
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_ORANGE);
  BSP_LED_Init(LED_RED);
  BSP_LED_Init(LED_BLUE);
  
  /*Display init*/
  Display_Init(App_Config_Ptr);
  
  /*Camera init*/
  Camera_Init(App_Config_Ptr); 
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 400000000 (Cortex-M7 CPU Clock)
 *            HCLK(Hz)                       = 200000000 (Cortex-M4 CPU, Bus matrix Clocks)
 *            AHB Prescaler                  = 2
 *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
 *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
 *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
 *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
 *            HSE Frequency(Hz)              = 25000000
 *            PLL_M                          = 5
 *            PLL_N                          = 160
 *            PLL_P                          = 2
 *            PLL_Q                          = 4
 *            PLL_R                          = 2
 *            VDD(V)                         = 3.3
 *            Flash Latency(WS)              = 4
 * @param  None
 * @retval None
 */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /* The voltage scaling allows optimizing the power consumption when the device is
  clocked below the maximum system frequency, to update the voltage scaling value
  regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
  {
  }

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if (ret != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
  if (ret != HAL_OK)
  {
    Error_Handler();
  }

  /*
  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
  (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
  The I/O Compensation Cell activation  procedure requires :
  - The activation of the CSI clock
  - The activation of the SYSCFG clock
  - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR
  */

  /*activate CSI clock mandatory for I/O Compensation Cell*/
  __HAL_RCC_CSI_ENABLE();

  /* Enable SYSCFG clock mandatory for I/O Compensation Cell */
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* Enables the I/O Compensation Cell */
  HAL_EnableCompensationCell();
}

/**
 * @brief  CPU L1-Cache enable.
 * @param  None
 * @retval None
 */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
 * @brief  Configure the MPU attributes for the device's memories.
 * @param  None
 * @retval None
 */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

#if EXT_SDRAM_CACHE_ENABLED == 0
  /*
  To make the memory region non cacheable and avoid any cache coherency maintenance:
  - either: MPU_ACCESS_NOT_BUFFERABLE + MPU_ACCESS_NOT_CACHEABLE
  - or: MPU_ACCESS_SHAREABLE => the S field is equivalent to non-cacheable memory
  */
  /* External SDRAM memory: LCD Frame buffer => non-cacheable */
  /*TEX=001, C=0, B=0*/
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xD0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
#elif EXT_SDRAM_CACHE_ENABLED == 1
  /* External SDRAM memory: all as WBWA */
  /*TEX=001, C=1, B=1*/
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xD0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1; 
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE; 
#elif EXT_SDRAM_CACHE_ENABLED == 2
  /*External SDRAM memory: all as Write Thru:*/
  /*TEX=000, C=1, B=0*/
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xD0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0; 
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
#else
#error Please check definition of EXT_SDRAM_CACHE_ENABLED define
#endif

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /*Internal SRAM memory: cache policies are WBWA (Write Back and Write Allocate) by default */

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}


/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void Error_Handler(void)
{
  BSP_LED_Off(LED_GREEN);
  BSP_LED_Off(LED_ORANGE);
  BSP_LED_Off(LED_RED);
  BSP_LED_Off(LED_BLUE);

  /* Turn LED RED on */
  BSP_LED_On(LED_RED);
  while (1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

