#ifndef _BYTE_ORDER_H
#define _BYTE_ORDER_H

#define CONFIG_LITTLE_ENDIAN

#define __swab16(x) \
    ((__u16)( \
        (((__u16)(x) & (__u16)0x00ffU) << 8) | \
        (((__u16)(x) & (__u16)0xff00U) >> 8) ))
#define __swab32(x) \
    ((__u32)( \
        (((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
        (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
        (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
        (((__u32)(x) & (__u32)0xff000000UL) >> 24) ))


#ifdef CONFIG_LITTLE_ENDIAN
/*
 * Little Endian
 */
#define __cpu_to_le32(x) ((__u32)(x))
#define __le32_to_cpu(x) ((__u32)(x))
#define __cpu_to_le16(x) ((__u16)(x))
#define __le16_to_cpu(x) ((__u16)(x))

#define __cpu_to_be32(x) __swab32((x))
#define __be32_to_cpu(x) __swab32((x))
#define __cpu_to_be16(x) __swab16((x))
#define __be16_to_cpu(x) __swab16((x))

#else
/*
 * Big Endian
 */
#define __cpu_to_le32(x) __swab32((x))
#define __le32_to_cpu(x) __swab32((x))
#define __cpu_to_le16(x) __swab16((x))
#define __le16_to_cpu(x) __swab16((x))

#define __cpu_to_be32(x) ((__u32)(x))
#define __be32_to_cpu(x) ((__u32)(x))
#define __cpu_to_be16(x) ((__u16)(x))
#define __be16_to_cpu(x) ((__u16)(x))
#endif

#define cpu_to_le32 __cpu_to_le32
#define le32_to_cpu __le32_to_cpu
#define cpu_to_le16 __cpu_to_le16
#define le16_to_cpu __le16_to_cpu
#define cpu_to_be32 __cpu_to_be32
#define be32_to_cpu __be32_to_cpu
#define cpu_to_be16 __cpu_to_be16
#define be16_to_cpu __be16_to_cpu

#endif	/* _BYTE_ORDER_H */
