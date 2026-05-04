#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

//typedef unsigned char byte;


typedef uint8_t byte;
typedef byte* byte_ptr;

#define MAX_PAGES_PER_POOL 2

//Block sizes
#define POOL16_SIZE 16
#define POOL32_SIZE 32
#define POOL64_SIZE 64
#define POOL128_SIZE 128

//Số slot mỗi page
#define SLOT_COUNT 32

/*class slot//12 byte, hy sinh 4 byte de toc do doc nhanh hon
{
public:
    byte_ptr handle;//8 byte, truyen du lieu vao day
    int capacity;//4 byte, kich thuoc slot
public:
    slot()
    {
        handle = NULL;
        capacity = 0;
    }
    void Load(byte_ptr val)
    {

    }
    int* GetSlotIndexPtr() const
    {
        //return (int*)(handle - 4);
        return (int*)(handle - sizeof(int));
    }
    int* GetPageIndexPtr() const
    {
        //return (int*)(handle - 2*sizeof(int));
        return GetSlotIndexPtr() - 1;
    }
};*/

//Page chia thanh nhieu loai theo kich thuoc
class Page//16 byte
{
public:
    int slotCount;//32 slot
    int slotCapacity;//Each slot's size (16/32/64/128)
    byte_ptr data;//Pointer to memory
    unsigned long long freeSlots;//Bitmap of slots

public:
    void Init(byte_ptr buffer, int cap, int count)// Tạo page
    {
        slotCount = count;
        slotCapacity = cap; 
        data = buffer;
        freeSlots = 0;
    }

    //Return index can use to saving data
    //Check every slot that free for saving enough data
    int allocSlot()
    {
        for (int i = 0; i < slotCount; i++)
        {
            if (!(freeSlots & (1ULL << i)))//if slot is empty
            {
                freeSlots |= (1ULL << i);//Sign this slot is busy now
                return i;// return busy slot's index 
            }
        }
        return -1;//Have no empty slot
    }

    void freeSlot(int i)//give exactly rented slot
    {
        freeSlots &= ~(1ULL << i);//Sign this slot is empty now
    }

    byte_ptr getSlot(int i)// Take pointer of that index
    {
        return data + i * slotCapacity;//1st addr + index * slotCap
    }
    bool checkSlot(byte_ptr ptr)//Check ptr included in page or not
    {
        /*(Addr of ptr > 1st addr of data) AND (Addr of ptr < Last addr of data) */
        return ((ptr >= data) && (ptr < data + slotCapacity * slotCount));
    }
};

static byte pool16_mem[MAX_PAGES_PER_POOL][POOL16_SIZE * SLOT_COUNT];//[2][16*32]
static byte pool32_mem[MAX_PAGES_PER_POOL][POOL32_SIZE * SLOT_COUNT];//[2][32*32]
static byte pool64_mem[MAX_PAGES_PER_POOL][POOL64_SIZE * SLOT_COUNT];//[2][64*32]
static byte pool128_mem[MAX_PAGES_PER_POOL][POOL128_SIZE * SLOT_COUNT];//[2][128*32]

class Pool
{
public:
    Page pages[MAX_PAGES_PER_POOL];//Pages[2]
    int slotSize;

public:
    //
    void init(byte_ptr mem,int pageSize, int sSize)
    {
        slotSize = sSize;//Capacity of pools

        for (int i = 0; i < MAX_PAGES_PER_POOL; i++)
        {
            //Init 2 pages
            pages[i].Init(mem + i * pageSize, sSize, SLOT_COUNT);
        }
    }

    byte_ptr alloc()
    {
        for (int i = 0; i < MAX_PAGES_PER_POOL; i++)
        {
            int idx = pages[i].allocSlot();//Take free slots
            if (idx >= 0)//Have addr of fit slots
            {
                return pages[i].getSlot(idx);//Return that addr
            }
        }
        return NULL;
    }

    bool free(byte_ptr ptr)//Free all pages
    {
        for (int i = 0; i < MAX_PAGES_PER_POOL; i++)//0 -> 2
        {
            if (pages[i].checkSlot(ptr))//That addr is in data
            {
                //Check that exactly 1st addr of pages
                if ((ptr - pages[i].data) % pages[i].slotCapacity != 0) 
                    return false;
                
                int index = (ptr - pages[i].data) / pages[i].slotCapacity;//Calculate index of that addr

                pages[i].freeSlot(index);//Free slot
                return true;
            }
        }
        return false;//Cannot free pages
    }
};

class MemoryManager
{
private:
    Pool pool16;
    Pool pool32;
    Pool pool64;
    Pool pool128;

public:
    void init()
    {
        pool16.init(&pool16_mem[0][0], POOL16_SIZE * SLOT_COUNT,POOL16_SIZE);
        pool32.init(&pool32_mem[0][0], POOL32_SIZE * SLOT_COUNT, POOL32_SIZE);
        pool64.init(&pool64_mem[0][0], POOL64_SIZE * SLOT_COUNT, POOL64_SIZE);
        pool128.init(&pool128_mem[0][0], POOL128_SIZE * SLOT_COUNT, POOL128_SIZE);
    }

    byte_ptr alloc(size_t size)//Allocate pools depend on sizes
    {
        if (size <= 16) return pool16.alloc();
        if (size <= 32) return pool32.alloc();
        if (size <= 64) return pool64.alloc();
        if (size <= 128) return pool128.alloc();
        return NULL;
    }

    void free(byte_ptr ptr)
    {
        if (pool16.free(ptr)) return;
        if (pool32.free(ptr)) return;
        if (pool64.free(ptr)) return;
        if (pool128.free(ptr)) return;
    }
};

MemoryManager mm;//Create MemoryManager protocol