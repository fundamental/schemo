#pragma once
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include "widgets.h"
#include "draw.h"
#include "synth.h"
#include <vector>
#include <map>
#include <cassert>

using namespace std::placeholders;

typedef std::vector<std::tuple<std::string, int, int>> ParameterSet;

//Collection of fixed sized views
//550x130
class View:public Fl_Group
{
public:
    View(int x, int y)
        :Fl_Group(x,y,550,130)
    {}
    void draw_overlay_highlights(void)
    {
        fl_color(0xff,0xee,0x4e);
        fl_line_style(FL_SOLID,1);
        for(int i=0; i<children(); ++i)
        {
            if(auto *pack = dynamic_cast<SliderVpack*>(child(i)))
                pack->draw_overlay_highlights();
        }
    }
    void draw_overlay_labels(void);
    const char *getAddress(int x, int y);

    Synth *synth;

    std::vector<std::string> addr;
    std::multimap<Widget*,std::string> addresses;
    std::multimap<std::string, std::pair<Widget*,int>> widgets;
    void changeCb(int i, float f)
    {
        printf("Trying to update param #%d(%s) to %f\n",
                i, addr[i].c_str(), f);
    }
};

std::vector<std::string> &
operator<<(std::vector<std::string> &v, std::string str)
{
    v.push_back(str);
    return v;
}

class ViewSet
{
public:
    ViewSet(Synth &synth_)
        :synth(synth_)
    {}
    Synth &synth;

    void add(View *view)
    {
        assert(view);
        view->hide();
        view->synth = &synth;
        views.push_back(view);
    }

    void select(int idx)
    {
        for(auto view:views)
            view->hide();
        views[idx]->show();
    }

    View &active(void)
    {
        for(auto view:views)
            if(view->visible())
                return *view;
        assert(0);
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
        new TinyButton(x+405, y+47, "L");
        new TinyButton(x+425, y+47, "D");
        new TinyButton(x+405, y+67, "L");
        new TinyButton(x+425, y+67, "D");
        //draw_middings(x()+350,y()+40);
        end();
    }

    static void draw_middings(int x, int y)
    {
        fl_color(255,255,255);
        fl_draw("Coarse", x,    y+20);
        fl_draw("Fine",   x,    y+40);
        fl_draw("CC42",   x+100, y+20);
        fl_draw("(none)", x+100, y+40);
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
        osc1->callback(_cb, 0);

        auto *osc2 = new Input2D(x+330, y+20, 200, 100);
        osc2->setCursor(100,60);
        osc2->callback(_cb, (void*)2);
        end();

        std::string base = "/mix/";
        addr << base + "pan1"
             << base + "mix1"
             << base + "pan2"
             << base + "mix2";
    }
    
    void draw(void) override
    {
        fl_color(255,255,255);
        fl_draw("OSC 1", x()+10, y()+70);
        fl_draw("OSC 2", x()+280, y()+70);
        draw_children();
    }

    static void _cb(Fl_Widget *w, void *v)
    {
        Input2D &i = *(Input2D*)w;
        int offset = (int)v;
        View &view = *(View*)i.parent();
        view.changeCb(offset,   i.cursorX*1.0/i.w());
        view.changeCb(offset+1, i.cursorY*1.0/i.h());
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
            pack->add("Sync",       "On",          100, true);
        } else {
            pack->add("Square Mix", "17.0%",       17);
            pack->add("Saw Mix",    "80.0%",       80);
            pack->add("Sin Mix",    "20.0%",       20);
            pack->add("Detune",     "+100 cents",  60);
        }
        pack->childrenCb(std::bind(&View::changeCb,this,_1,_2));

        end();
        init_parameters();
    }
    void init_parameters(void)
    {
        std::string base = hasSync?"/osc1/":"/osc2/";
        addr << base + "square-mix"
             << base + "saw-mix"
             << base + "sin-mix"
             << base + "detune";
        if(hasSync)
            addr << base + "sync";

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
        pack->childrenCb(std::bind(&View::changeCb,this,_1,_2));
        end();
        init_parameters();
    }
    
    void draw(void) override
    {
        draw::env(20, 300);
        draw_children();
    }

    void init_parameters(void)
    {
        std::string base = "/amp/";
        addr << base + "volume"
             << base + "sustain"
             << base + "attack"
             << base + "decay"
             << base + "release";
    }
};
