#include <msp430.h>

#define PC r0                   
#define SP r1
#define SR r2
#define ARG1 R15
#define ARG2 R14
#define ARG3 R13
#define ARG4 R12

; serial.asm

            .lcomm  in_bit_mask, 2      ; Serial in pin
            .lcomm  out_bit_mask, 2     ; Serial out pin
            .lcomm  bit_dur, 2          ; Bit duration in cycles
            .lcomm  half_dur, 2         ; Half bit duration in cycles
                                        ;
            .text                       ;
            .global serial_setup        ; void serial_setup(unsigned out_mask, unsigned in_mask, unsigned bit_duration);
            .global serial_putc         ; void serial_putc(unsigned c);
            .global serial_puts         ; void serial_puts(char *s);
            .global serial_getc         ; unsigned serial_getc(void);
                                        ;
            .p2align 1,0                ; align on a word boundary (GCC)
                                        ;
serial_setup:                           ; - Setup serial I/O bitmasks and bit duration (32 minimum)
            mov     ARG1, &out_bit_mask ; Save serial output bitmask
            mov     ARG2, &in_bit_mask  ; Save serial input bitmask
            bis.b   ARG1, &P1DIR        ; Setup output pin
            bis.b   ARG1, &P1OUT        ;
            bic.b   ARG2, &P1DIR        ; Setup input pin
            bis     ARG2, ARG1          ;
            bic.b   ARG1, &P1SEL        ; Setup peripheral select
            mov     ARG3, ARG1          ;
            sub     #16, ARG3           ; Adjust count for loop overhead
            rla     ARG3                ; Multiply by 2 because NOP is two bytes
            mov     ARG3, &bit_dur      ; Save bit duration
            sub     #32, ARG1           ; Adjust count for loop overhead
            mov     ARG1, &half_dur     ; Save half bit duration
            ret                         ; Return
                                        ;
                                        ; - Send a single char
serial_putc:                            ; Char to tx in ARG1
                                        ; ARG1, ARG2, ARG3, ARG4 trashed
            mov     &out_bit_mask, ARG4 ; Serial output bitmask
            mov     &bit_dur, ARG3      ; Bit duration
            bis     #0x0300, ARG1       ; Stop bit(s)
            jmp     bit_low             ; Send start bit...
                                        ;
tx_bit:     mov     ARG3, ARG2          ; Get bit duration
tx_delay:   nop                         ; 4 cycle loop
            sub     #8, ARG2            ;
            jc      tx_delay            ;
            subc    ARG2, PC            ; 0 to 3 cycle delay
            nop                         ; 3
            nop                         ; 2
            nop                         ; 1
                                        ;
            rra     ARG1                ; Get bit to tx, test for zero
            jc      bit_high            ; If high...
bit_low:    bic.b   ARG4, &P1OUT        ; Send zero bit
            jmp     tx_bit              ; Next bit...
bit_high:   bis.b   ARG4, &P1OUT        ; Send one bit
            jnz     tx_bit              ; If tx data is not zero, then there are more bits to send...
                                        ;
            ret                         ; Return when all bits sent         
                                        ;
                                        ;
                                        ; - Send a NULL terminated string
serial_puts:                            ; Tx string using serial_putc
            push    R11                 ;
            mov     ARG1, R11           ; String pointer in ARG1, copy to R11
putsloop:                               ;
            mov.b   @R11+, ARG1         ; Get a byte, inc pointer
            tst.b   ARG1                ; Test if end of string
            jz      putsx               ; Yes, exit...
            call    #serial_putc        ; Call serial_putc
            jmp     putsloop            ;
putsx:      pop     R11                 ;
            ret                         ;
                                        ;
serial_getc:                            ; - Get a char
            mov     &bit_dur, ARG3      ; Bit duration
            mov     &in_bit_mask, ARG2  ; Input bitmask
            mov     #0x01FF, ARG1       ; 9 bits - 8 data + stop
                                        ;
rx_start:                               ; Wait for start bit
            mov.b   &P1IN, ARG4         ; Get serial input          
            and     ARG2, ARG4          ; Mask and test bit
            jc      rx_start            ; Wait for low...
                                        ;
            mov     &half_dur, ARG2     ; Wait for 1/2 bit time
                                        ;
rx_delay:   nop                         ; Bit delay
            sub     #8, ARG2            ;
            jc      rx_delay            ;
            subc    ARG2, PC            ; 0 to 3 cycle delay
            nop                         ; 3
            nop                         ; 2
            nop                         ; 1
                                        ;
            mov.b   &P1IN, ARG4         ; Get serial input          
            and     &in_bit_mask, ARG4  ;
            rrc     ARG1                ; Shift in a bit
                                        ;
            mov     ARG3, ARG2          ; Setup bit timer
            jc      rx_delay            ; Next bit...
                                        ;
            rla     ARG1                ; Move stop bit to carry
            swpb    ARG1                ; Move rx byte to lower byte, start bit in msb
            ret                         ; Return with rx char and start bit in ARG1, stop bit in carry
                                        ;        
