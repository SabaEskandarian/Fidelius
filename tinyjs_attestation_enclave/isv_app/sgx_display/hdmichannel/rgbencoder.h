#ifndef RGB_ENCODER_H
#define RGB_ENCODER_H

void encode_rgb(char *dst, char *src, int len);
void decode_rgb(char *dst, char *src, int len);
void rgb_to_rgba(char *dst, char *src, int size);

#endif // RGB_ENCODER_H
