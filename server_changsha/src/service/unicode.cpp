

#include "unicode.h"
////////////////////////////////////////////////////////////////////////////////
namespace unicode{
////////////////////////////////////////////////////////////////////////////////
static int utf8_size(const unsigned char c)
{
    if (c < 0x80) return 0;
    if (c >=0x80 && c <0xC0) return -1;
    if (c >=0xC0 && c <0xE0) return 2;
    if (c >=0xE0 && c <0xF0) return 3;
    if (c >=0xF0 && c <0xF8) return 4;
    if (c >=0xF8 && c <0xFC) return 5;
    return 6;
}
////////////////////////////////////////////////////////////////////////////////
int from_utf8_char(const unsigned char *input, unsigned long *output)
{
    // b1 表示UTF-8编码的input中的高字节, b2 表示次高字节, ...
    char b1, b2, b3, b4, b5, b6;
    *output = 0x0; // 把 *output 初始化为全零
    int utfbytes = utf8_size(*input);
    unsigned char *pOutput = (unsigned char *) output;

    switch ( utfbytes ){
    case 0:
        *pOutput = *input;
        utfbytes += 1;
        break;
    case 2:
        b1 = *input;
        b2 = *(input + 1);
        if ( (b2 & 0xE0) != 0x80 )
            return 0;
        *pOutput = (b1 << 6) + (b2 & 0x3F);
        *(pOutput+1) = (b1 >> 2) & 0x07;
        break;
    case 3:
        b1 = *input;
        b2 = *(input + 1);
        b3 = *(input + 2);
        if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) )
            return 0;
        *pOutput = (b2 << 6) + (b3 & 0x3F);
        *(pOutput+1) = (b1 << 4) + ((b2 >> 2) & 0x0F);
        break;
    case 4:
        b1 = *input;
        b2 = *(input + 1);
        b3 = *(input + 2);
        b4 = *(input + 3);
        if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
            || ((b4 & 0xC0) != 0x80) )
            return 0;
        *pOutput = (b3 << 6) + (b4 & 0x3F);
        *(pOutput+1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
        *(pOutput+2) = ((b1 << 2) & 0x1C) + ((b2 >> 4) & 0x03);
        break;
    case 5:
        b1 = *input;
        b2 = *(input + 1);
        b3 = *(input + 2);
        b4 = *(input + 3);
        b5 = *(input + 4);
        if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
            || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) )
            return 0;
        *pOutput = (b4 << 6) + (b5 & 0x3F);
        *(pOutput+1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
        *(pOutput+2) = (b2 << 2) + ((b3 >> 4) & 0x03);
        *(pOutput+3) = (b1 << 6);
        break;
    case 6:
        b1 = *input;
        b2 = *(input + 1);
        b3 = *(input + 2);
        b4 = *(input + 3);
        b5 = *(input + 4);
        b6 = *(input + 5);
        if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
            || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80)
            || ((b6 & 0xC0) != 0x80) )
            return 0;
        *pOutput = (b5 << 6) + (b6 & 0x3F);
        *(pOutput+1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
        *(pOutput+2) = (b3 << 2) + ((b4 >> 4) & 0x03);
        *(pOutput+3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
        break;
    default:
        return 0;
        break;
    }
    return utfbytes;
}
////////////////////////////////////////////////////////////////////////////////
} //End of namespace unicode
////////////////////////////////////////////////////////////////////////////////
