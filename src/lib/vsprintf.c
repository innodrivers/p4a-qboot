#include <common.h>
#include <types.h>

#define TOLOWER(x) ((x) | 0x20)

#define in_range(c, lo, up)	((unsigned char)c >= lo && (unsigned char)c <= up)
#define isascii(c)		in_range(c, 0x20, 0x7f)
#define isdigit(c)		in_range(c, '0', '9')
#define isxdigit(c)		(isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)		in_range(c, 'a', 'z')
#define isupper(c)		in_range(c, 'A', 'Z')

static unsigned int simple_guess_base(const char *cp)
{
    if (cp[0] == '0') {
        if (TOLOWER(cp[1]) == 'x' && isxdigit(cp[2]))
            return 16;
        else
            return 8;
    } else {
        return 10;
    }
}

unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
    unsigned long long result = 0;

    if (!base)
        base = simple_guess_base(cp);

    if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
        cp += 2;

    while (isxdigit(*cp)) {
        unsigned int value;

        value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
        if (value >= base)
            break;
        result = result * base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;

    return result;
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
    return simple_strtoull(cp, endp, base);
}

