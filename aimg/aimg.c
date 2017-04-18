#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BOOT_MAGIC_SIZE     8
#define BOOT_NAME_SIZE      16 
#define BOOT_ARGS_SIZE      512

#define PHYSICAL_DRAM_BASE   0x00200000
#define KERNEL_ADDR          (PHYSICAL_DRAM_BASE + 0x00008000)
#define RAMDISK_ADDR         (PHYSICAL_DRAM_BASE + 0x01000000)
#define TAGS_ADDR            (PHYSICAL_DRAM_BASE + 0x00000100)
#define NEWTAGS_ADDR         (PHYSICAL_DRAM_BASE + 0x00004000)

struct boot_img_hdr{
    unsigned char magic[BOOT_MAGIC_SIZE];
    unsigned  kernel_size;
    unsigned  kernel_addr;
    unsigned  ramdisk_size;
    unsigned  ramdisk_addr;
    unsigned  second_size;
    unsigned  second_addr;
    unsigned  tags_addr;
    unsigned  page_size;
    unsigned  unused[2];
    unsigned  char  name[BOOT_NAME_SIZE];
    unsigned  char cmdline[BOOT_ARGS_SIZE];
    unsigned  id[8]; //存放时间戳，校验和，SHA加密等内容
} __attribute__((packed));

static void dump_chars(void* p, int l)  {
    char* c = (char*)p;
    int i;
    for (i = 0; i < l && *c; ++i) {
        printf("%c", *c++);
    }
}

static unsigned ceil_page_size(unsigned sz, struct boot_img_hdr* h) {
    div_t d = div(sz, h->page_size);
    if (d.rem) {
        d.quot += 1;
    }
    return d.quot * h->page_size;
}

unsigned kernel_offset(struct boot_img_hdr* h) {
    if (!h) {
        return -1;
    }
    return h->page_size;
}

unsigned ramdisk_offset(struct boot_img_hdr* h) {
    return ceil_page_size(h->kernel_size, h) + kernel_offset(h);
}



unsigned second_offset(struct boot_img_hdr* h) {
    const unsigned page_size = h->page_size;
    return ceil_page_size(h->ramdisk_size, h) + ramdisk_offset(h);
}

static void dump_ih(struct boot_img_hdr* h) {
    int i; 
    if (!h) {
        return;
    }
    printf("magic:\t");
    dump_chars(h->magic, BOOT_MAGIC_SIZE);
    printf("\n");
    printf("kernel_size:\t0x%08x(dec:%u)\n", h->kernel_size, h->kernel_size);
    printf("kernel_addr:\t0x%08x(dec:%u)\n", h->kernel_addr, h->kernel_addr);
    printf("ramdisk_size:\t0x%08x(dec:%u)\n", h->ramdisk_size, h->ramdisk_size);
    printf("ramdisk_addr:\t0x%08x(dec:%u)\n", h->ramdisk_addr, h->ramdisk_addr);
    printf("second_size:\t0x%08x(dec:%u)\n", h->second_size, h->second_size);
    printf("second_addr:\t0x%08x(dec:%u)\n", h->second_addr, h->second_addr);
    printf("tags_addr:\t0x%08x(dec:%u)\n", h->tags_addr, h->tags_addr);
    printf("page_size:\t0x%08x(dec:%u)\n", h->page_size, h->page_size);
    printf("unused:\t\t0x%08x(dec:%u),0x%08x(dec:%u)\n",
            h->unused[0], h->unused[0], 
            h->unused[1], h->unused[1]);
    printf("name:\t");
    dump_chars(h->name, BOOT_NAME_SIZE);
    printf("\n");
    printf("cmdline:\t");
    dump_chars(h->cmdline, BOOT_ARGS_SIZE);
    printf("\n");
    printf("id:\n[\n");
    for(i = 0; i < 8; ++i) {
        printf("\t0x%08x(dec:%u)\n", h->id[i], h->id[i]);
    }
    printf("]\n");
}

int dump_to(const char* path_to_save, unsigned offset, unsigned size, FILE* f) {
    FILE* out;
    char buf[4096];
    int readbytes;
    if (!f) {
        return -1;
    }
    if (fseek(f, offset, SEEK_SET) != 0) {
        return -1;
    }
    
    out = fopen(path_to_save, "wb");
    if (!out) {
        return -1;
    }
     
    while((readbytes=fread(buf, 1, sizeof(buf), f)) > 0) {
        fwrite(buf, 1, readbytes, out);
    }
    fclose(out);
    return 0;
}

int main(int argc, char* argv[]) {
    FILE* f = fopen(argv[1], "rb+");
    struct boot_img_hdr ih;
    unsigned t; 
    if (!f) {
        printf("%s open failed", argv[1]);
        return -1;
    }
    fread(&ih, 1, sizeof(ih), f);
    dump_ih(&ih);
    t =  kernel_offset(&ih);
    printf("kernel_offset:\t%08x(dec:%u)\n", t, t);
    dump_to("zImage", t, ih.kernel_size, f);
  
    t = ramdisk_offset(&ih);
    printf("ramdisk_offset:\t%08x(dec:%u)\n", t, t);
    dump_to("ramdisk.cpio.gz", t,  ih.ramdisk_size, f);
   
    t = second_offset(&ih);
    printf("second_offset:\t%08x(dec:%u)\n", t, t);
    dump_to("second_image.cpio.gz", t, ih.second_size, f);
    fclose(f);

    return 0;
}
