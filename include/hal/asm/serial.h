#ifndef HAL_ASM_SERIAL_H_
#define HAL_ASM_SERIAL_H_

#define SERIAL_COM1_IOPORT  0x3f8

#define SERIAL_COM2_IOPORT  0x2f8

#define SERIAL_COM3_IOPORT  0x3e8

#define SERIAL_COM4_IOPORT  0x2e8


#define SERIAL_REG_DATA_BUFFER      0

#define SERIAL_REG_DIVISOR_LOW      0

#define SERIAL_REG_INTR_ENABLE      1

#define SERIAL_REG_DIVISOR_HIGH     1

#define SERIAL_REG_INTR_ID          2

#define SERIAL_REG_FIFO_CTRL        2

#define SERIAL_REG_LINE_CTRL        3

#define SERIAL_REG_MODEM_CTRL       4

#define SERIAL_REG_LINE_STATUS      5

#define SERIAL_REG_MODEM_STATUS     6

#define SERIAL_REG_SCRATCH          7


#endif
