#ifndef _IO_H
#define _IO_H

#define __iomem

static __INLINE__ uint8_t __raw_readb(const void __iomem *addr)
{
    return (*((volatile uint8_t *) (addr)));
}

static __INLINE__ uint16_t __raw_readw(const void __iomem *addr)
{
    return (*((volatile uint16_t *) (addr)));
}

static __INLINE__ uint32_t __raw_readl(const void __iomem *addr)
{
    return (*((volatile uint32_t *) (addr)));
}

static __INLINE__ void __raw_writeb(uint8_t b, void __iomem *addr)
{
	*((volatile uint8_t  *) (addr)) = ((uint8_t)(b));
}

static __INLINE__ void __raw_writew(uint16_t b, void __iomem *addr)
{
	*((volatile uint16_t *) (addr)) = ((uint16_t)(b));
}

static __INLINE__ void __raw_writel(uint32_t b, void __iomem *addr)
{
	*((volatile uint32_t   *) (addr)) = ((uint32_t)(b));
}

static __INLINE__ void SET_REG(unsigned int addr, uint32_t val)
{
	__raw_writel(val, (void __iomem*)addr);
}

static __INLINE__ uint32_t GET_REG(unsigned int addr)
{
	return __raw_readl((void __iomem*)addr);
}

static __INLINE__ void CLEAR_REG_BITS(unsigned int addr, uint32_t bitmask)
{   
    __raw_writel(__raw_readl((void __iomem*)addr) & ~bitmask, (void __iomem*)addr);
}   
	    
static __INLINE__ void SET_REG_BITS(unsigned int addr, uint32_t bitmask)
{   
	__raw_writel(__raw_readl((void __iomem*)addr) | bitmask, (void __iomem*)addr);
}

#define REG32(addr) ((volatile uint32_t *)(addr))
#define REG16(addr) ((volatile uint16_t *)(addr))
#define REG8(addr) ((volatile uint8_t *)(addr))

#endif	/* _IO_H */
