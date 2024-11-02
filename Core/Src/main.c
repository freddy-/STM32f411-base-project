/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "dma.h"
#include "fatfs.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7789.h"
#include "tjpgd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* Session identifier for input/output functions (Name, members and usage are as user defined) */
typedef struct {
    FIL *fp;               /* Input stream */
    uint8_t *fbuf;         /* Pointer to the frame buffer */
    unsigned int wfbuf;    /* Width of the frame buffer [pix] */
    unsigned int bytesRead;
} IODEV;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Bytes per pixel of image output */
#define N_BPP (3 - JD_FORMAT)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi1_tx;

FATFS fs;
FIL fil;
uint8_t buffer[100];
char buff[50];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*------------------------------*/
/* User defined input funciton  */
/*------------------------------*/

size_t in_func (    /* Returns number of bytes read (zero on error) */
    JDEC* jd,       /* Decompression object */
    uint8_t* buff,  /* Pointer to the read buffer (null to remove data) */
    size_t nbyte    /* Number of bytes to read/remove */
)
{
    IODEV *dev = (IODEV*)jd->device;   /* Session identifier (5th argument of jd_prepare function) */

    unsigned int bytesRead = 0;
    if (buff) { /* Read data from input stream */
        FRESULT res = f_read(dev->fp, buff, nbyte, &bytesRead);
        if (res != FR_OK) {
          ST7789_WriteString(10, 20, "f_read nok", Font_11x18, RED, BLACK);
        }
        // we must keep track of how many bytes have been read in total
        dev->bytesRead += bytesRead;
        return bytesRead;
    } else {    /* Remove data from input stream */
        // calculate the amount of bytes to seek
        dev->bytesRead += nbyte;
        return f_lseek(dev->fp, dev->bytesRead) ? 0 : nbyte;
    }
}


/*------------------------------*/
/* User defined output funciton */
/*------------------------------*/

int out_func (      /* Returns 1 to continue, 0 to abort */
    JDEC* jd,       /* Decompression object */
    void* bitmap,   /* Bitmap data to be output */
    JRECT* rect     /* Rectangle region of output image */
)
{
    IODEV *dev = (IODEV*)jd->device;   /* Session identifier (5th argument of jd_prepare function) */
    //uint8_t *src, *dst;
    //uint16_t y, bws;
    //unsigned int bwd;


    /* Progress indicator */
    if (rect->left == 0) {
        //printf("\r%lu%%", (rect->top << jd->scale) * 100UL / jd->height);
    }

    /* Copy the output image rectangle to the frame buffer */
    /* this is is memory intensive! */
    //src = (uint8_t*)bitmap;                           /* Output bitmap */
    //dst = dev->fbuf + N_BPP * (rect->top * dev->wfbuf + rect->left);  /* Left-top of rectangle in the frame buffer */
    //bws = N_BPP * (rect->right - rect->left + 1);     /* Width of the rectangle [byte] */
    //bwd = N_BPP * dev->wfbuf;                         /* Width of the frame buffer [byte] */
    //for (y = rect->top; y <= rect->bottom; y++) {
    //    memcpy(dst, src, bws);   /* Copy a line */
    //    src += bws; dst += bwd;  /* Next line */
    //}

    // the image start position in the screen
    uint8_t xPos = 0;
    uint8_t yPos = 0;

    uint16_t  x = rect->left + xPos;
    uint16_t  y = rect->top  + yPos;
    uint16_t w = rect->right  + 1 - rect->left;
    uint16_t h = rect->bottom + 1 - rect->top;

    ST7789_DrawImage(x, y, w, h, (uint16_t*)bitmap);

    return 1;    /* Continue to decompress */
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	SCB->VTOR = FLASH_BASE | 0x4000; // 添加至工程开头时钟初始化之后
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable DWT
	DWT->CYCCNT = 0;                                // Clear counter
	DWT->CTRL = DWT_CTRL_CYCCNTENA_Msk;             // Enable counter
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_SPI3_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
	ST7789_Init();
	HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);

	ST7789_Fill_Color(BLACK);
	ST7789_WriteString(10, 0, "DISPLAY OK", Font_11x18, GREEN, BLACK);

	FRESULT res = f_mount(&fs, "", 1);
	if (res != FR_OK) {
		sprintf(buff, "ERROR MOUNTING: %d", (int) res);
		ST7789_WriteString(10, 20, buff, Font_11x18, GREEN, BLACK);
	} else {
		//ST7789_WriteString(10, 20, "MOUNT OK", Font_11x18, GREEN, BLACK);
	}

  DIR dp;
	// list dir
	/*
	DIR dp;
	FILINFO fno;
	res = f_opendir(&dp, "/");
	if (res != FR_OK) {
    sprintf(buff, "ERROR OPEN DIR: %d", (int) res);
    ST7789_WriteString(10, 40, buff, Font_11x18, GREEN, BLACK);
	} else {
    ST7789_WriteString(10, 40, "/", Font_11x18, WHITE, BLACK);
	  for (uint8_t counter = 60;; counter += 20) {
	    res = f_readdir(&dp, &fno);                   // Read a directory item
	    if (res != FR_OK || fno.fname[0] == 0) break;  // Break on error or end of dir
	    sprintf(buff, "> %s", (int) fno.fname);
	    ST7789_WriteString(10, counter, buff, Font_7x10, WHITE, BLACK);
	  }
	  f_closedir(&dp);
	}
	*/

  //RGB
  //BRG

  // test jpeg decompressor
  JRESULT jRes;      /* Result code of TJpgDec API */
  JDEC jdec;        /* Decompression object */
  void *work;       /* Pointer to the work area */
  size_t sz_work = 3500; /* Size of work area */
  IODEV devid;      /* Session identifier */
  unsigned int bytesRead;
  devid.fp = &fil;
  devid.bytesRead = 0;

  res = f_open(devid.fp, "img6.jpg", FA_READ);
  if (res != FR_OK) {
    sprintf(buff, "open file nok: %d", (int) res);
    ST7789_WriteString(10, 0, buff, Font_11x18, GREEN, BLACK);
    return;
  }
  /*res = f_read(devid.fp, buffer, sizeof(buffer), &bytesRead);
  if (res != FR_OK) {
    sprintf(buff, "read file nok: %d", (int) res);
    ST7789_WriteString(10, 0, buff, Font_11x18, GREEN, BLACK);
  }*/

  work = (void*)malloc(sz_work);
  jRes = jd_prepare(&jdec, in_func, work, sz_work, &devid);
  if (jRes == JDR_OK) {
      /* It is ready to dcompress and image info is available here */
      //printf("Image size is %u x %u.\n%u bytes of work ares is used.\n", jdec.width, jdec.height, sz_work - jdec.sz_pool);

      sprintf(buff, "size %u x %u", jdec.width, jdec.height);
      ST7789_WriteString(0, 0, buff, Font_11x18, GREEN, BLACK);

      /* Initialize output device (Create a frame buffer) */
      //devid.fbuf = (uint8_t*)malloc(N_BPP * jdec.width * jdec.height);
      //devid.wfbuf = jdec.width;

      jRes = jd_decomp(&jdec, out_func, 0);   /* Start to decompress with 1/1 scaling */
      if (jRes == JDR_OK) {
          /* Decompression succeeded. You have the decompressed image in the frame buffer here. */
          ST7789_WriteString(10, 20, "Decompressed!", Font_11x18, GREEN, BLACK);

          //ST7789_DrawImage(0, 0, jdec.width, jdec.height, devid.fbuf);

      } else {
          //printf("jd_decomp() failed (rc=%d)\n", res);
          sprintf(buff, "Decompression failed %d", jRes);
          ST7789_WriteString(10, 40, buff, Font_11x18, GREEN, BLACK);
      }

      //free(devid.fbuf);    /* Discard frame buffer */

  } else {
      //printf("jd_prepare() failed (rc=%d)\n", res);
      sprintf(buff, "jd_prepare %d", jRes);
      ST7789_WriteString(10, 40, buff, Font_11x18, RED, BLACK);
  }

  free(work);             /* Discard work area */
  f_close(devid.fp);       /* Close the JPEG file */
  // test jpeg decompressor end




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		// printar um ok na tela, usar o display como debug do cartão sd!
		// Enter the HID Bootloader
		if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) {
			NVIC_SystemReset();
		}
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
  ST7789_WriteString(10, 40, "ERROR", Font_11x18, RED, BLACK);
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
