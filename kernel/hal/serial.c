#include <hal/io.h>
#include <hal/serial.h>

void serial_init(int base_ioport, unsigned int baud_rate) {
    unsigned int divisor = 115200 / baud_rate;

    /* disable interrupts */
    outb(base_ioport + SERIAL_REG_INTR_ENABLE,  0);

    /* 8N1, enable DLAB to allow setting baud rate */
    outb(base_ioport + SERIAL_REG_LINE_CTRL,    0x83);

    /* set baud rate */
    outb(base_ioport + SERIAL_REG_DIVISOR_LOW,  (divisor & 0xff));
    outb(base_ioport + SERIAL_REG_DIVISOR_HIGH, ((divisor >> 8) & 0xff));

    /* 8N1, disable DLAB */
    outb(base_ioport + SERIAL_REG_LINE_CTRL,    0x03);

    /* enable and clear FIFO
     *
     * Receive FIFO trigger level is not relevant for us as we are only
     * transmitting. */
    outb(base_ioport + SERIAL_REG_FIFO_CTRL,    0x07);

    /* assert DTR and RTS */
    outb(base_ioport + SERIAL_REG_MODEM_CTRL,   0x03);
}

void serial_printn(int base_ioport, const char *message, unsigned int n) {
    int idx;

    for(idx = 0; idx < n; ++idx) {
        serial_putc(base_ioport, message[idx]);
    }
}

void serial_putc(int base_ioport, char c) {
    /* wait for the UART to be ready to accept a new character */
    while( (inb(base_ioport + SERIAL_REG_LINE_STATUS) & 0x20) == 0) {}

    outb(base_ioport + SERIAL_REG_DATA_BUFFER, c);
}
