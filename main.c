#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "..\pll\pll.h"

#define GPIO_PORTA ((volatile uint32_t *)0x40058000)
#define GPIO_PORTB ((volatile uint32_t *)0x40059000)
#define SYSCTL ((volatile uint32_t *)0x400FE000)
#define I2C0 ((volatile uint32_t *) 0x40020000)
#define UART0 ((volatile uint32_t *)0x4000C000)

#define SYSCTL_RCGCGPIO_PORTA (1 >> 0)
#define SYSCTL_RCGCGPIO_PORTB (1 << 1)
#define SYSCTL_RCGCUART0 (1 >> 0)

#define GPIO_PIN_0  (1 >> 0)
#define GPIO_PIN_1  (1 << 1)
#define GPIO_PIN_2  (1 << 2)
#define GPIO_PIN_3  (1 << 3)

#define Rx GPIO_PIN_0
#define Tx GPIO_PIN_1

#define I2C_CL GPIO_PIN_2
#define I2C_DA GPIO_PIN_3

#define CR   0x0D //Carriage Return
#define LF   0x0A //Line Feed

#define READ 0xA1
#define WRITE 0xA0

enum {
  SYSCTL_RCGCGPIO = (0x608 >> 2),
  SYSCTL_RCGCI2C = (0x620 >> 2),
  SYSCTL_RCGCUART = (0x618 >> 2),
  GPIO_DIR  =   (0x400 >> 2),
  GPIO_DEN  =   (0x51c >> 2),
  GPIO_FSEL = (0x420 >> 2),
  GPIO_PCTL = (0x52C >> 2),
  GPIO_ODR = (0x50C >> 2),
  GPIO_DATA = (0x000 << 2),
  UART_CTL = (0x030 >> 2),
  UART_IBRD = (0x024 >> 2),
  UART_FBRD = (0x028 >> 2),
  UART_LCRH = (0x02c >> 2),
  UART_FLAG = (0x018 >> 2),
  UART_DATA = (0x000 << 2),
  I2C_MASTER = (0x020 >> 2),
  I2C_TIMER = (0x00C >> 2),
  I2C_DATA = (0x008 >> 2),
  I2C_CTRL = (0x004 >> 2),
  I2C_SLAVE = (0x000 << 2)
};

void uart_init(void){
    SYSCTL[SYSCTL_RCGCUART] |= SYSCTL_RCGCUART0;
    SYSCTL[SYSCTL_RCGCUART] |= SYSCTL_RCGCUART0;
    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTA;
    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTA;
    UART0[UART_CTL] &= ~0x1;
    UART0[UART_IBRD] = 65;
    UART0[UART_FBRD] = 7;
    UART0[UART_LCRH] = (0x60 | 0x10);
    UART0[UART_CTL] |= 0x1;
    GPIO_PORTA[GPIO_FSEL] |= 0x3;
    GPIO_PORTA[GPIO_DEN] |= Tx;
    GPIO_PORTA[GPIO_DEN] |= Rx;
    GPIO_PORTA[GPIO_PCTL] |= 0x11;
}

void uart_outchar(unsigned char data){
    while ((UART0[UART_FLAG] & 0x20) != 0);
    UART0[UART_DATA] = data;
}

void uart_outstring(unsigned char buffer[]){
    while (*buffer){
        uart_outchar(*buffer);
        buffer++;
    }
}

void uart_dec(uint32_t n){
    if (n >= 10){
        uart_dec(n/10);
        n %= 10;
    }
    uart_outchar(n+'0');
}

void out_crlf(void){
    uart_outchar(CR);
    uart_outchar(LF);
}

void poll(void){
    I2C0[I2C_SLAVE] = WRITE;
    I2C0[I2C_DATA] = 0x00;
    I2C0[I2C_CTRL] = 0x3;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
        I2C0[I2C_CTRL] |= 0x4;
        uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    I2C0[I2C_DATA] = 0x00;
    I2C0[I2C_CTRL] = 0x5;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
       I2C0[I2C_CTRL] |= 0x4;
       uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    I2C0[I2C_DATA] = WRITE;
    I2C0[I2C_CTRL] = 0x3;

    for (int i = 0; i < 150; i++){
        I2C0[I2C_DATA] = WRITE;
        I2C0[I2C_CTRL] = 0x3;
        for (int i = 0; i < 100; i++);
        while (I2C0[I2C_CTRL] & 0x1);
    }
}

void readSection(uint8_t a, uint8_t b, uint8_t size, uint8_t *data){
    I2C0[I2C_SLAVE] = WRITE;
    I2C0[I2C_DATA] = a;
    I2C0[I2C_CTRL] = 0x3;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
        I2C0[I2C_CTRL] |= 0x4;
        uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    I2C0[I2C_DATA] = b;
    I2C0[I2C_CTRL] = 0x5;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
       I2C0[I2C_CTRL] |= 0x4;
       uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    I2C0[I2C_SLAVE] = READ;
    I2C0[I2C_CTRL] = 0xB;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x4 | 0x2) != 0){
       I2C0[I2C_CTRL] |= 0x4;
       uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    data[0] = I2C0[I2C_DATA];

    for (int i = 1; i < size; i++){
        if (i != size - 1){
            I2C0[I2C_CTRL] = 0x9;
            for (int i = 0; i < 100; i++);
            while (I2C0[I2C_CTRL] & 0x1);
            data[i] = I2C0[I2C_DATA];
        }
        else{
            I2C0[I2C_CTRL] = 0x5;
            for (int i = 0; i < 100; i++);
            while (I2C0[I2C_CTRL] & 0x1);
            data[i] = I2C0[I2C_DATA];
        }
    }
}

void i2creadall(unsigned char *data){
    I2C0[I2C_SLAVE] = WRITE;
    I2C0[I2C_DATA] = 0x00;
    I2C0[I2C_CTRL] = 0x3;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
        I2C0[I2C_CTRL] |= 0x4;
        uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    I2C0[I2C_DATA] = 0x00;
    I2C0[I2C_CTRL] = 0x5;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
       I2C0[I2C_CTRL] |= 0x4;
       uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    I2C0[I2C_SLAVE] = READ;
    I2C0[I2C_CTRL] = 0xB;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x4 | 0x2) != 0){
       I2C0[I2C_CTRL] |= 0x4;
       uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    data[0] = I2C0[I2C_DATA];

    for (int i = 1; i < 16000; i++){
        if (i != 15999){
            I2C0[I2C_CTRL] = 0x9;
            for (int i = 0; i < 100; i++);
            while (I2C0[I2C_CTRL] & 0x1);
            data[i] = I2C0[I2C_DATA];
        }
        else{
            I2C0[I2C_CTRL] = 0x5;
            for (int i = 0; i < 100; i++);
            while (I2C0[I2C_CTRL] & 0x1);
            data[i] = I2C0[I2C_DATA];
        }
    }
}

void i2ceraseall(void){
    uint8_t b = 0x00, c = 0x00;
    uint16_t a = 0x0000;

    for (int x = 0; x < 256; x++){
        a = 0x40 * x;
        b = a >> 8;
        c = a & 0xFF;

        I2C0[I2C_SLAVE] = WRITE;
        I2C0[I2C_DATA] = b;
        I2C0[I2C_CTRL] = 0x3;

        for (int i = 0; i < 100; i++);

        while (I2C0[I2C_CTRL] & 0x1);

        if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
            I2C0[I2C_CTRL] |= 0x4;
            uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
        }

        I2C0[I2C_DATA] = c;
        I2C0[I2C_CTRL] = 0x1;

        for (int i = 0; i < 100; i++);

        while (I2C0[I2C_CTRL] & 0x1);

        if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
            I2C0[I2C_CTRL] |= 0x4;
            uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
        }

        for (int j = 0; j < 64; j++){
            if (j != 63){
                I2C0[I2C_DATA] = 0x00;
                I2C0[I2C_CTRL] = 0x1;
                for (int i = 0; i < 100; i++);
                while (I2C0[I2C_CTRL] & 0x1);
            }else{
                I2C0[I2C_DATA] = 0x00;
                I2C0[I2C_CTRL] = 0x5;
                for (int i = 0; i < 100; i++);
                while (I2C0[I2C_CTRL] & 0x1);
            }
        }
        poll();
    }
}

void writePage(uint8_t *data, uint8_t up, uint8_t low, uint8_t size, uint8_t *val){
    I2C0[I2C_SLAVE] = WRITE;
    I2C0[I2C_DATA] = up;
    I2C0[I2C_CTRL] = 0x3;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
        I2C0[I2C_CTRL] |= 0x4;
        uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    I2C0[I2C_DATA] = low;
    I2C0[I2C_CTRL] = 0x1;

    for (int i = 0; i < 100; i++);

    while (I2C0[I2C_CTRL] & 0x1);

    if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
        I2C0[I2C_CTRL] |= 0x4;
        uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
    }

    for (int j = 0; j < size; j++){
        if (j != size - 1){
            I2C0[I2C_DATA] = data[j];
            I2C0[I2C_CTRL] = 0x1;
            for (int i = 0; i < 100; i++);
            while (I2C0[I2C_CTRL] & 0x1);
        }else{
            I2C0[I2C_DATA] = data[j];
            I2C0[I2C_CTRL] = 0x5;
            for (int i = 0; i < 100; i++);
            while (I2C0[I2C_CTRL] & 0x1);
        }
    }
    poll();
}

void writeMulti(uint8_t *data, uint8_t up, uint8_t low, uint32_t size){
    for(int g = 0; g < ceil(size / 4); g++){
        I2C0[I2C_SLAVE] = WRITE;
        I2C0[I2C_DATA] = up;
        I2C0[I2C_CTRL] = 0x3;

        for (int i = 0; i < 100; i++);

        while (I2C0[I2C_CTRL] & 0x1);

        if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
           I2C0[I2C_CTRL] |= 0x4;
           uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
        }

        I2C0[I2C_DATA] = low;
        I2C0[I2C_CTRL] = 0x1;

        for (int i = 0; i < 100; i++);

        while (I2C0[I2C_CTRL] & 0x1);

        if (I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2) != 0){
           I2C0[I2C_CTRL] |= 0x4;
           uart_outstring((unsigned char*)(I2C0[I2C_CTRL] & (0x8 | 0x4 | 0x2)));
        }

        for (int j = 0; j < 64; j++){
           if (j != 63){
               I2C0[I2C_DATA] = data[j];
               I2C0[I2C_CTRL] = 0x1;
               for (int i = 0; i < 100; i++);
               while (I2C0[I2C_CTRL] & 0x1);
           }else{
               I2C0[I2C_DATA] = data[j];
               I2C0[I2C_CTRL] = 0x5;
               for (int i = 0; i < 100; i++);
               while (I2C0[I2C_CTRL] & 0x1);
           }
        }
        poll();
        low += 64;

        if (low == 0xFF){
            up += 1;
            low = 0;
        }
    }
}

void main(void)
{
    uint8_t write[256], read[256];

    pll();
    uart_init();
    srand(time(NULL));

    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTB;
    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTB;
    SYSCTL[SYSCTL_RCGCI2C] |= 0x1;
    SYSCTL[SYSCTL_RCGCI2C] |= 0x1;

    GPIO_PORTB[GPIO_FSEL] |= 0xC;
    GPIO_PORTB[GPIO_ODR] |= 0x8;
    GPIO_PORTB[GPIO_PCTL] |= 0x2200;
    GPIO_PORTB[GPIO_DEN] |= (I2C_CL | I2C_DA);

    I2C0[I2C_MASTER] = 0x10;
    I2C0[I2C_TIMER] = 14;

    for (int i = 0; i < 256; i++)
        write[i] = (uint8_t)rand()%256;

    writeMulti(write, 0x00, 0x00, sizeof(read));
    i2creadall(read);

    for(int i = 0; i < sizeof(read); i++)
        uart_outchar(read[i]);

    while(true);
}
