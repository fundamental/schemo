#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include "draw.h"
#include <FL/fl_draw.H>
#include <cstdio>
#include <vector>

template<class T>
T max(T a, T b) { return a<b?b:a; }

class Slider:public Fl_Widget
{
public:
    Slider(int x, int y)
        :Fl_Widget(x,y,100,19), value(0), name(0)
    {}
    void draw(void) {
        draw::slider(x(),y(),value);
    }
    int handle(int ev) {
        if(ev == FL_PUSH || ev == FL_DRAG) {
            value = max(0,Fl::event_x()-x());
            redraw();
        }

        return 1;
    }
    int value;
    const char *name;
};

class SliderVpack:public Fl_Group
{
public:
    SliderVpack(int x, int y, int n)
        :Fl_Group(x,y,280,n*20+10)
    {end();}

    void draw_overlay_highlights(void)
    {
        for(int i=0; i<children(); ++i)
            if(auto *slider = dynamic_cast<Slider*>(child(i)))
                fl_rect(slider->x(),slider->y(),slider->w(),slider->h());
    }
    void add(const char *label, const char *val, int fillage)
    {
        begin();
        auto *slider = new Slider(x()+80, y()+5+20*value.size());
        slider->value = fillage;
        slider->name  = label;
        end();
        this->label.push_back(label);
        value.push_back(fillage);
        this->val.push_back(val);
    }
    std::vector<const char *> label, val;
    std::vector<int>          value;

    void add(const char *address)
    {
        (void) address;
    };
    void draw(void) override
    {
        //Draw Labels
        fl_color(255,255,255);
        fl_font(FL_COURIER,14);
        for(int i=0; i<(int)label.size(); ++i) {
            fl_draw(label[i], x(),     y()+(1+i)*20);
            fl_draw(val[i],   x()+185, y()+(1+i)*20);
        }

        draw_children();
    }
};

class Plot:public Fl_Widget
{
public:
    Plot(int x, int y, int w, int h)
        :Fl_Widget(x,y,w,h)
    {}
    void draw(void)
    {
        draw::grid(x(),y(),w(),h());
        draw::sinplot2(x(),y(),w(),h());
    }
};

class Input2D:public Fl_Widget
{
public:
    Input2D(int x, int y, int w, int h)
        :Fl_Widget(x,y,w,h),cursorX(x+w/2),cursorY(y+h/2)
    {}
    void setCursor(int x,int y)
    {
        cursorX=x;
        cursorY=y;
    }
    void draw(void)
    {
        draw::grid(x(),y(),w(),h());

        fl_line_style(FL_SOLID,2);
        fl_color(0xeb,0x90,0x00);
        fl_arc(x()+cursorX, y()+cursorY, 10, 10,0,360);
    }
    int cursorX;
    int cursorY;
};


class Toggle:public Fl_Widget
{
};
