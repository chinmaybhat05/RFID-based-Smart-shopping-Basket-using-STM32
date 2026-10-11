#ifndef MFRC522_H
#define MFRC522_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* HW-specific: change these if your pins are different */
#define MFRC522_CS_PORT    GPIOA
#define MFRC522_CS_PIN     GPIO_PIN_4
#define MFRC522_RST_PORT   GPIOB
#define MFRC522_RST_PIN    GPIO_PIN_0

/* extern handles from main */
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;

/* MFRC522 commands and constants */
#define PCD_IDLE              0x00
#define PCD_TRANSCEIVE        0x0C
#define PCD_AUTHENT           0x0E
#define PCD_SOFTRESET         0x0F

/* MFRC522 registers (only those needed) */
#define CommandReg            0x01
#define CommIEnReg            0x02
#define DivIEnReg             0x03
#define CommIrqReg            0x04
#define DivIrqReg             0x05
#define ErrorReg              0x06
#define FIFODataReg           0x09
#define FIFOLevelReg          0x0A
#define ControlReg            0x0C
#define BitFramingReg         0x0D
#define ModeReg               0x11
#define TxModeReg             0x12
#define RxModeReg             0x13
#define TxControlReg          0x14
#define CRCResultRegL         0x22
#define CRCResultRegM         0x21
#define TModeReg              0x2A
#define TPrescalerReg         0x2B
#define TReloadRegL           0x2D
#define TReloadRegH           0x2C
#define CollReg               0x0E
#define VersionReg            0x37

/* PICC commands */
#define PICC_REQIDL           0x26
#define PICC_ANTICOLL         0x93

/* API */
void MFRC522_Init(void);
bool MFRC522_IsCardPresent(void);
bool MFRC522_ReadUID(uint8_t *uid, uint8_t *uidLen);
void MFRC522_Reset(void);
uint8_t MFRC522_ReadRegister(uint8_t reg);


#endif /* MFRC522_H */
