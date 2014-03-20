#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <cmath>
#include "draw.h"
#include "view.h"
#include "synth.h"
#include "UndoWidget.h"

class FooClass:public Fl_Group
{
public:
    FooClass(int x, int y, Synth &synth)
        :Fl_Group(x,y,750,430),vs(synth),
        uw(552,52,198,498),state(1)
    {
        uw.init(*getHistory());
        vs.add(new ControlView(0,300));
        vs.add(new MixerView(0,300));
        vs.add(new OscView(0,300,true));
        vs.add(new OscView(0,300,false));
        vs.add(new AmpView(0,300));
        auto *midi_mode = new ToggleButton(650,0, 100, 50, "MIDI");
        auto *osc1_sel = new InvisibleButton(20,20,80,80);
        auto *osc2_sel = new InvisibleButton(20,200,80,80);
        auto *mix_sel  = new InvisibleButton(210,110,80,80);
        auto *amp_sel  = new InvisibleButton(380,150-35,80,80);
        midi_mode->callback([](Fl_Widget*w, void*v){
                FooClass *foo = (FooClass*)v;
                ToggleButton *t = (ToggleButton*)w;
                if(t->pushed_state)
                    foo->vs.setOverlayMode(2);
                else
                    foo->vs.setOverlayMode(0);}, this);
        osc1_sel->callback([](Fl_Widget*, void *v){
                FooClass *foo = (FooClass*)v;
                foo->vs.select(2);}, this);
        osc2_sel->callback([](Fl_Widget*, void *v){
                FooClass *foo = (FooClass*)v;
                foo->vs.select(3);}, this);
        mix_sel->callback([](Fl_Widget*, void *v){
                FooClass *foo = (FooClass*)v;
                foo->vs.select(1);}, this);
        amp_sel->callback([](Fl_Widget*, void *v){
                FooClass *foo = (FooClass*)v;
                foo->vs.select(4);}, this);
    }

    void draw(void) override
    {
        draw::schematic();

        //Draw parameter Group
        fl_color(0x6b, 0x6b, 0x6b);
        fl_rectf(0,300,550,500);
        fl_rectf(550,0,250, 600);
        fl_color(0,0,0);
        fl_line(0,300,550,300);
        fl_line(550,0,550,600);

        draw_children();

        //if(state == 3)
        //    draw_overlay(0,300);
        //if(state == 4) {
        //    //vs.active().draw_overlay_highlights();
        //    draw_overlay(0,300);
        //}

        fl_color(0,0,0);
        fl_line_style(FL_SOLID,2);
        fl_line(550,50, 750, 50);
        fl_line(650,0, 650, 50);
        fl_color(255,255,255);
        fl_draw("UNDO", 577,30);
        //fl_draw("MIDI", 682,30);
    }

    //int handle(int ev) override
    //{
    //    if(ev == FL_PUSH && Fl::event_button2()) {
    //        state = (state+1)%7;
    //        vs.select(state%vs.views.size());
    //        if(state == 4)
    //            vs.setOverlayMode(2);
    //        else
    //            vs.setOverlayMode(0);
    //        draw();
    //    }
    //    return Fl_Group::handle(ev);
    //}

    //void draw_overlay(int x, int y)
    //{
    //    draw::capped_label(x+415, y+56, 27,14, "18");
    //    draw::capped_label(x+415, y+76, 27,14, "23");
    //}

    ViewSet vs;
    UndoWidget uw;
    int state;
};

int main()
{
    Synth synth;
    setup_jack();
    Fl_Window *w  = new Fl_Double_Window(750,430, "Schemo");
    auto foo = new FooClass(0,0, synth);
    (void) foo;
    w->show();
    while(!NTK || w->shown()) {
        Fl::wait(0.01);
        handleUpdates([foo](const char *addr, std::string s, float f)
                {
                foo->vs.propigate(addr, s, f);
                },
                [foo](){foo->uw.totalRefresh();},
                [foo](){foo->vs.redraw();});

    }
    printf("Done...\n");
    return 0;
}
