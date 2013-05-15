#ifndef _BLKDEV_H
#define _BLKDEV_H

typedef struct block_dev_desc {
    int     dev;        /* device number */
//    unsigned char   part_type;  /* partition type */

    unsigned long   (*block_read)(int dev,
                      unsigned long start,
                      unsigned long blkcnt,
                      void *buffer);

}block_dev_desc_t;

#endif	/* _BLKDEV_H */
