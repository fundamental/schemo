#pragma once

namespace draw
{
//Variable Size Routines (1)
void grid(int x, int y, int w, int h);
void capped(int x, int y, int w, int h);
void capped_label(int x, int y, int w, int h,
                  const char *label);

//Fixed Size (2)
void sigma(int x, int y);

//Widget Specific (3)
void slider(int x,int y, int w, int active);

void sinplot(int x, int y, int w, int h);
void sinplot2(int x, int y, int w, int h);
void plot(int x, int y, int w, int h, float *smps);

//PreRendered Elements (4)
void schematic(void);
void env(int x, int y);
}
