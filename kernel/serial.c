#include <stdint.h>
#include <stdbool.h>

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#define PORT 0x3f8 // COM1

int serial_init()
{
    outb(PORT + 1, 0x00); // Disable all interrupts
    outb(PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00); //                  (hi byte)
    outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set

    // test serial in loopback mode
    outb(PORT + 4, 0x1E);
    outb(PORT + 0, 0xAE);
    if (inb(PORT + 0) != 0xAE)
    {
        return 1;
    }

    // set normal operation mode
    outb(PORT + 4, 0x0F);

    return 0;
}

int serial_received()
{
    return inb(PORT + 5) & 1;
}

char serial_getc()
{
    while (serial_received() == 0)
        ;

    return inb(PORT);
}

int is_transmit_empty()
{
    return inb(PORT + 5) & 0x20;
}

void serial_putc(char a)
{
    while (is_transmit_empty() == 0)
        ;

    outb(PORT, (uint8_t)a);
}

void serial_write(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
        {
            serial_putc('\r');
        }
        serial_putc(*s++);
    }
}