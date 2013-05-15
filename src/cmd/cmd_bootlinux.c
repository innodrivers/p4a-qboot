/**
 * @file
 * @brief	boot linux helper
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>

struct tag_header {
	uint32_t	size;
	uint32_t	tag;
};

/* The list must start with an ATAG_CORE node */
#define ATAG_CORE		0x54410001

struct tag_core {
	uint32_t	flags;		/* bit 0 = read-only */
	uint32_t	pagesize;
	uint32_t	rootdev;
};

/* it is allowed to have multiple ATAG_MEM node */
#define ATAG_MEM		0x54410002

struct tag_mem32 {
	uint32_t	size;
	uint32_t	start;	/* physical start address */
};

/* describes how the ramdisk will be used in kernel */
#define ATAG_RAMDISK	0x54410004

struct tag_ramdisk {
	uint32_t	flags;		/* bit 0 = load, bit 1 = prompt */
	uint32_t	size;		/* decompressed ramdisk size in kilo bytes */
	uint32_t	start;		/* starting block of floppy-base RAM disk image */
};

/* describes where the compressed ramdisk image lives (virtual address) */
#define ATAG_INITRD		0x54410005

/* describes where the compressed ramdisk image lives (physical address) */
#define ATAG_INITRD2	0x54420005

struct tag_initrd {
	uint32_t	start;		/* physical start address */
	uint32_t	size;		/* size of compressed ramdisk image in bytes */
};

/* board serial number */
#define ATAG_SERIAL		0x54410006

struct tag_serialnr {
	uint32_t	low;
	uint32_t	high;
};

/* board revision */
#define ATAG_REVISION	0x54410007

struct tag_revision {
	uint32_t	rev;
};

/* command line: \0 terminated string */
#define ATAG_CMDLINE	0x54410009

struct tag_cmdline {
	char	cmdline[1];		/* this is the minimun size */
};

#define ATAG_NONE		0x00000000

struct tag {
	struct tag_header hdr;
	union {
		struct tag_core		core;
		struct tag_cmdline	cmdline;
		struct tag_mem32	mem;
	}u;
};

#define tag_next(t)		((struct tag*)((uint32_t*)(t) + (t)->hdr.size))
#define tag_size(type)	((sizeof(struct tag_header) + sizeof(struct type)) >> 2)



static struct tag* atag_core_setup(void* tag_base)
{
	struct tag  *cur_tag = (struct tag*)tag_base;
	
	cur_tag->hdr.tag = ATAG_CORE;
	cur_tag->hdr.size = tag_size(tag_core);
	
	cur_tag->u.core.flags = 0;
	cur_tag->u.core.pagesize = 0;
	cur_tag->u.core.rootdev = 0;
	
	return cur_tag;
}

static struct tag* atag_cmdline_setup(struct tag* cur_tag, char* cmdline)
{
	while (*cmdline && ' '==*cmdline)	//remove the space of beginning
		cmdline++;
	
	cur_tag->hdr.tag = ATAG_CMDLINE;
	cur_tag->hdr.size = (sizeof(struct tag_header) + strlen(cmdline) + 3) >> 2;
	
	strcpy(cur_tag->u.cmdline.cmdline, cmdline);
	
	return cur_tag;
}

static struct tag* atag_mem_setup(struct tag* cur_tag, uint32_t mem_phys_start, uint32_t mem_phys_size)
{	
	cur_tag->hdr.tag = ATAG_MEM;
	cur_tag->hdr.size = tag_size(tag_mem32);
	
	cur_tag->u.mem.start = mem_phys_start;
	cur_tag->u.mem.size = mem_phys_size;
	
	return cur_tag;
}

static struct tag* atag_none_setup(struct tag* cur_tag)
{
	cur_tag->hdr.tag = ATAG_NONE;
	cur_tag->hdr.size = 0;
	
	return cur_tag;
}
#if 1
#define DEFAULT_ATAG_BASE		0x40000100
#define DEFAULT_KERNEL_BASE		0x40008000
#define DEFAULT_MACHINE_ID		3300	 //3339//2695
#define DEFAULT_CMDLINE_STRING	"console=ttyS1,115200 init=/linuxrc mem=96M initrd=0x41000000,1998661 root=/dev/ram0,rw rdinit=/linuxrc"
//#define DEFAULT_CMDLINE_STRING 	"initrd=0x00800000,3845544 root=/dev/ram0,rw rdinit=/linuxrc console=ttyS0,115200n8 mem=64M"
//#define DEFAULT_CMDLINE_STRING	"root=/dev/nfs rw nfsroot=172.16.8.100:/nfsboot/buildroot,proto=tcp,nolock ip=dhcp init=/linuxrc console=ttyS0,115200n8 mem=64M"
#else
#define DEFAULT_ATAG_BASE		0xe0000100
#define DEFAULT_KERNEL_BASE		0xe0008000
#define DEFAULT_MACHINE_ID		3339
#define DEFAULT_CMDLINE_STRING	"initrd=0xe0800000,3845544 root=/dev/ram0,rw rdinit=/linuxrc console=ttyS0,115200n8 mem=64M"
//#define DEFAULT_CMDLINE_STRING	"root=/dev/nfs rw nfsroot=172.16.8.100:/nfsboot/buildroot,proto=tcp,nolock ip=dhcp init=/linuxrc console=ttyS0,115200n8 mem=64M"
#endif
static unsigned long	__atag_base = DEFAULT_ATAG_BASE;
static unsigned long	__kernel_base = DEFAULT_KERNEL_BASE;
static unsigned long	__machine_id = DEFAULT_MACHINE_ID;
static char __cmdline_str[256] = DEFAULT_CMDLINE_STRING;

/*
 * "nfsroot=" format:
 *
 * nfsroot=[<server-ip>:]<root-dir>[,<nfs-options>]
 *
 * "ip=" format:
 *
 * ip=<client-ip>:<server-ip>:<gw-ip>:<netmask>:<hostname>:<device>:<autoconf>
 */

typedef void (*start_kernel_fn)(int zero, uint32_t machid, uint32_t params);

static int show_help(void)
{
	PRINT("linux [-m machine_id] [-a atag_base] [-c cmdline_str] [-k kernel_base] [-b]\n");
	PRINT("\t-m Machine ID\n"
		"\t-a ATAG Address\n"
		"\t-c Command Line String\n"
		"\t-k Kernel Address\n"
		"\t-b Boot the Kernel\n");
	return 0;
}

static int cmd_bootlinux(int argc, char** argv)
{
	struct tag* tag;
	int i = 1;
	int boot = 0;

	if (argc == 1)
		return show_help();

	while (i < argc) {
		if (strcmp(argv[i], "-m") == 0 && ++i < argc) {
			__machine_id = simple_strtoul(argv[i++], NULL, 0);
			continue;

		} else if (strcmp(argv[i], "-a") == 0 && ++i < argc) {
			__atag_base = simple_strtoul(argv[i++], NULL, 0);
			continue;

		} else if (strcmp(argv[i], "-k") == 0 && ++i < argc) {
			__kernel_base = simple_strtoul(argv[i++], NULL, 0);
			continue;

		} else if (strcmp(argv[i], "-c") == 0 && ++i < argc) {
			char* p = __cmdline_str;
			int chars = 0;

			while(argv[i][0] != '-' && ((sizeof(__cmdline_str) - chars) > 0)) {
				int len = strlen(argv[i]);
				strncpy(p + chars, argv[i], sizeof(__cmdline_str) - chars);
				chars += len;
				if (++i >= argc)
					break;
				p[chars++] = ' ';
			}
			p[chars] = '\0';
			continue;

		} else if (strcmp(argv[i], "-b") == 0) {
			boot = 1;

		} else if (strcmp(argv[i], "-h") == 0) {
			return show_help();
		}
		i++;
	}

	if (boot) {
		//PRINT("%s\n", __cmdline_str);
		tag = atag_core_setup((void*)__atag_base);
		tag = atag_cmdline_setup(tag_next(tag), __cmdline_str);
		tag = atag_none_setup(tag_next(tag));
		PRINT("booting linux kernel..\n");	
		((start_kernel_fn)__kernel_base)(0, __machine_id, __atag_base);
	}

	return 0;
}

INSTALL_CMD(
	linux, 
	cmd_bootlinux, 
	"boot linux with parameters"
	);

