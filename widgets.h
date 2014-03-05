#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include "draw.h"
#include <cstdio>
#include <vector>
#include <string>
#include <functional>

template<class T>
T limit(T val, T min, T max)
{
    return val < min ? min : val > max ? max : val;
}

enum ValueType
{
    VALUE_NORM_FLOAT, //0..1
    VALUE_RAW_FLOAT,  //a..b
    VALUE_RAW_INT,    //a..b
    VALUE_BOOL,       //true xor false
    VALUE_STRING,     //some arbitrary string
};

class Widget:public Fl_Widget
{
public:
    Widget(int x, int y, int w, int h)
        :Fl_Widget(x,y,w,h)
    {}

    virtual void updateParam(int, float){}
    virtual void updateLabel(int, std::string){};
};

class Slider:public Widget
{
public:
    Slider(int x, int y)
        :Widget(x,y,100,19), value(0), name(0)
    {}
    void draw(void) override {
        draw::slider(x(),y(),value);
    }
    int handle(int ev) {
        if(ev == FL_PUSH || ev == FL_DRAG) {
            value = limit(Fl::event_x()-x(),0,95);
            redraw();
            do_callback();
            return 1;
        }

        return 0;
    }

    void updateParam(int, float f)
    {
        value=f*95;
    }

    int value;
    const char *name;
};

class SliderVpack:virtual public Widget, public Fl_Group
{
public:
    SliderVpack(int x, int y, int n)
        :Widget(x,y,280,n*20+10),Fl_Group(x,y,280,n*20+10),  offset(0)
    {end();}

    void draw_overlay_highlights(void)
    {
        for(int i=0; i<children(); ++i)
            if(auto *slider = dynamic_cast<Slider*>(child(i)))
                fl_rect(slider->x(),slider->y(),slider->w(),slider->h());
    }
    void add(const char *label, const char *val, int fillage,
            bool bool_=false)
    {
        begin();
        auto *slider = new Slider(Fl_Group::x()+80, Fl_Group::y()+5+20*value.size());
        slider->value = fillage;
        slider->name  = label;
        slider->callback(SliderVpack::_cb,this);
        end();

        sliders.push_back(slider);
        bool_spec.push_back(bool_);
        this->label.push_back(label);
        value.push_back(fillage);
        this->val.push_back(val);
    }
    std::vector<bool> bool_spec;
    std::vector<Slider *>     sliders;
    std::vector<const char *> label, val;
    std::vector<int>          value;

    void add(const char *address)
    {
        (void) address;
    }

    void draw(void) override
    {
        //Draw Labels
        fl_color(255,255,255);
        fl_font(FL_COURIER,14);
        for(int i=0; i<(int)label.size(); ++i) {
            fl_draw(label[i], Fl_Group::x(),     Fl_Group::y()+(1+i)*20);
            fl_draw(val[i],   Fl_Group::x()+185, Fl_Group::y()+(1+i)*20);
        }

        draw_children();
    }

    void childrenCb(std::function<void(int, float)> foo, int off=0)
    {
        offset = off;
        callback = foo;
    }
    
    void updateParam(int i, float f)
    {
        if(i > (int)sliders.size()) {
            printf("Bad parameter index\n");
            return;
        }
        sliders[i]->updateParam(0,f);
    }

    void updateLabel(int i, std::string str)
    {
        printf("trying to update label '%d' to '%s'\n", i, str.c_str());
        if(i > (int)val.size()) {
            printf("Bad parameter index\n");
            return;
        }
        val[i] = strdup(str.c_str());
        Fl_Group::redraw();
    }

    std::function<void(int,float)> callback;
    int offset;

    void handleWidget(Fl_Widget*w)
    {
        Slider &s = *(Slider*)w;
        int ind = offset;
        for(unsigned i=0;i<sliders.size(); ++i, ++ind)
            if(sliders[i] == &s)
                break;
        if(bool_spec[ind-offset]) {
            if(s.value < 50)
                s.value = 0;
            if(s.value > 50)
                s.value = 95;
        }
        if(callback)
            callback(ind, s.value/95.0);
    };

    static void _cb(Fl_Widget *w, void *v)
    {
        SliderVpack &svp = *(SliderVpack*)v; 
        svp.handleWidget(w);
    }
};

class Plot:public Fl_Widget
{
public:
    Plot(int x, int y, int w, int h)
        :Fl_Widget(x,y,w,h)
    {
        smps = new float[w+1];
        memset(smps, 0, sizeof(float)*w+1);
    }
    void draw(void)
    {
        draw::grid(x(),y(),w(),h());
        draw::plot(x(),y(),w(),h(), smps);
        //draw::sinplot2(x(),y(),w(),h());
    }
    float *smps;
};

class Input2D:public Widget
{
public:
    Input2D(int x, int y, int w, int h)
        :Widget(x,y,w,h),cursorX(x+w/2),cursorY(y+h/2)
    {}

    int handle(int ev)
    {
        if(ev == FL_PUSH || ev == FL_DRAG) {
            cursorX = limit(Fl::event_x()-x(),0,w()-10);
            cursorY = limit(Fl::event_y()-y(),0,h()-10);
            do_callback();
            redraw();
            return 1;
        }

        return 0;
    }

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

    void updateParam(int i, float f)
    {
        if(i==0)
            cursorX = f*w();
        else
            cursorY = f*h();
    }

    int cursorX;
    int cursorY;
};


class TinyButton:public Fl_Widget
{
public:
    TinyButton(int x, int y, const char *label)
        :Fl_Widget(x,y,14,14,label), pushed_state(false)
    {}

    int handle(int ev) override
    {
        if(ev==FL_PUSH) {
            pushed_state = true;
            redraw();
            return 1;
        }
        if(ev==FL_RELEASE) {
            pushed_state = false;
            redraw();
            return 1;
        }
        return 0;
    }

    void draw(void) override
    {
        if(pushed_state)
            fl_color(80,80,80);
        else
            fl_color(0,0,0);
        fl_rectf(x(),y(),w(),h());
        fl_color(255,255,255);
        fl_draw(label(),x()+2,y()+12);
    }
    bool pushed_state;
};
