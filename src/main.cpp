#include <stdint.h>
#include "timer.cpp"
#include "memorymanager.cpp"

//USART1
#define USART1_BRR    (*(volatile uint32_t*)0x40013808) //Baudrate
#define USART1_CR1    (*(volatile uint32_t*)0x4001380C) //Control Register
#define USART1_SR     (*(volatile uint32_t*)0x40013800) //Status Register
#define USART1_DR     (*(volatile uint32_t*)0x40013804) //Data register

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)//Chân ngắt Clock
#define GPIOA_CRH     (*(volatile uint32_t*)0x40010804)//GPIOA (PA9) để xuất dữ liệu ra chân vật lí

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
    // 1. Bật Clock cho GPIOA và USART1
    RCC_APB2ENR |= (1 << 2);  // Enable GPIOA Clock
    RCC_APB2ENR |= (1 << 14); // Enable USART1 Clock

    // 2. Cấu hình chân PA9 (TX) là Alternate Function Push-Pull (50MHz)
    // Xóa bit cũ và set 0xB (1011) cho Pin 9
    GPIOA_CRH &= ~(0xF << 4); 
    GPIOA_CRH |=  (0xB << 4);

    // Baudrate 72MHz -> 9600
    USART1_BRR = 0x1D4C;

    // Enable TX + USART
    USART1_CR1 |= (1 << 3); // Transmitter Enable
    USART1_CR1 |= (1 << 13); // USART Enable
}


MemoryManager mm;//Create MemoryManager protocol

// ================= TEST HELPER =================
void fill(byte_ptr ptr, int size, byte val)
{
    for (int i = 0; i < size; i++)
        ptr[i] = val;
}

bool check(byte_ptr ptr, int size, byte val)
{
    for (int i = 0; i < size; i++)
        if (ptr[i] != val) return false;
    return true;
}

// ================= TEST 1 =================
// Basic alloc/free
void test_basic()
{
    uart_send_string("TEST BASIC\n");

    //Allocate pool16/pool32 to saving data
    byte_ptr a = mm.alloc(10);
    byte_ptr b = mm.alloc(20);

    //Push values to pointer
    fill(a, 10, 0xAA);
    fill(b, 20, 0xBB);

    //Checking
    uart_send_string("Check A: ");
    if (check(a, 10, 0xAA))
    {
        uart_send_string("OK\n"); 
    }
    else
    {
        uart_send_string("FAIL\n");
    }

    uart_send_string("Check B: ");
    if(check(b, 20, 0xBB))
    {
        uart_send_string("OK\n"); 
    }
    else
    {
        uart_send_string("FAIL\n");
    }

    //Free pointer
    mm.free(a);
    mm.free(b);
}

// ================= TEST 2 =================
// Full capacity test
void test_full()
{
    uart_send_string("\nTEST FULL\n");

    byte_ptr arr[200];
    int count = 0;

    //Push all blocks until full
    while (1)
    {
        byte_ptr p = mm.alloc(16); //pool16
        if (!p) break;//p have no value -> break loop

        arr[count++] = p;//
    }

    uart_send_string("Allocated ");
    uart_send_char(char(count));
    uart_send_string("blocks\n");

    // free all
    for (int i = 0; i < count; i++)
        mm.free(arr[i]);
}

// ================= TEST 3 =================
// Reuse slot after free
void test_reuse()
{
    uart_send_string("\nTEST REUSE\n");

    byte_ptr a = mm.alloc(16);
    mm.free(a);

    byte_ptr b = mm.alloc(16);


    uart_send_string("Reuse same slot: ");
    if(a == b)
    {
        uart_send_string("YES\n"); 
    }
    else
    {
        uart_send_string("NO\n");
    }

    mm.free(b);
}

// ================= TEST 4 =================
// Overlap detection
void test_overlap()
{
    printf("\nTEST OVERLAP\n");

    byte_ptr a = mm.alloc(32);
    byte_ptr b = mm.alloc(32);

    fill(a, 32, 0xAA);
    fill(b, 32, 0xBB);

    bool ok = check(a, 32, 0xAA);

    uart_send_string("Memory isolated: ");
    if(ok)
    {
        uart_send_string("YES\n"); 
    }
    else
    {
        uart_send_string("FAIL\n");
    }

    mm.free(a);
    mm.free(b);
}

// ================= TEST 5 =================
// Invalid free
void test_invalid_free()
{
    uart_send_string("\nTEST INVALID FREE\n");

    byte fake[16];

    mm.free(fake); // không thuộc pool

    uart_send_string("No crash -> OK\n");
}

// ================= TEST 6 =================
// Alignment test
void test_alignment()
{
    uart_send_string("\nTEST ALIGNMENT\n");

    byte_ptr a = mm.alloc(32);

    printf("Aligned: %s\n", ((uintptr_t)a % 4 == 0) ? "YES" : "NO");


    uart_send_string("Aligned:  ");
    if(((uintptr_t)a % 4 == 0))
    {
        uart_send_string("YES\n"); 
    }
    else
    {
        uart_send_string("NO\n");
    }

    mm.free(a);
}


int main()
{
    uart_init();

    while (1) 
    {
        systime.Start();

        mm.init();

        test_basic();
        test_full();
        test_reuse();
        test_overlap();
        test_invalid_free();
        test_alignment();
    }
}
