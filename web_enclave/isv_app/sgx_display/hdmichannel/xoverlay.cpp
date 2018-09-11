#include "xoverlay.h"
#include "rgbencoder.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>

#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <algorithm>

/* XLib Variables */
static Display *d;
static Window overlay_window;
static GC gc;

/* List of all active overlays */
std::list<Overlay*> overlays;

Overlay::Overlay(char *rgb_array, int w, int h, int x, int y)
{
   width = w;
   height = h;
   xpos = x;
   ypos = y;

   data = (char*) malloc(sizeof(char) * w * h * 4);
   rgb_to_rgba(data, rgb_array, w*h*sizeof(char));

   XWindowAttributes attr;
   XGetWindowAttributes(d, overlay_window, &attr);

   /* d = Connection to X server
    * attr.visual = Visual Structure (Direct Color?)
    * 24 = Depth of image (3 colors, 1 byte each)
    * ZPixmap = data format is in RGB triplets
    * 0 = offset in data
    * rgba_array = image data
    * width = width of image in pixels
    * height = height of image in pixels
    * 32 = quantum of scanline, X seems to use a whole word for each pixel not
    *      24 bits.
    * width * 4 = bytes per row of the image */
   img = XCreateImage(d, attr.visual, 24, ZPixmap, 0, data, width, height, 32,
                      width * 4);
}

Overlay::~Overlay(void)
{
  XDestroyImage(img);
}

/* Sets the position of the overlay without reinitializing the image */
void Overlay::set_position(int x, int y)
{
  xpos = x;
  ypos = y;
}

/* Initializes the XLib context. Must be called before calling add_overlay */
void init_overlays(void)
{
  /* Initializing with NULL defaults to the value of the DISPLAY environment
   * variable */
  d = XOpenDisplay(NULL);
  int screen = DefaultScreen(d);
  Window root = RootWindow(d, screen);
  overlay_window = XCompositeGetOverlayWindow (d, root);
  gc = XCreateGC(d, overlay_window, 0, NULL);
}

/* Removes the overlay from the list of active overlays and destroys it */
void remove_overlay(Overlay *o)
{
  delete(o);
  overlays.remove(o);
}

/* Cleans up the active overlays and shuts down the XLib context */
void cleanup_overlays(void)
{
  std::list<Overlay*>::iterator it;
  for (it = overlays.begin(); it != overlays.end(); it++)
  {
    delete(*it);
  }
  overlays.clear();
  XCompositeReleaseOverlayWindow(d, overlay_window);
  XCloseDisplay(d);
}

/* Creates a new overlay and adds it to the list of active overlays.
 * rgb_array = RGB data stored in RGB888 format
 * width = width of overlay in pixels
 * height = height of overlay in pixels
 * xpos = x coordinate of the top left corner of the overlay
 * ypos = y coordinate of the top left corner of the overlay
 */
Overlay *add_overlay(char *rgb_array, int width, int height, int xpos, int ypos)
{
  Overlay *o = new Overlay(rgb_array, width, height, xpos, ypos);
  overlays.push_back(o);
  return o;
}

/* Refreshes the active overlays on the screen. Should be called as frequently
 * as possible */
void draw_overlays(void)
{
  std::list<Overlay*>::iterator it;
  for (it = overlays.begin(); it != overlays.end(); it++)
  {
    Overlay *o = *it;
    XPutImage(d, overlay_window, gc, o->img, 0, 0, o->xpos, o->ypos, o->width,
              o->height);
  }
}

