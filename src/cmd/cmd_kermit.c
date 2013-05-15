/**
 * @file
 * @brief	kermit
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>

#define MARK 0x01
#define TOCHAR(a)   ((a) + 32)
#define UNCHAR(a)   ((a) - 32)
#define CTRL(a)     ((a) ^ 64)

typedef struct KPACKET_tag
{
	uint8_t data_len;
	uint8_t seq;
	uint8_t type;
	uint8_t check;
	uint8_t data[94];
	uint8_t filler[2];
} KPACKET_t;

typedef int (*KHANDLER_t)(KPACKET_t *);

static int kermit_rf(KPACKET_t *pPacket);
static int kermit_r(KPACKET_t *pPacket);
static int kermit_rd(KPACKET_t *pPacket);

static KHANDLER_t kermit_handler;

static uint8_t kermit_seq;
static int kermit_size;
static uint8_t* kermit_addr;
static uint8_t remote_eof;


static int kermit_send_ack(uint8_t type, uint8_t seq)
{
	uint8_t data[6];
	uint8_t checksum = 0;
	int i;

	data[0] = MARK;
	data[1] = TOCHAR(3);
	data[2] = TOCHAR(seq);
	data[3] = type;
	checksum = data[1] + data[2] + data[3];
	data[4] = (checksum + ((checksum >> 6) & 0x03)) & 63;
	data[4] = TOCHAR(data[4]);
	data[5] = remote_eof;

	for(i = 0; i < 6; ++i) {
		serial_putc(data[i]);
	}
}


static int kermit_getpacket(KPACKET_t *pPacket)
{
	uint8_t inputc;
	uint8_t checksum = 0;
	uint8_t packet_len;
	uint8_t data_cnt;
	int i;

	/* MARK */
	while(MARK != serial_getc());

	/* LEN */
	inputc = serial_getc();
	checksum += inputc;
	packet_len = UNCHAR(inputc);
	
	/* SEQ */
	inputc = serial_getc();
	checksum += inputc;
	pPacket->seq = UNCHAR(inputc);

	/* TYPE */
	inputc = serial_getc();
	pPacket->type = inputc;
	checksum += inputc;
			
	/* DATA */
	i = 0;
	data_cnt = 0;
	while(i < packet_len - 3) {
		inputc = serial_getc();
		checksum += inputc;
		++i;

		if('#' == inputc) {
			inputc = serial_getc();
			checksum += inputc;
			++i;
//  			if( 0x40 == (inputc & 0x60) || 0x3f == (inputc & 0x7f) )
//				inputc = CTRL(inputc);
			if (0x40 == (inputc & 0x60))
				inputc &= ~0x40;
			else if (0x3f == (inputc & 0x7f))
				inputc |= 0x40;
		}
		pPacket->data[data_cnt] = inputc;
		++data_cnt;
	}
	pPacket->data_len = data_cnt;

	/* CHECK */
	inputc = serial_getc();
	//checksum = (checksum + (checksum &(0xc0) >> 6)) & 63;
	checksum = (checksum + (checksum >> 6)) & 63;
	checksum = TOCHAR(checksum);
	if(inputc != checksum) {
		PRINT("checksum(%d) != inputc(%d)\n", checksum, inputc);
		return -1;
	}

	return 0;
}


static int kermit_r(KPACKET_t *pPacket)
{
	switch(pPacket->type) {
	case 'S':
		kermit_handler = kermit_rf;
		break;

	default:
		return -1;
		break;
	}

	return 0;
}

static int kermit_rf(KPACKET_t *pPacket)
{
	switch(pPacket->type)
	{
	/* FILE HEADER */
	case 'F':
		kermit_handler = kermit_rd;
		break;

	/* SEND INITIATION */
	case 'S':
		/* Nothing to do */
		break;

	/* EOF */
	case 'Z':
		/* Nothing to do */
		break;

	/* FILE ATTRIBUTE */
	case 'A':
		/* Nothing to do */
		break;

	/* BREAK */
	case 'B':
		/* Finished */
		return 1;
		break;

	default:
		return -2;
		break;
	}

	return 0;
}

static int kermit_rd(KPACKET_t *pPacket)
{
	int i;

	switch(pPacket->type) {
	case 'D':
		for(i = 0; i < pPacket->data_len; ++i) {
			kermit_addr[kermit_size] = pPacket->data[i];
			++kermit_size;
		}
		break;

	case 'Z':
		kermit_handler = kermit_rf;
		break;

	default:
		break;
	}

	return 0;
}


static int kermit_recv(uint8_t *file_addr)
{
	uint32_t         ret;
	KPACKET_t   packet;

	kermit_addr = file_addr;
	remote_eof = 0x0d;
	kermit_seq = 0xff;

	kermit_handler = kermit_r;
	kermit_size = 0;

	while(1) {
		ret = kermit_getpacket(&packet);
		if(ret || packet.seq > kermit_seq) {
			kermit_send_ack('N', kermit_seq);
			continue;
		}

		if(0xff == kermit_seq) {
			kermit_seq = packet.seq;
		}

		if(packet.seq > kermit_seq) {
			kermit_send_ack('N', kermit_seq);
			continue;
		}

		kermit_send_ack('Y', packet.seq);
   
		if(packet.seq < kermit_seq) {
			continue;
		}
		kermit_seq =  (kermit_seq + 1) & 63;

		if(!kermit_handler) {
			return -2;
		}
		ret = kermit_handler(&packet);
		if(ret < 0) {
			goto error;
		} else if(ret == 1)	{
			goto done;
		}
	}

done:
	return kermit_size;
error:
	return ret - 2;
}

static int cmd_loadb(int argc, char** argv)
{
	unsigned long address;
	void (*jump_to)(void);
	int ret;

	if (argc != 2) {
		PRINT("loadb <address>\n");
		return -1;
	}

	address = simple_strtoul(argv[1], NULL, 0);

	ret = kermit_recv((uint8_t*)address);
	if (ret > 0) {
		PRINT("%d bytes loaded!\n", ret);
	}

	return 0;
}

INSTALL_CMD(loadb, 	cmd_loadb, "load file via serial (kermit mode)");

