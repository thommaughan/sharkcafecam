//void serial_setup (unsigned out_mask, unsigned in_mask, unsigned duration);
void serial_init (unsigned long baud);
void serial_putc (unsigned);
void serial_puts (char *);
unsigned serial_getc (void);


#define SIZEOF_RXBUF    20
#define SIZEOF_TXBUF    20
