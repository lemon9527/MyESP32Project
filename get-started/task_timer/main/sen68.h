#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#define SEN68_SLAVE_ADDR               0x6B

#define SEN68_CMD_START_MEASUREMENT    0x0021
#define SEN68_CMD_STOP_MEASUREMENT     0x0104
#define SEN68_CMD_READ_DATA_READY      0x0202
#define SEN68_CMD_READ_MEASUREMENT     0x0467
#define SEN68_CMD_READ_PRODUCT_NAME    0xD014
#define SEN68_CMD_READ_SERIAL_NUMBER   0xD033
#define SEN68_CMD_READ_DEVICE_STATUS   0xD206
#define SEN68_CMD_CLEAR_DEVICE_STATUS  0xD210
#define SEN68_CMD_RESET                0xD304

typedef struct {
    float pm1_0;
    float pm2_5;
    float pm4_0;
    float pm10;
    float humidity;
    float temperature;
    float voc_index;
    float nox_index;
    float hcho;
} sen68_data_t;

esp_err_t sen68_read_product_name(i2c_master_dev_handle_t dev_handle);
esp_err_t sen68_read_serial_number(i2c_master_dev_handle_t dev_handle);
esp_err_t sen68_start_measurement(i2c_master_dev_handle_t dev_handle);
esp_err_t sen68_read_data_ready(i2c_master_dev_handle_t dev_handle, bool *ready);
esp_err_t sen68_read_measurement(i2c_master_dev_handle_t dev_handle, sen68_data_t *out_data);
esp_err_t sen68_stop_measurement(i2c_master_dev_handle_t dev_handle);

/*
  I2C Address: 0x6B

  ## Commands

  | Function                                             | Command  |
  | ---------------------------------------------------- | -------- |
  | Start Continuous measurement                         |  0x0021  |
  | Stop measurement                                     |  0x0104  |
  | Read Data-Ready Flag                                 |  0x0202  |
  | Read Measured Values                                 |  0x0467  |
  | Read/Write Temperature Compensation Parameters       |  0x60B2  |
  | Read/Write VOC Algorithm Tuning Parameters           |  0x60D0  |
  | Read Sensor Name                                     |  0xD014  |
  | Read Serial Number                                   |  0xD033  |
  | Read Device Status                                   |  0xD206  |
  | Clear Device Status                                  |  0xD210  |
  | Reset                                                |  0xD304  |

*/

/*
    ## Starts continuous measurement on the SEN68 sensor. (0x0021)

    Start Continuous Measurement
    Command ID 0x0021
    Available in Idle mode
    Execution Time 50 ms
    Max. RX Data With CRC 0 Bytes
    TX Data None
    RX Data None

    Description: Starts a continuous measurement. After starting the measurement, it takes some time (~1.1s) until 
    the first measurement results are available. You could poll with the command Get Data Ready to check when 
    the results are ready to be read
*/

/*
  ## Stop Measurement (0x0104)

    Stop Measurement
    Command ID 0x0104
    Available in Measurement mode
    Execution Time 1400 ms
    Max. RX Data With CRC 0 Bytes
    TX Data None
    RX Data None

  Returns the sensor to idle mode. Required before re-writing tuning parameters
  (e.g. VOC Algorithm Tuning) so the new values can take effect on the next
  `new_start_measurement/1`.
*/

/*
  ## Set VOC Algorithm Tuning Parameters (0x60D0)

  Writes the six VOC engine tuning parameters to the sensor. Each parameter is a
  big-endian int16 (scale factor 1) followed by its CRC checksum. The parameters
  only take effect when measurement starts, so this must be sent in idle mode
  (before `new_start_measurement/1`).

    Send: 0x60D0 [DF0] [DF1] [CS] ... [DF15] [DF16] [CS]

  Parameter order (matches the SGP40 software algorithm constants in
  `BlofeldFirmware.AQSensor.Driver.VOC.SenSGP40DR4Voc.VocAlgorithm`):

    1. index_offset           -> VOC Index Offset
    2. learning_time          -> Learning Time Offset Hours (bytes 3..4)
    3. learning_gain          -> Learning Time Gain Hours
    4. gating_max             -> Gating Max Duration Minutes
    5. std_initial            -> Std Initial
    6. gain_factor            -> Gain Factor
*/

/*
  ## Reads the data-ready flag from the SEN68 sensor.
  This command checks if the sensor has new data available.

    Get Data Ready
    Command ID 0x0202
    Available in Measurement mode
    Execution Time 20 ms
    Max. RX Data With CRC 3 Bytes
    TX Data None
    RX Data             Byte #                  Description
                        0                       Padding: uint8
                                                Padding byte, always 0x00.
                        1                       Data Ready: bool8
                                                True (0x01) if data is ready, False (0x00) if not. When no 
                                                measurement is running, False will be returned.
                        2                       CRC CRC for the previous two bytes.

*/

/*
  ## Reads the Product name from the SEN68 sensor.
  This command retrieves the product name of the sensor.

    Get Product Name
    Command ID 0xD014
    Available in Idle and measurement mode
    Execution Time 20 ms
    Max. RX Data With CRC 48 Bytes
    TX Data None
    RX Data             Byte #                  Description
                        0 Char 0
                        1 Char 1
                        2 CRC
                        … …
                        45 Char 30
                        46 Char 31
                        47 CRC
    Product Name: string<32>
    Null-terminated ASCII string containing the product name. Up 
    to 32 characters can be read from the device
*/

/*
  ## Reads the Serial number from the SEN68 sensor.
  This command retrieves the serial number of the sensor.

    Command ID 0xD033
    Available in Idle and measurement mode
    Execution Time 20 ms
    Max. RX Data With CRC 48 Bytes
    TX Data None
    RX Data             Byte #                  Description
                        0 Char 0
                        1 Char 1
                        2 CRC
                        … …
                        45 Char 30
                        46 Char 31
                        47 CRC
    Serial Number: string<32>
    Null-terminated ASCII string containing the serial number. Up 
    to 32 characters can be read from the device.
*/

/*
  ## Reads the measured values from the SEN68 sensor.
  This command retrieves the latest sensor measurements.
Description: Returns the measured values. The command Get Data Ready can be used to check if new data is 
available since the last read operation. If no new data is available, the previous values will be returned. If no 
data is available at all (e.g. measurement not running for at least one second), all values will be at their upper 
limit (0xFFFF for uint16, 0x7FFF for int16).

Read Measured Values SEN68
Command ID 0x0467
Available in Measurement mode
Execution Time 20 ms
Max. RX Data With CRC 27 Bytes
TX Data None

RX Data:

  **Byte #**|** Datatype**   |** Scale factor**|** Description**
  ------|-------------------------|-----|-----------------------------------
  0..1  | big-endian uint16       |10   | Mass Concentration PM1.0 [µg/m³]
  2     | Checksum for bytes 0 1  |     |
  3..4  | big-endian uint16       |10   | Mass Concentration PM2.5 [µg/m³]
  5     | Checksum for bytes 3 4  |     |
  6..7  | big-endian uint16       |10   | Mass Concentration PM4.0 [µg/m³]
  8     | Checksum for bytes 6 7  |     |
  9..10 | big-endian uint16       |10   | Mass Concentration PM10 [µg/m³]
  11    | Checksum for bytes 9 10 |     |
  12..13| big-endian int16        |100  | Compensated Ambient Humidity [%RH]
  14    | Checksum for bytes 12 13|     |
  15..16| big-endian int16        |200  | Compensated Ambient Temperature [°C]
  17    | Checksum for bytes 15 16|     |
  18..19| big-endian int16        |10   | VOC Index
  20    | Checksum for bytes 18 19|     |
  21..22| big-endian int16        |10   | NOx Index
  23    | Checksum for bytes 21 22|     |
  24..25| big-endian int16        |10   | Formaldehyde HCHO [ppb] = value / 10
  26    | Checksum for bytes 24 25|     |

*/

/*
checksum 参考代码(Elixir)
  def calculate_checksum(<<value_hi, value_lo>>) do
    0xFF
    |> Bitwise.bxor(value_hi)
    |> byte_crc_calc(8)
    |> Bitwise.bxor(value_lo)
    |> byte_crc_calc(8)
  end

  defp byte_crc_calc(crc, bits) when bits > 0 do
    updated_crc =
      if Bitwise.band(crc, 0x80) > 0 do
        crc |> Bitwise.bsl(1) |> Bitwise.bxor(0x31)
      else
        crc |> Bitwise.bsl(1)
      end
      |> Bitwise.band(0xFF)

    byte_crc_calc(updated_crc, bits - 1)
  end

  defp byte_crc_calc(crc, 0), do: crc

*/