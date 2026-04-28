#include <stdint.h>
#include "timer.cpp"

//USART1
#define USART1_BRR    (*(volatile uint32_t*)0x40013808) //Baudrate
#define USART1_CR1    (*(volatile uint32_t*)0x4001380C) //Control Register
#define USART1_SR     (*(volatile uint32_t*)0x40013800) //Status Register
#define USART1_DR     (*(volatile uint32_t*)0x40013804) //Data register

Timer systime(1000000);

// Gửi 1 ký tự
void uart_send_char(char c) 
{
    while (!(USART1_SR & (1 << 7))); // Bit 7: TX Empty
    USART1_DR = c;//Write in Data register
}

// Gửi chuỗi
void uart_send_string(const char* str) 
{
    while (*str) 
    {
        uart_send_char(*str++);
    }
}

// Khởi tạo UART1
void uart_init() 
{
    // Baudrate 72MHz -> 9600
    USART1_BRR = 0x1D4C;

    // Enable TX + USART
    USART1_CR1 |= (1 << 3); // Transmitter Enable
    USART1_CR1 |= (1 << 13); // USART Enable
}

int main()
{
    uart_init();

    while (1) 
    {
        systime.Start();
        uart_send_string("Hello World!\r\n");
    }
}
