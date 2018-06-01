#include "rgbencoder.h"
#include "xoverlay.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
  char *test = (char *)malloc(2000);
  char *rgb = (char *)malloc(3000);

  char *r = rgb;
  char *g = rgb+1;
  char *b = rgb+2;

  for (int i = 0; i < 100; i++){
	  *r = 255;
	  *g = 0;
	  *b = 0;
	  r += 3;
	  g += 3;
	  b += 3;
  }

  for (int i = 0; i < 100; i++){
	  *r = 0;
	  *g = 0;
	  *b = 255;
	  r += 3;
	  g += 3;
	  b += 3;
  }

  for (int i = 0; i < 100; i++){
	  *r = 0;
	  *g = 255;
	  *b = 0;
	  r += 3;
	  g += 3;
	  b += 3;
  }

  init_overlays();
  add_overlay(rgb, 100, 3, 0, 0);
  while(1)
  {
    draw_overlays();
  }
}
