/* main.c - minimal example using HAL, CubeMX generated init functions assumed. */
#include "main.h"
#include "mfrc522.h"
#include <stdio.h>
#include"lcd.h"
#include"keypad.h"
#include"delay.h"
#include"gsm.h"

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();

    MFRC522_Init();

    char msg[80];
    uint8_t uid[5];
    uint8_t uidLen;
    uint8_t version = MFRC522_ReadRegister(0x37);
    printf("Version Reg: 0x%02X\r\n", version);

    HAL_Delay(200);

    while (1)
    {
        // simple card check
        if (MFRC522_IsCardPresent())
        {
            bool ok = MFRC522_ReadUID(uid, &uidLen);
            if (ok)
            {
                int len = snprintf(msg, sizeof(msg), "UID: %02X %02X %02X %02X\r\n",
                                   uid[0], uid[1], uid[2], uid[3]);
                HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);
                HAL_Delay(600); // simple debounce
            }
        }
        HAL_Delay(150);
    }
}

/* ---- Below are minimal HAL init functions - your CubeMX will generate these --- */
/* Replace them with the functions / content CubeMX created for your board */

void SystemClock_Config(void)
{
    /* CubeMX generated clock setup goes here */
}

static void MX_SPI1_Init(void)
{
    /* Example SPI init: adjust per CubeMX for your clock & pins */
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // tune if needed
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) { Error_Handler(); }
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // CS pin
    GPIO_InitStruct.Pin = MFRC522_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(MFRC522_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(MFRC522_CS_PORT, MFRC522_CS_PIN, GPIO_PIN_SET);

    // RST pin
    GPIO_InitStruct.Pin = MFRC522_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(MFRC522_RST_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(MFRC522_RST_PORT, MFRC522_RST_PIN, GPIO_PIN_SET);

    // SPI1 pins (PA5/PA6/PA7) should be configured by CubeMX for AF
}

/* Error handler */
void Error_Handler(void)
{
    while (1) { HAL_Delay(500); }
}
