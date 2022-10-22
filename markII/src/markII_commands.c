#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <shell.h>
#include <6502_shell.h>
#include <SDL2/SDL.h>
#include <Stuff.h>
#include "logging.h"
#include "bus.h"
#include "cpu_iface.h"
#include "disk.h"
#include "dos33.h"

static bool watching = false;
static Uint32 prev_time = 0;
static int total_clocks;
static char buf[256];

/**
 * clocks
 */
static void watch_clock(BYTE clocks)
{
    Uint32 timer = SDL_GetTicks();
    Uint32 timer_diff = timer - prev_time;

    
    total_clocks += clocks;

    if (timer_diff >= 1000) {
        prev_time = timer;   
        if (watching) {
            sprintf(buf, "Clocks this second: %d\n", total_clocks);
            shell_print(buf);
        }
        total_clocks = 0;
    }
}

static int watch_clocks(int argc, char **argv)
{
    printf("Watch clocks...\n");
    watching = !watching;
    return 0;
}

/**
 * reboot
 */

static bool rebooting = false;

static int reboot(int argc, char **argv)
{
    LOG_INF("Rebooting...\n");
    rebooting = true;
    return -1;
}

bool is_rebooting()
{
    bool tmp = rebooting;
    rebooting = false;
    return tmp;
}

/**
 * mount
 */
BYTE dos33_order[] = {
    0x00, 0x7D, 0xEB, 0x69, 0xD7, 0x55, 0xC3, 0x41,
    0xBE, 0x3C, 0xAA, 0x28, 0x96, 0x14, 0x82, 0xFF
};


BYTE prodos_order[] = {
    0x00, 0x82, 0x14, 0x96, 0x28, 0xAA, 0x3C, 0xBE,
    0x41, 0xC3, 0x55, 0xD7, 0x69, 0xEB, 0x7D, 0xFF
};
/*
BYTE prodos_order[] = {
    0x00, 0x02, 0x08, 0x0c, 0x01, 0x05, 0x09, 0x0d,
    0x02, 0x06, 0x0a, 0x0e, 0x03, 0x07, 0x0b, 0x0F
};*/
void calibrate_rw_head(struct drive_t* drive)
{
    int i;

    for (i = 0; i < 80; i++)
        move_rw_head(drive, false);
}

bool dsk_to_map(struct drive_t *drive, char *file_name, int order)
{
    BYTE *sector_order = (order == 1)? prodos_order : dos33_order;
    int i;
    int j;
    int fd;
    size_t count;
    BYTE physical_place;

    fd = open(file_name, O_RDONLY, NULL);
    if (fd == -1) {
        LOG_ERR("Error opening dsk.\n");
        return false;
    } 
      
    LOG_INF("Writting map...\n");
    calibrate_rw_head(drive);

    for(i = 0; i < 35; ++i) {
        move_rw_head(drive, true);

        for (j = 0; j < 16; ++j) {
            physical_place = sector_order[j] & 0x0F; //>> 4;

            count = read(fd, buf, 256);
            if (count < 256) {
                LOG_ERR("Error reading dsk.\n");
                close(fd);
                return false;
            }
           
            //dump_buffer(buf, 256);
            seek_sector(drive, physical_place & 0x0F);
            write_sector(drive, buf);
            write_sync_bytes(drive, 5);
        }      

        for (j = 0; j < 8; ++j)
            read_byte(drive);
        move_rw_head(drive, true);
    }

    close(fd);
    return true;
}

void mount_disk(char *file_name, int drive_no, int order,
    bool write_protected)
{
    BYTE buf[256];
    struct drive_t drive;
    struct drive_t *dest_drive;
    struct map_header_t *header;
    char map_filename[32];

    memset(buf, 0, 256);
    //6395
    drive.empty = true;
    dest_drive = get_drive(drive_no - 1);
    unload_disk(dest_drive);

    init_steppers(&drive);

    drive.verbose = true;
    sprintf(map_filename, "disk%d.map", drive_no);
    create_disk(&drive, 35, 6500, map_filename);

    //load_disk(&drive, map_filename);
    header = (struct map_header_t *) drive.map->header;
    strncpy(header->filename, file_name, 255);
    header->write_protected = write_protected;
    header->valid = true;
    drive.empty = false;

    format_disk(&drive, 254);
    if (dsk_to_map(&drive, file_name, order)) {    
        header->valid = true;
        header->hash = hash_fnv1a_32((char *) drive.map->data, 
            (int) drive.map_size);
    } else
        header->valid = false;

    unload_disk(&drive);
    init_drive(drive_no - 1);
    load_disk(dest_drive, map_filename);
}

void unmount_disk(char *file_name, int drive_no, BYTE sector_order[])
{
    BYTE buf[256];
    char map_filename[32];
    int fd;
    int i;
    int j;
    BYTE physical_place;
    struct drive_t drive; 
    struct drive_t *mounted = get_drive(0);
    struct map_header_t *header = (struct map_header_t *) mounted->map->header;
    bool modified;
    bool write_protected;
    
    memset(buf, 0, 256);
   
    sprintf(map_filename, "disk%d.map", drive_no);

    if (file_name == 0)
        file_name = header->filename;

    write_protected = header->write_protected;
    if (!write_protected)
        modified = (hash_fnv1a_32((char *) mounted->map->data,  
            (int) mounted->map_size) != header->hash);

    LOG_INF("Unmounting disk %s...\n", file_name);
    unload_disk(get_drive(drive_no - 1));
   
    if (write_protected) {
        LOG_INF("Write protected.\n");
        return;
    }
         
    if (!modified) {
        LOG_INF("Not modified.\n");
        return;
    }

    init_steppers(&drive);
    load_disk(&drive, map_filename);

    fd = creat(file_name, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("Error creating dsk.\n");
        return;
    }

    calibrate_rw_head(&drive);
    for (i = 0; i < 35; ++i) {
        move_rw_head(&drive, true);

        for (j = 0; j < 16; ++j) {
            physical_place = sector_order[j] & 0x0F;
            seek_sector(&drive, physical_place);
            read_sector(&drive, buf);
            write(fd, buf, 256);
        }

        move_rw_head(&drive, true);
    }

    close(fd);
    unload_disk(&drive);
}
char *mnt_file;
char *unmnt_file;
bool show_help;
bool run_test_main;
int drive_no;
char *sector_order;
bool write_protected;
#define omitted  "`" 

struct parameter_struct param[] = {
    PARAM("drive", 'd', 'i', &drive_no, 1, "Select drive (1 or 2)")
    PARAM("help", 'h', 'b', &show_help, FALSE, "Show this help and exit")
    PARAM("order", 'o', 'c', &sector_order, "D", 
        "Sector order - D = DOS 3.3, P = ProDOS")
    PARAM("write-protected", 'w', 'b', &write_protected, FALSE, 
        "Disk is write protected.")
};

int mount_command(int argc, char **argv)
{
    struct param_parse_struct *param_parse =
        parse_command_line(param, ARRAY_SIZE(param), argc, argv);

    char *map_filename[] = {"disk1.map", "disk2.map"};

    LOG_INF("Drive = %d, file_name = %s\n", drive_no, 
        map_filename[drive_no - 1]);
    
    if (show_help) {
        list_params(param_parse);
        shell_print("\n");
        return 0;
    }

    mnt_file = get_next_argument(param_parse);

    if (0 == strcmp("mount",argv[0])) {
        if (mnt_file == 0) {
            LOG_ERR("Must specifiy a file name.\n");
        } else {
            LOG_INF("Mounting disk file %s in %s order (%c), "
                " %swrite protected...\n", mnt_file,
                *sector_order == 'D'? "DOS 3.3" :  "ProDOS", *sector_order,
                write_protected? "" : "not ");
            mount_disk(mnt_file, drive_no, 
                *sector_order == 'D'? 0 : 1, 
                write_protected);
        }
    } else {
        unmount_disk(mnt_file, drive_no, 
            *sector_order == 'D'? dos33_order : prodos_order);
 
    }
    
   
   /*else if (unmnt_file != 0  && *unmnt_file != '`' ) {
       printf("Unmounting disk");
       if (unmnt_file != 0)
           printf(" to %s", unmnt_file);
       printf("...\n");
       unmount_disk(unmnt_file, map_filename[drive_no - 1],
                sector_order == 'D'? dos33_order : prodos_order);
   }
 */
    clean_up_params(param_parse);

    return 0;
}

int mounted_command(int argc, char **argv)
{
    struct drive_t *drive;
    struct map_header_t *header;
    unsigned int hash;
    int i;

    for (i = 0; i < 2; ++i) {
        drive = get_drive(i);
        if (drive->empty) {
            printf("Drive %d: Not mounted.\n", i + 1);
            continue;
        }
    
        header = (struct map_header_t *) drive->map->header;
        printf("Drive %d: File = %s, %swrite protected\n", i + 1, 
            header->filename, header->write_protected? "" : "not ");
        hash = hash_fnv1a_32((char *) drive->map->data, (int) drive->map_size);
        printf("Map size = %d, hash = \%x, header hash = %x\n", 
            drive->map_size, hash, header->hash);
    }

    return 0;
}

void init_mount_command()
{
    init_denibble();
    shell_add_command("mount", "Mount disk.", mount_command, false);
    shell_add_command("unmount", "Unmount disk.", mount_command, false);
    shell_add_command("mounted", "List mounted disks.", 
        mounted_command, false);
}

/**
 * add commands
 */
void add_shell_commands()
{
    shell_add_command("clocks", "Watch clocks per second.", 
        watch_clocks, false);
    add_device(watch_clock);

    shell_add_command("reboot", "Reboot MarkII.", reboot, false);
    init_mount_command();
}

