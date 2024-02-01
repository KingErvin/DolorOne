#ifndef __EZ_LCD_FONTS_H
#define __EZ_LCD_FONTS_H

#define TNEB_SIZE_32 32 
#define TNEB_SIZE_48 48/*字体尺寸*/
#define TNEB_SIZE_64 64/*字体尺寸*/

#define SONG_SIZE_16 16
#define SONG_SIZE_24 24

extern const unsigned char font_Transbold_Neue_Euro_Bold_1632[][TNEB_SIZE_32*TNEB_SIZE_32/8/2];
extern const unsigned char font_Transbold_Neue_Euro_Bold_2448[][TNEB_SIZE_48*TNEB_SIZE_48/8/2];
extern const unsigned char font_Transbold_Neue_Euro_Bold_3264[][TNEB_SIZE_64*TNEB_SIZE_64/8/2];

extern const unsigned char font_Song_0816[][SONG_SIZE_16*SONG_SIZE_16/8/2];
extern const unsigned char font_Song_1224[][SONG_SIZE_24*SONG_SIZE_24/8/2];

extern const char unit_hz[];
extern const char information_title[];
extern char information_hardware[];
extern char information_software[];
extern char information_instruction[];

#endif