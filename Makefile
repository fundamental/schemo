schemo: main.cpp draw.h draw.cpp view.h widgets.h synth.h synth.cpp Makefile UndoWidget.h
	g++ -std=c++11 -DNTK=1 -g -O0 main.cpp draw.cpp synth.cpp -o schemo `pkg-config ntk  --cflags --libs` `pkg-config librtosc --cflags --libs` -lrtosc-cpp -ljack -Wall -Wextra

schemo-fltk: main.cpp draw.h draw.cpp view.h widgets.h synth.h synth.cpp Makefile UndoWidget.h
	g++ -std=c++11 -DNTK=0 -g -O0 main.cpp draw.cpp synth.cpp -o schemo-fltk `fltk-config --cflags --ldflags` `pkg-config librtosc --cflags --libs` -lrtosc-cpp -ljack

clean:
	rm -f ./schemo

test: schemo
	./schemo

