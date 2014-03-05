schemo: main.cpp draw.h draw.cpp view.h widgets.h synth.h synth.cpp Makefile UndoWidget.h
	clang++ -std=c++11 -g -O0 main.cpp draw.cpp synth.cpp -o schemo `pkg-config ntk  --cflags --libs` `pkg-config librtosc --cflags --libs` -lrtosc-cpp -ljack

test: schemo
	./schemo

