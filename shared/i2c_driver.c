/* I2C Driver */

#include <math.h>
#include <stdint.h>

#include "../apdet_hanna/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_i2c.h"
#include "./general.h"

/* Constants -------------------------------------------------- */
#define I2C_SCL_CLOCK       100000
#define I2C_READ_TIMEOUT    100
#define I2C_WRITE_TIMEOUT   100

#define BAROMETER_ADDRESS       0x76
#define ACCELEROMETER_ADDRESS   0x18

#define BAROMETER_CMD_RESET     0x1E
#define BAROMETER_CMD_PROM_READ 0xA0
#define BAROMETER_CMD_ADC_READ  0x00
#define BAROMETER_CMD_ADC_CONV  0x40
#define BAROMETER_CMD_ADC_D1    0x00
#define BAROMETER_CMD_ADC_D2    0x10
#define BAROMETER_CMD_ADC_256   0x00
#define BAROMETER_CMD_ADC_512   0x02
#define BAROMETER_CMD_ADC_1024  0x04
#define BAROMETER_CMD_ADC_2048  0x06
#define BAROMETER_CMD_ADC_4096  0x08

#define ACCELEROMETER_AUTO_INCREMENT        0x80
#define ACCELEROMETER_REG_CTRL_REG1         0x20
#define ACCELEROMETER_REG_CTRL_REG2         0x21
#define ACCELEROMETER_REG_CTRL_REG3         0x22
#define ACCELEROMETER_REG_CTRL_REG4         0x23
#define ACCELEROMETER_REG_CTRL_REG5         0x24
#define ACCELEROMETER_REG_HP_FILTER_RESET   0x25
#define ACCELEROMETER_REG_REFERENCE         0x26
#define ACCELEROMETER_REG_STATUS_REG        0x27
#define ACCELEROMETER_REG_OUT_X_L           0x28
#define ACCELEROMETER_REG_OUT_X_H           0x29
#define ACCELEROMETER_REG_OUT_Y_L           0x2A
#define ACCELEROMETER_REG_OUT_Y_H           0x2B
#define ACCELEROMETER_REG_OUT_Z_L           0x2C
#define ACCELEROMETER_REG_OUT_Z_H           0x2D
#define ACCELEROMETER_REG_INT1_CFG          0x30
#define ACCELEROMETER_REG_INT1_SOURCE       0x31
#define ACCELEROMETER_REG_INT1_THS          0x32
#define ACCELEROMETER_REG_INT1_DURATION     0x33
#define ACCELEROMETER_REG_INT2_CFG          0x34
#define ACCELEROMETER_REG_INT2_SOURCE       0x35
#define ACCELEROMETER_REG_INT2_THS          0x36
#define ACCELEROMETER_REG_INT2_DURATION     0x37

/* Variables -------------------------------------------------- */
static I2C_HandleTypeDef hi2c;
static uint16_t barometer_calibration[6];

/* I2C Functions -------------------------------------------------- */

/**
  * @brief  Initializes the I2C controller
  * @retval Status
  */
extern status_t i2cInit(void)
{
    if (HAL_I2C_DeInit(&hi2c) != HAL_OK) {
        return STATUS_ERROR;
    }

    hi2c.Instance = I2C1;
    hi2c.Init.ClockSpeed = I2C_SCL_CLOCK;
    hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c.Init.OwnAddress1 = 0;
    hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c.Init.OwnAddress2 = 0;
    hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c) != HAL_OK) {
        return STATUS_ERROR;
    }

    // TODO init MSP

    return STATUS_OK;
}

/**
  * @brief  Reads data from the I2C bus
  * @param  address The address of the device to read from
  * @param  data A pointer to the buffer to write the data to
  * @param  size The number of bytes to read
  * @retval Status
  */
extern status_t i2cRead(uint16_t address, uint8_t *data, uint16_t size)
{
    if (HAL_I2C_Master_Receive(&hi2c, address, data, size, I2C_READ_TIMEOUT) != HAL_OK) {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/**
  * @brief  Writes data to the I2C bus
  * @param  address The address of the device to write to
  * @param  data A pointer to the buffer to read the data from
  * @param  size The number of bytes to write
  * @retval Status
  */
extern status_t i2cWrite(uint16_t address, uint8_t *data, uint16_t size)
{
    if (HAL_I2C_Master_Transmit(&hi2c, address, data, size, I2C_WRITE_TIMEOUT) != HAL_OK) {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/* Barometer Functions -------------------------------------------------- */

/**
  * @brief  Sends the reset command to the barometer
  * @note   This function should be called once during barometer initialization
  * @retval Status
  */
extern status_t barometerReset(void)
{
    uint8_t cmd = BAROMETER_CMD_RESET;

    if (i2cWrite(BAROMETER_ADDRESS, &cmd, 1) != STATUS_OK) {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/**
  * @brief  Gets the calibration coefficients from the barometer's PROM
  * @note   This function should be called once during barometer initialization (must call barometerReset first!)
  *         Values are stored in the global array 'barometer_calibration'
  * @retval Status
  */
extern status_t barometerGetCalibration(void)
{
    uint8_t cmd;
    uint8_t buffer[2];

    for (int i = 0; i < 8; i++) {
        cmd = BAROMETER_CMD_PROM_READ | i * 2;
        if (i2cWrite(BAROMETER_ADDRESS, &cmd, 1) != STATUS_OK) {
            return STATUS_ERROR;
        }
        if (i2cRead(BAROMETER_ADDRESS, buffer, 2) != STATUS_OK) {
            return STATUS_ERROR;
        }
        barometer_calibration[i] = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
    }

    return STATUS_OK;
}

/**
  * @brief  Initializes the barometer
  * @note   This function should be called once during system initialization
  * @retval Status
  */
extern status_t barometerInit(void)
{
    if (barometerReset() != STATUS_OK) {
        return STATUS_ERROR;
    }
    if (barometerGetCalibration() != STATUS_OK) {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/**
  * @brief  Requests ADC conversion and then reads the resulting data
  * @param  d The requested data (choose from constants defined at top of file)
  * @param  osr The over sampling rate for the ADC conversion (choose from constants defined at top of file)
  * @param  buffer A pointer to the buffer to store the data
  * @retval Status
  */
extern status_t barometerGetData(uint8_t d, uint8_t osr, uint8_t *buffer)
{
    uint8_t cmd = BAROMETER_CMD_ADC_CONV | d | osr;

    if (i2cWrite(BAROMETER_ADDRESS, &cmd, 1)  != STATUS_OK) {
        return STATUS_ERROR;
    }
    /* Delay as needed to allow ADC conversion to complete */
    switch (osr & 0x0f) {
        case BAROMETER_CMD_ADC_256:
            HAL_Delay(1);
            break;
        case BAROMETER_CMD_ADC_512:
            HAL_Delay(2);
            break;
        case BAROMETER_CMD_ADC_1024:
            HAL_Delay(3);
            break;
        case BAROMETER_CMD_ADC_2048:
            HAL_Delay(5);
            break;
        case BAROMETER_CMD_ADC_4096:
            HAL_Delay(10);
            break;
        default:
            return STATUS_ERROR;
    }
    cmd = BAROMETER_CMD_ADC_READ;
    if (i2cWrite(BAROMETER_ADDRESS, &cmd, 1)  != STATUS_OK) {
        return STATUS_ERROR;
    }
    if (i2cRead(BAROMETER_ADDRESS, buffer, 3) != STATUS_OK) {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/**
  * @brief  Gets the uncompensated pressure and temperature values
  * @param  d1 A pointer to store uncompensated pressure value in mbar
  * @param  d2 A pointer to store uncompensated temperature value in *C
  * @retval Status
  */
extern status_t barometerGetUncompensatedValues(uint32_t *d1, uint32_t *d2)
{
    uint8_t buffer[3];

    if (barometerGetData(BAROMETER_CMD_ADC_D2, BAROMETER_CMD_ADC_4096, buffer) != STATUS_OK) {
        return STATUS_ERROR;
    }
    *d1 = ((uint32_t)buffer[0] << 16) | ((uint32_t)buffer[1] << 8) | (uint32_t)buffer[2];

    if (barometerGetData(BAROMETER_CMD_ADC_D2, BAROMETER_CMD_ADC_4096, buffer) != STATUS_OK) {
        return STATUS_ERROR;
    }
    *d2 = ((uint32_t)buffer[0] << 16) | ((uint32_t)buffer[1] << 8) | (uint32_t)buffer[2];

    return STATUS_OK;
}

/**
  * @brief  Calculates the compensated pressure and temperature values
  * @note   The below algorithm is based on the MS5607-02BA03 datasheet
  * @param  d1 The uncompensated pressure value
  * @param  d2 The uncompensated temperature value
  * @param  pressure A pointer to store compensated pressure value
  * @param  temperature A pointer to store compensated temperature value
  * @retval Status
  */
extern status_t barometerCompensateValues(uint32_t d1, uint32_t d2, uint32_t *pressure, uint32_t *temperature)
{
    uint32_t dt, p, t;
    int64_t off, sens;
    uint32_t t2 = 0;
    int64_t off2 = 0;
    int64_t sens2 = 0;

    dt = d2 - (barometer_calibration[4] * pow(2,8));
    t = (2000 + (dt * barometer_calibration[5]) / pow(2,23));
    off = barometer_calibration[1] * pow(2,17) + (barometer_calibration[3] * dt) / pow(2,6);
    sens = barometer_calibration[0] * pow(2,16) + (barometer_calibration[2] * dt) / pow(2,7);

    if (t < 2000) {
        t2 = (dt * dt) / pow(2,31);
        off2 = (61 * (t - 2000) * (t - 2000)) / pow(2,4);
        sens2 = 2 * (t - 2000) * (t - 2000);

        if (t < -1500) {
            off2 = off2 + 15 * (t + 1500) * (t + 1500);
            sens2 = sens2 + 8 * (t + 1500) * (t + 1500);
        }
    }
    t = t - t2;
    off = off - off2;
    sens = sens - sens2;

    p = (((d1 * sens) / pow(2,21) - off) / pow(2,15));

    /* convert to mbar */
    *pressure = p / 100;
    /* convert to *C */
    *temperature = t / 100;

    return STATUS_OK;
}

/**
  * @brief  Gets the compensated pressure and temperature values
  * @param  pressure A pointer to store compensated pressure value
  * @param  temperature A pointer to store compensated temperature value
  * @retval Status
  */
extern status_t barometerGetCompensatedValues(uint32_t *pressure, uint32_t *temperature)
{
    uint32_t d1, d2, p, t;

    if (barometerGetUncompensatedValues(&d1, &d2) != STATUS_OK) {
        return STATUS_ERROR;
    }
    if (barometerCompensateValues(d1, d2, &p, &t) != STATUS_OK) {
        return STATUS_ERROR;
    }

    *pressure = p;
    *temperature = t;

    return STATUS_OK;
}

/**
  * @brief  Gets the compensated pressure value
  * @param  pressure A pointer to store compensated pressure value
  * @retval Status
  */
extern status_t barometerGetCompensatedPressure(uint32_t *pressure)
{
    uint32_t d1, d2, p, t;

    if (barometerGetUncompensatedValues(&d1, &d2) != STATUS_OK) {
        return STATUS_ERROR;
    }
    if (barometerCompensateValues(d1, d2, &p, &t) != STATUS_OK) {
        return STATUS_ERROR;
    }

    *pressure = p;

    return STATUS_OK;
}

/* Accelerometer Functions -------------------------------------------------- */

/**
  * @brief  Reads from an accelerometer register(s)
  * @param  sub_address The address of the register to read from (choose from constants defined at top of file)
  * @param  data A pointer to the buffer to store the data
  * @param  size The number of bytes to read (0 < size <= 8)
  * @note   If size > 1 the register address is automatically incremented for reading from consecutive registers
  * @retval Status
  */
extern status_t accelerometerReadRegister(uint8_t sub_address, uint8_t *buffer, uint16_t size)
{
    uint8_t reg;

    if (size > 1) {
        reg = sub_address | ACCELEROMETER_AUTO_INCREMENT;
    } else {
        reg = sub_address;
    }

    if (i2cWrite(ACCELEROMETER_ADDRESS, &reg, 1) != STATUS_OK) {
        return STATUS_ERROR;
    }  

    if (i2cRead(ACCELEROMETER_ADDRESS, buffer, size) != STATUS_OK) {
        return STATUS_ERROR;
    }    

    return STATUS_OK;
}

/**
  * @brief  Writes to an accelerometer register
  * @param  sub_address The address of the register to write to (choose from constants defined at top of file)
  * @param  data The data to write
  * @retval Status
  */
extern status_t accelerometerWriteRegister(uint8_t sub_address, uint8_t data)
{
    uint8_t cmd[2];

    cmd[0] = sub_address;
    cmd[1] = data;
    if (i2cWrite(ACCELEROMETER_ADDRESS, cmd, 2) != STATUS_OK) {
        return STATUS_ERROR;
    }    

    return STATUS_OK;
}

/**
  * @brief  Initializes the accelerometer
  * @note   This function should be called once during system initialization
  * @retval Status
  */
extern status_t accelerometerInit(void)
{
    return STATUS_OK;
}

/**
  * @brief  Gets the z acceleration data from the accelerometer's registers
  * @param  z A pointer to store z-axis acceleration value
  * @retval Status
  */
extern status_t accelerometerGetData(int16_t *z)
{
    uint8_t buffer[6];

    if (accelerometerReadRegister(ACCELEROMETER_REG_OUT_X_L, buffer, 6) != STATUS_OK) {
        return STATUS_ERROR;
    }
    *z = (int16_t)buffer[4] | ((int16_t)buffer[5] << 8);

    return STATUS_OK;
}
