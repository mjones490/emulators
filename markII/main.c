#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include "6502_cpu.h"
#include "6502_shell.h"

const int PAGE_SIZE = 0x100;

BYTE *ram;

struct page_block_t {
    accessor_t accessor;
    BYTE first_page;
    int total_pages;
    BYTE *buffer;
};

typedef BYTE (*soft_switch_accessor_t)(BYTE switch_no, bool read, BYTE value);

struct soft_switch_t
{
    soft_switch_accessor_t read_accessor;
    soft_switch_accessor_t write_accessor; 
};

#define SS_READ  1
#define SS_WRITE 2

struct page_block_t *page_block[0x100];

BYTE bus_accessor(WORD address, bool read, BYTE value)
{
//    printf("Page = %02x\n", hi(address));
    if (NULL != page_block[hi(address)] && 
        NULL != page_block[hi(address)]->accessor)
        value = page_block[hi(address)]->accessor(address, read, value);
    else
        value = 0x00;
    
    return value;
}

inline struct page_block_t *get_page_block(WORD address)
{
    return page_block[hi(address)];
}
  
inline WORD pb_offset(struct page_block_t *pb, WORD address)
{
    return address - word(0, pb->first_page);
}
           

struct page_block_t *create_page_block(BYTE first_page, int total_pages)
{
    struct page_block_t *pb = malloc(sizeof(struct page_block_t));
    memset(pb, 0, sizeof(struct page_block_t));
    pb->first_page = first_page;
    pb->total_pages = total_pages;
    return pb;
}

BYTE *create_page_buffer(int total_pages)
{
    BYTE *buffer = malloc(total_pages * PAGE_SIZE);
    memset(buffer, 0, total_pages * PAGE_SIZE);
    return buffer;
}
           
bool install_page_block(struct page_block_t *pb)
{
    int i;
    struct page_block_t *prev_pb = page_block[pb->first_page];

    if (NULL != prev_pb) {
        if (NULL != prev_pb->accessor && NULL == pb->accessor)
            pb->accessor = prev_pb->accessor;

        if (NULL != prev_pb->buffer && NULL == pb->buffer)
            pb->buffer = prev_pb->buffer + 
                ((pb->first_page - prev_pb->first_page) * PAGE_SIZE);
    }

    for (i = 0; i < pb->total_pages; ++i)
        page_block[i + pb->first_page] = pb;
    
    return true;
}

void bus_init()
{
    
    printf("Size of page_block is %p\n", sizeof(page_block));
    memset(page_block, 0, sizeof(page_block));
}

static void cycle()
{
    static bool prev_state = true; // true == halted
    usleep(1);
    
    if (!cpu_get_signal(SIG_HALT)) {
        cpu_execute_instruction();
        prev_state = false;
    } else if (false == prev_state) {
        shell_print("CPU halted.\n");
        prev_state = true;
    }
}

            
BYTE RAM_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(address);
    
    if (NULL != pb->buffer) {
        if (read)
            value = pb->buffer[pb_offset(pb, address)];
        else
            pb->buffer[pb_offset(pb, address)] = value;
    } else {
        value = 0;
    }

    return value;
}

static BYTE *alt_zp_buf;
static BYTE *norm_zp_buf;

#define SS_SETSTDZP     0x08
#define SS_SETALTZP     0x09
#define SS_RDALTZP      0x16

struct page_block_t *soft_switch_pb;

BYTE zp_soft_switch(BYTE switch_no, bool read, BYTE value)
{
    struct page_block_t *pb = page_block[0];

    if (SS_SETSTDZP == switch_no)
        pb->buffer = norm_zp_buf;
    else if(SS_SETALTZP == switch_no)
        pb->buffer = alt_zp_buf;
    else if (SS_RDALTZP == switch_no)
        value = (pb->buffer == alt_zp_buf)? 0x80 : 0x00;

    return value;
}

struct soft_switch_t soft_switch[0x100];

BYTE soft_switch_accessor(WORD address, bool read, BYTE value)
{
    BYTE switch_no = address & 0x00FF;

    if (read && NULL != soft_switch[switch_no].read_accessor)
        value = soft_switch[switch_no].read_accessor(switch_no, read, value);
    else if (!read && NULL != soft_switch[switch_no].write_accessor)
        soft_switch[switch_no].write_accessor(switch_no, read, value);

    return value;
}


void init_soft_switches()
{
    memset(soft_switch, 0, sizeof(soft_switch));
    soft_switch_pb = create_page_block(0xC0, 1);
    soft_switch_pb->accessor = soft_switch_accessor;
    install_page_block(soft_switch_pb);

}

void install_soft_switch(BYTE switch_no, int switch_type, 
    soft_switch_accessor_t accessor)
{
    if (switch_type & SS_READ)
        soft_switch[switch_no].read_accessor = accessor;

    if (switch_type & SS_WRITE)
        soft_switch[switch_no].write_accessor = accessor;
}

BYTE my_accessor(WORD address, bool read, BYTE value)
{
    return hi(address);
}

BYTE output_soft_switch(BYTE switch_no, bool read, BYTE value)
{
    static char buf[256];
    static char *buf_ptr = buf;
  
    if (value)
        buf_ptr += sprintf(buf_ptr, "%c", value);

    if ('\n' == value || (buf_ptr - buf) > 254) { 
        shell_print(buf);
        buf_ptr = buf;
    }

    return value;
}

int anonymous_command(int argc, char **argv)
{
    int matches;
    int value, value2;
    char c;
    command_func_t peek_function = shell_get_command_function("peek");
    command_func_t poke_function = shell_get_command_function("poke");
   
    matches = sscanf(argv[1], "%04x%c", &value, &c);
    if (matches == 2 && c == ':' && NULL != poke_function) {
        poke_function(argc, argv);
    } else {
        matches = sscanf(argv[1], "%04x%c%04x", &value,
        &c, &value2);
        if (((2 <= matches && '-' == c) || (1 == matches)) &&
            (NULL != peek_function))
            peek_function(argc, argv);
        else
            printf("Unknown command \"%s\".\n", argv[1]);
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct page_block_t *my_pb;
    struct page_block_t *zpage_pb;
    
    printf("Mark II Ready\n\n");

    bus_init();
    init_soft_switches();

    my_pb = create_page_block(0, 8);
    my_pb->buffer = create_page_buffer(my_pb->total_pages);
    my_pb->accessor = RAM_accessor;
    install_page_block(my_pb);
    
    zpage_pb = create_page_block(0, 2);
    install_page_block(zpage_pb);
    alt_zp_buf = create_page_buffer(2);
    norm_zp_buf = zpage_pb->buffer;
    install_soft_switch(SS_SETSTDZP, SS_WRITE, zp_soft_switch);
    install_soft_switch(SS_SETALTZP, SS_WRITE, zp_soft_switch);
    install_soft_switch(SS_RDALTZP, SS_READ, zp_soft_switch);

    install_soft_switch(0x2A, SS_WRITE, output_soft_switch);

    shell_set_accessor(bus_accessor);
    shell_set_loop_cb(cycle);
    shell_set_anonymous_command_function(anonymous_command);
    shell_initialize("MarkII");
    cpu_shell_load_commands();
    cpu_init(bus_accessor);
    cpu_set_signal(SIG_HALT);
    shell_loop();
    shell_finalize();
    printf("\nMark II Done\n");
    return 0;
}
