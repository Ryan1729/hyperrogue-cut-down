CXXFLAGS ?=

all: hyper

savepng-loc.o: savepng.c
	gcc savepng.c -c -o savepng-loc.o

hyper.o: hyper.cpp graph.cpp hyperpoint.cpp geometry.cpp cell.cpp heptagon.cpp game.cpp classes.cpp polygons.cpp language.cpp language-data.cpp achievement.cpp netgen.cpp mapeditor.cpp
	g++ hyper.cpp -c -o hyper.o -DFHS -Wall ${CXXFLAGS} ${CPPFLAGS} ${LDFLAGS} -O3 -DNOTRANS

hyper: hyper.o savepng-loc.o
	g++ savepng-loc.o hyper.o -o hyper -lSDL -lSDL_ttf -lSDL_mixer -lSDL_gfx ${CXXFLAGS} ${CPPFLAGS} ${LDFLAGS} -lGL -lpng


