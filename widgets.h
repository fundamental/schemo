#pragma once
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include "draw.h"
#include <FL/fl_draw.H>
#include <cstdio>
#include <vector>

class SliderVpack:public Fl_Group
{
public:
    SliderVpack(int x, int y, int n)
        :Fl_Group(x,y,100,n*20)
    {end();}

    void add(const char *label, const char *val, int fillage)
    {
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

        //Draw sliders
        for(int i=0; i<(int)value.size(); ++i)
            draw::slider(x()+80,y()+10+20*i, value[i]);
    }
};

class Plot:public Fl_Widget
{
public:
    void draw(void)
    {
        draw::grid(x(),y(),w(),h());
    }
};

class Input2D:public Fl_Widget
{
public:
    void draw(void)
    {
        draw::grid(x(),y(),w(),h());
    }
};

class Slider:public Fl_Widget
{
public:
    Slider(int x, int y)
        :Fl_Widget(x,y,100,20)
    {}
    void draw(void) {
        draw::slider(x(),y(),w());
    }
    int handle(int ev) {
        if(ev == FL_PUSH || ev == FL_DRAG)
            printf("Should move the pointer...\n");

        return 1;
    }
};

class Toggle:public Fl_Widget
{
};
