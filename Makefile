schemo: main.cpp draw.h draw.cpp view.h widgets.h Makefile
	g++ -std=c++11 main.cpp draw.cpp -o schemo `pkg-config ntk --cflags --libs`

test: schemo
	./schemo

