files= serial.S i2c.c printf.c main.c

main.elf: $(files)
	msp430-gcc -Os $(files) -o main.elf -mmcu=msp430g2553
	msp430-objdump -z -EL -D -W main.elf >main.lss
	msp430-size main.elf
	msp430-objcopy -O ihex main.elf main.hex

flash:
	mspdebug tilib -d USB --force-reset "prog main.elf"

clean:
	rm -f *.elf *.hex *.lss
