#ifndef XOVERLAY_H
#define XOVERLAY_H

#include <X11/Xlib.h>

/* Stores the context of an individual overlay, manipulate using the
 * add_overlay, remove_overlay, refresh_overlays and set_position functions */
class Overlay {
  public:
    Overlay(char*, int, int, int, int);
    ~Overlay();
    void set_position(int, int);
    int width;                  /* Width of the overlay image */
    int height;                 /* Height of the overlay image */
    int xpos;                   /* X coordinate of top left of the overlay */
    int ypos;                   /* Y coordinate of the left of the overlay */
    char *data;
    XImage *img;
};

void init_overlays(void);
void draw_overlays(void);
void cleanup_overlays(void);
Overlay *add_overlay(char *rgb_array, int width, int height, int xpos, int ypos);
void remove_overlay(Overlay *o);

#endif // XOVERLAY
