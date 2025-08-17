# Westberry Technology Wireless Module Prototype

This adds support for the Westberry Technology CH582F Wireless Module for use with QMK Keyboards.

Add the following to the list of modules in your `keymap.json` to enable this module:

```json
{
    "modules": ["emolitor/westberry_wireless"]
}
```

Then add configuration in `config.h` for the UART and EEPROM pins as well
as the bluetooth name.
```c
/* UART */
#define UART_TX_PIN A9
#define UART_TX_PAL_MODE 7
#define UART_RX_PIN A10
#define UART_RX_PAL_MODE 7

/* FLASH */
#define SPI_DRIVER SPIDQ
#define SPI_SCK_PIN B3
#define SPI_MOSI_PIN B5
#define SPI_MISO_PIN B4
#define SPI_MOSI_PAL_MODE 5
#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN C12

/* WIRELESS NAMES */
#define MD_BT_NAME "Bridge75 BT$"
```

Then add configuration in `mcuconf.h` for the UART and EEPROM.
```c
#undef WB32_SERIAL_USE_UART1
#define WB32_SERIAL_USE_UART1 TRUE

#undef WB32_SPI_USE_QSPI
#define WB32_SPI_USE_QSPI TRUE
```
