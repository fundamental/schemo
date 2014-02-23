#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <cmath>
#include "draw.h"
#include "view.h"

void draw_undo(int x, int y)
{
    fl_color(255,255,255);
    fl_draw("foobar 80:120", x+10,y+20);
    fl_draw("bax      20:21", x+10,y+40);
}

class FooClass:public Fl_Group
{
public:
    FooClass(int x, int y)
        :Fl_Group(x,y,750,430),state(1)
    {
        vs.add(new ControlView(0,300));
        vs.add(new MixerView(0,300));
        vs.add(new OscView(0,300,true));
        vs.add(new OscView(0,300,false));
        vs.add(new AmpView(0,300));
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
        if(state == 3)
            draw_overlay(0,300);
        if(state == 4) {
            vs.active().draw_overlay_highlights();
            draw_overlay(0,300);
        }

        fl_color(0,0,0);
        fl_line_style(FL_SOLID,2);
        fl_line(550,50, 750, 50);
        fl_line(650,0, 650, 50);
        fl_color(255,255,255);
        fl_draw("UNDO", 577,30);
        fl_draw("MIDI", 682,30);
        draw_undo(550,50);

    }

    int handle(int ev) override
    {
        if(ev == FL_PUSH && Fl::event_button2()) {
            state = (state+1)%7;
            vs.select(state%vs.views.size());
            draw();
        }
        return Fl_Group::handle(ev);
    }

    void draw_overlay(int x, int y)
    {
        draw::capped_label(x+415, y+56, 27,14, "18");
        draw::capped_label(x+415, y+76, 27,14, "23");
    }

    ViewSet vs;
    int state;
};

int main()
{
    Fl_Window *w  = new Fl_Double_Window(750,430, "Schemo");
    auto foo = new FooClass(0,0);
    (void) foo;
    w->show();
    return Fl::run();
}
