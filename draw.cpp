#include <FL/fl_draw.H>
#include <cmath>

namespace draw
{
/**************************************
 * Section 1 - Variable size Routines *
 **************************************/

void grid(int x, int y, int w, int h)
{
    fl_color(80,80,80);
    fl_rectf(x,y,w,h);
    fl_color(20,20,20);
    fl_line_style(FL_DOT, 1);
    //hlines
    for(int i=0; i<9; ++i) {
        fl_push_clip(x,y+h*i/8-1, w, 2);
        fl_line(x, y+h*i/8, x+w, y+h*i/8);
        fl_pop_clip();
    }
    //vlines
    for(int i=0; i<9; ++i) {
        fl_push_clip(x+i*w/8-1,y, 2, h);
        fl_line(x+i*w/8, y, x+i*w/8, y+h);
        fl_pop_clip();
    }
}

void capped(int x, int y, int w, int h)
{
    fl_color(0x9f,0x06,0x00);
    fl_pie(x,y,h,h,90,270);
    fl_pie(x+w-h,y,h,h,-90,90);
    fl_rectf(x+h/2,y,w-h,h);
    fl_color(0,0,0);
    fl_line_style(FL_SOLID,1);
    //Roundabout way of drawing as fl_arc does not seem to join well
    fl_begin_loop();
    //LHS
    for(int i=0; i<10; ++i)
        fl_vertex(x+h/2+h/2*sin(i*M_PI/9.0+M_PI),
                y+h/2+h/2*cos(i*M_PI/9.0));

    //RHS
    for(int i=0; i<10; ++i)
        fl_vertex(x+w-h/2+h/2*sin(i*M_PI/9.0),
                y+h/2+h/2*cos(i*M_PI/9.0+M_PI));

    fl_end_loop();
}

void capped_label(int x, int y, int w, int h,
                       const char *label)
{
    capped(x, y, w,h);
    fl_color(255,255,255);
    fl_font(14,12);
    fl_draw(label, x+5, y+11);
    fl_font(4,14);
}

/**********************************************
 * Section 2 - Fixed Size Routines at offsets *
 **********************************************/

void sigma(int x, int y)
{
    fl_line_style(FL_SOLID|FL_CAP_ROUND|FL_JOIN_ROUND, 2);
    fl_color(0,0,0);
    fl_begin_line();
    fl_vertex(x+40, y+5);
    fl_vertex(x+40, y+0);
    fl_vertex(x+0,  y+0);
    fl_vertex(x+20, y+20);
    fl_vertex(x+0,  y+40);
    fl_vertex(x+40, y+40);
    fl_vertex(x+40, y+35);
    fl_end_line();
}

/*******************************
 * Section 3 Widget Routines   *
 *******************************/
template<class T>
T min(T a, T b) { return a<b?a:b; }

void slider(int x,int y, int w)
{
    w = min(w, 100-6);
    fl_color(0,0,0);
    fl_line_style(FL_SOLID, 1);
    fl_color(80,80,80);
    fl_rectf(x,y+4,100,12);
    fl_color(0xeb,0x90,0x00);
    fl_rectf(x,y+4,w,12);
    fl_color(0,0,0);
    fl_rect(x,y+4,100,12);
    fl_color(0,0,0);
    fl_rectf(x+w,y+4-3,6,18);
}

void sinplot(int x, int y, int w, int h)
{
    fl_line_style(FL_SOLID, 2);
    fl_color(0,0,0);
    fl_begin_line();
    for(int i=0; i<w; ++i) {
        float phase = i*2*M_PI/(w-1);
        float mag = 0.5+sin(phase)/2;
        fl_vertex(x+i,y+mag*h);
    }
    fl_end_line();
}

void sinplot2(int x, int y, int w, int h)
{
    fl_line_style(FL_SOLID, 2);
    fl_color(0xdb,0x70,0x00);
    fl_begin_polygon();
    for(int i=0; i<w; ++i) {
        float phase = i*2*M_PI/(w-1);
        float mag = 0.5+sin(phase)/2;
        fl_vertex(x+i,y+mag*h);
    }
    fl_end_polygon();
    sinplot(x,y,w,h);
}

/**********************************
 * Section 3 Prerendered elements *
 **********************************/
static void circle(int x, int y)
{
    fl_line_style(FL_SOLID, 5);
    fl_color(0,0,0);
    fl_arc(x,y,80,80,0,360);
    fl_color(230, 230, 200);
    fl_pie(x-1,y-1,80,80,0,360);
}

static void osc(int x, int y)
{
    circle(x,y);
    float inner = sqrt(2.0)*40;
    fl_color(130, 130, 200);
    sinplot(x+(80-inner)/2,
            y+(80-inner)/2,inner,inner);
}

void schematic(void)
{
    fl_line_style(FL_SOLID, 5);
    fl_color(200,200,200);
    //fl_rect(10,10,50,50);

    //Two circles
    osc(20,20);
    osc(20,200);

    //Draw down arrow
    fl_line_style(FL_SOLID, 3);
    fl_color(0, 0, 0);
    fl_line(60,100,60,185);
    fl_begin_polygon();
    fl_vertex(60-8,185);
    fl_vertex(60+8,185);
    fl_vertex(60,197);
    fl_end_polygon();

    //Draw connection to mixer
    fl_begin_line();
    fl_vertex(100,60);
    fl_vertex(200,60);
    fl_vertex(230,120);
    fl_end_line();

    fl_begin_line();
    fl_vertex(100,240);
    fl_vertex(200,240);
    fl_vertex(230,180);
    fl_end_line();

    //Draw Mixer
    circle(210,110);
    sigma(230,130);

    //Draw Amp Connection
    fl_line_style(FL_SOLID, 3);
    fl_color(0, 0, 0);
    fl_line(290, 150, 380, 150);

    fl_line_style(FL_SOLID, 5);
    fl_begin_loop();
    fl_vertex(380,150-35);
    fl_vertex(380,150+35);
    fl_vertex(435,150);
    fl_end_loop();
    fl_color(250, 250, 220);
    fl_begin_polygon();
    fl_vertex(380-1,150-35-1);
    fl_vertex(380-1,150+35);
    fl_vertex(435,150-1);
    fl_end_polygon();

    //Draw endings
    fl_line_style(FL_SOLID, 3);
    fl_color(0, 0, 0);
    fl_line(435, 150, 500, 150);

    fl_begin_polygon();
    fl_vertex(500,150-8);
    fl_vertex(500,150+8);
    fl_vertex(513,150);
    fl_end_polygon();
}
    
void env(int x, int y)
{
    (void) y;
    fl_color(200,240,200);
    fl_begin_polygon();
    fl_vertex(x,400);
    fl_vertex(x+50,320);
    fl_vertex(x+50,400);
    fl_end_polygon();
    fl_begin_polygon();
    fl_vertex(x+80,360);
    fl_vertex(x+150,360);
    fl_vertex(x+150,400);
    fl_vertex(x+80,400);
    fl_end_polygon();
    fl_color(190,200,140);
    fl_begin_polygon();
    fl_vertex(x+150,400);
    fl_vertex(x+150,360);
    fl_vertex(x+200,400);
    fl_end_polygon();
    fl_begin_polygon();
    fl_vertex(x+50,320);
    fl_vertex(x+80,360);
    fl_vertex(x+80,400);
    fl_vertex(x+50,400);
    fl_end_polygon();

    fl_color(0,0,0);
    fl_line_style(FL_SOLID, 2);
    fl_begin_line();
    fl_vertex(x,400);
    fl_vertex(x+50,320);
    fl_vertex(x+80,360);
    fl_vertex(x+150,360);
    fl_vertex(x+200,400);
    fl_end_line();
}
};
