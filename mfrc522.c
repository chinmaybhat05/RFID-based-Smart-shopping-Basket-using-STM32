#include "mfrc522.h"
#include <string.h>

/* External hardware handles (define in main.c) */
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;

/* Helper macros for CS and RST pins */
static inline void MFRC522_CS_LOW(void)  { HAL_GPIO_WritePin(MFRC522_CS_PORT, MFRC522_CS_PIN, GPIO_PIN_RESET); }
static inline void MFRC522_CS_HIGH(void) { HAL_GPIO_WritePin(MFRC522_CS_PORT, MFRC522_CS_PIN, GPIO_PIN_SET); }
static inline void MFRC522_RST_LOW(void) { HAL_GPIO_WritePin(MFRC522_RST_PORT, MFRC522_RST_PIN, GPIO_PIN_RESET); }
static inline void MFRC522_RST_HIGH(void){ HAL_GPIO_WritePin(MFRC522_RST_PORT, MFRC522_RST_PIN, GPIO_PIN_SET); }

/* Low-level SPI read/write of MFRC522 register */
static void MFRC522_WriteRegister(uint8_t reg, uint8_t val)
{
    uint8_t buf[2];
    buf[0] = (reg << 1) & 0x7E; // address format: 0XXXXXX0
    buf[1] = val;
    MFRC522_CS_LOW();
    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    MFRC522_CS_HIGH();
}

 uint8_t MFRC522_ReadRegister(uint8_t reg)
{
    uint8_t addr = ((reg << 1) & 0x7E) | 0x80; // read: set MSB
    uint8_t val = 0;
    MFRC522_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &val, 1, HAL_MAX_DELAY);
    MFRC522_CS_HIGH();
    return val;
}

/* write multiple bytes to FIFO and execute command */
static void MFRC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, tmp | mask);
}
static void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, tmp & (~mask));
}

/* CRC calculation using MFRC522's internal CRC coprocessor */
static void MFRC522_CalculateCRC(uint8_t *data, uint8_t len, uint8_t *result)
{
    MFRC522_ClearBitMask(DivIrqReg, 0x04);      // Clear CRCIRq
    MFRC522_SetBitMask(FIFOLevelReg, 0x80);     // Flush FIFO
    for (uint8_t i = 0; i < len; i++)
        MFRC522_WriteRegister(FIFODataReg, data[i]);
    MFRC522_WriteRegister(CommandReg, PCD_TRANSCEIVE);
    MFRC522_SetBitMask(BitFramingReg, 0x80);    // StartSend

    // Wait for CRC calculation to complete
    uint32_t i = 0xFFFF;
    uint8_t n;
    do {
        n = MFRC522_ReadRegister(DivIrqReg);
        if (--i == 0) { result[0] = result[1] = 0; return; }
    } while (!(n & 0x04));

    result[0] = MFRC522_ReadRegister(CRCResultRegL);
    result[1] = MFRC522_ReadRegister(CRCResultRegM);
}

/* send command to card (simplified) */
static uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                              uint8_t *backData, uint16_t *backLen)
{
    uint8_t status = 0;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    if (command == PCD_AUTHENT) {
        irqEn = 0x12;
        waitIRq = 0x10;
    } else if (command == PCD_TRANSCEIVE) {
        irqEn = 0x77;
        waitIRq = 0x30;
    }
    MFRC522_WriteRegister(CommIEnReg, irqEn | 0x80); // enable interrupts
    MFRC522_ClearBitMask(CommIrqReg, 0x80);
    MFRC522_SetBitMask(FIFOLevelReg, 0x80); // flush FIFO
    MFRC522_WriteRegister(CommandReg, PCD_IDLE);

    // write data to FIFO
    for (uint8_t i = 0; i < sendLen; i++) {
        MFRC522_WriteRegister(FIFODataReg, sendData[i]);
    }
    MFRC522_WriteRegister(CommandReg, command);
    if (command == PCD_TRANSCEIVE) {
        MFRC522_SetBitMask(BitFramingReg, 0x80); // StartSend
    }

    // wait for completion
    uint32_t i = 20000;
    uint8_t n;
    do {
        n = MFRC522_ReadRegister(CommIrqReg);
        if (--i == 0) break;
    } while (!(n & 0x01) && !(n & waitIRq));

    MFRC522_ClearBitMask(BitFramingReg, 0x80);

    uint8_t errorRegValue = MFRC522_ReadRegister(ErrorReg);
    if (!(errorRegValue & 0x1B)) {
        status = 1;
        if (n & irqEn & 0x01) status = 0;
        if (command == PCD_TRANSCEIVE) {
            uint8_t fifoLevel = MFRC522_ReadRegister(FIFOLevelReg);
            uint8_t lastBits = MFRC522_ReadRegister(ControlReg) & 0x07;
            *backLen = fifoLevel;
            if (lastBits) *backLen = (fifoLevel - 1) * 8 + lastBits;
            else *backLen = fifoLevel * 8;
            // read the FIFO
            for (uint16_t i = 0; i < fifoLevel; i++) {
                backData[i] = MFRC522_ReadRegister(FIFODataReg);
            }
        }
    } else {
        status = 0;
    }
    return status;
}

/* Basic initialization sequence */
void MFRC522_Init(void)
{
    // Hardware reset
    MFRC522_RST_HIGH();
    HAL_Delay(50);
    MFRC522_RST_LOW();
    HAL_Delay(50);
    MFRC522_RST_HIGH();
    HAL_Delay(50);

    MFRC522_WriteRegister(TModeReg, 0x8D);
    MFRC522_WriteRegister(TPrescalerReg, 0x3E);
    MFRC522_WriteRegister(TReloadRegL, 30);
    MFRC522_WriteRegister(TReloadRegH, 0);

    MFRC522_WriteRegister(TxModeReg, 0x00);
    MFRC522_WriteRegister(RxModeReg, 0x00);
    MFRC522_WriteRegister(ModeReg, 0x3D);
    MFRC522_WriteRegister(TxControlReg, 0x83); // turn on antenna (2x)
}

/* Reset */
void MFRC522_Reset(void)
{
    MFRC522_WriteRegister(CommandReg, PCD_SOFTRESET);
    HAL_Delay(50);
}

/* Check if a card is present (REQA) */
bool MFRC522_IsCardPresent(void)
{
    uint8_t requestMode = PICC_REQIDL;
    uint8_t backBits;
    uint8_t buffer[16];
    uint16_t backLen = 0;

    uint8_t status = MFRC522_ToCard(PCD_TRANSCEIVE, &requestMode, 1, buffer, &backLen);
    return (status && backLen == 0x10) ? true : (status != 0);
}

/* Anti-collision and read UID (returns true if UID read) */
bool MFRC522_ReadUID(uint8_t *uid, uint8_t *uidLen)
{
    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    uint8_t serNum[5];
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;

    uint16_t backLen = 0;
    uint8_t status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, buffer, &backLen);
    if (!status) return false;

    // If we have at least 5 bytes in FIFO read
    // buffer[0..4] -> UID (4 bytes) + BCC
    // We'll copy up to 5 bytes returned (but usually 5)
    uint8_t i;
    for (i = 0; i < 5 && i < (backLen/8); i++) {
        uid[i] = buffer[i];
    }
    *uidLen = 4; // 4-byte UID typical
    return true;
}
