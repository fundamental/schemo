#pragma once
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include "widgets.h"
#include "draw.h"
#include <vector>
#include <cassert>


//Collection of fixed sized views
//550x130
class View:public Fl_Group
{
public:
    View(int x, int y)
        :Fl_Group(x,y,550,130)
    {}
    void draw_overlay_highlights(void);
    void draw_overlay_labels(void);
    const char *getAddress(int x, int y);
};

class ViewSet
{
public:
    void add(View *view)
    {
        assert(view);
        view->hide();
        views.push_back(view);
    }

    void select(int idx)
    {
        for(auto view:views)
            view->hide();
        views[idx]->show();
    }
    std::vector<View*> views;
};

class ControlView:public View
{
public:
    ControlView(int x, int y)
        :View(x,y)
    {
        auto pack = new SliderVpack(x+60,y+40,3);
        pack->add("Minimum", "7.0%", 7);
        pack->add("Maximum", "30.0%", 30);
        pack->add("Scale",   "Linear", 95);
        end();
    }

    static void draw_middings(int x, int y)
    {
        fl_color(255,255,255);
        fl_draw("Coarse", x,    y+20);
        fl_draw("Fine",   x,    y+40);
        fl_draw("CC42",   x+100, y+20);
        fl_draw("(none)", x+100, y+40);
        fl_color(0,0,0);
        fl_rectf(x+55, y+7, 14,14);
        fl_rectf(x+75, y+7, 14,14);
        fl_rectf(x+55, y+27, 14,14);
        fl_rectf(x+75, y+27, 14,14);
        fl_color(255,255,255);
        fl_draw("L", x+57, y+19);
        fl_draw("D", x+77, y+19);
        fl_draw("L", x+57, y+39);
        fl_draw("D", x+77, y+39);
    }

    void draw(void) override
    {
        fl_color(255,255,255);
        fl_draw("/osc1/sqr_mag - Oscillator square magnitude", x()+20,y()+20);
        int w,h;
        fl_measure("/osc1/sqr_mag - ",w,h,1);
        fl_draw("'f' in [0,1]",x()+20+w,y()+40);

        draw_middings(x()+350,y()+40);
        draw_children();
    }

};

class MixerView:public View
{
public:
    MixerView(int x, int y)
        :View(x,y)
    {
        auto *osc1 = new Input2D(x+60,y+20,200,100);
        osc1->setCursor(20,10);

        auto *osc2 = new Input2D(x+330, y+20, 200, 100);
        osc2->setCursor(100,60);
        end();
    }
    
    void draw(void) override
    {
        fl_color(255,255,255);
        fl_draw("OSC 1", x()+10, y()+70);
        fl_draw("OSC 2", x()+280, y()+70);
        draw_children();
    }
};

class OscView:public View
{
public:
    OscView(int x, int y, bool sync)
        :View(x,y), hasSync(sync)
    {
        new Plot(x+20, y+20, 200, 100);
        auto *pack = new SliderVpack(x+255,y+10,5);
        if(sync) {
            pack->add("Square Mix", "7.0%",        7);
            pack->add("Saw Mix",    "30.0%",       30);
            pack->add("Sin Mix",    "80.0%",       80);
            pack->add("Detune",     "-1200 cents", 30);
            pack->add("Sync",       "On",          100);
        } else {
            pack->add("Square Mix", "17.0%",       17);
            pack->add("Saw Mix",    "80.0%",       80);
            pack->add("Sin Mix",    "20.0%",       20);
            pack->add("Detune",     "+100 cents",  60);
        }

        end();
    }

    bool hasSync;
};

class AmpView:public View
{
public:
    AmpView(int x, int y)
        :View(x,y)
    {
        auto *pack = new SliderVpack(x+255,y+10,5);
        pack->add("Volume",  "-2.0 dB", 30);
        pack->add("Sustain", "-8.2 dB", 70);
        pack->add("Attack",  "10 ms",   10);
        pack->add("Decay",   "100 ms", 90);
        pack->add("Release",  "0 ms",  0);

        end();
    }
    
    void draw(void) override
    {
        draw::env(20, 300);
        draw_children();
    }
};
