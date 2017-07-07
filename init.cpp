#define VER "9.4g"
#define VERNUM 9407
#define VERNUM_HEX 0x9407

#define GEN_M 0
#define GEN_F 1
#define GEN_N 2
#define GEN_O 3




#ifdef MOBWEB
#define NORUG
#define NOEDIT
#define NOMODEL
#endif




// desktop

#include <stdio.h>

#ifdef USE_SDL
#include <SDL/SDL.h>

#ifndef MAC
#undef main
#endif

#include <SDL/SDL_ttf.h>
#endif

#include <cmath>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>

#ifdef USE_UNORDERED_MAP
#include <unordered_map>
#else
#define unordered_map map
#endif

using namespace std;

string s0;
void addMessage(string s, char spamtype = 0);


FILE *debugfile;
int debugflags;

#ifdef USE_COMMANDLINE
const char *scorefile = "hyperrogue.log";
const char *conffile = "hyperrogue.ini";
string levelfile = "hyperrogue.lev";
string picfile = "hyperrogue.pic";
const char *musicfile = "";
const char *loadlevel = NULL;
#endif

#define S7 (sphere?5:7)
#define S42 (S7*6)
#define S14 (S7*2)
#define S21 (S7*3)
#define S28 (S7*4)
#define S84 (S7*12)

#include "util.cpp"
#include "hyperpoint.cpp"
#include "patterns.cpp"
#include "classes.cpp"
#include "fieldpattern.cpp"
#include "heptagon.cpp"
#include "language.cpp"
#include "hyper.h"
#include "cell.cpp"
#include "flags.cpp"
#include "yendor.cpp"
#include "complex.cpp"
#include "game.cpp"
#include "landgen.cpp"
#include "orbs.cpp"
#include "system.cpp"
#include "geometry.cpp"
#include "polygons.cpp"
#include "mapeditor.cpp"
#include "netgen.cpp"
#include "graph.cpp"
#include "sound.cpp"
#include "achievement.cpp"
#ifdef TOUR
#include "tour.cpp"
#endif
#include <unistd.h>

bool fixseed = false;

void initAll() {
  ca::init();
  arg::read(1);
  srand(time(NULL));
  shrand(fixseed ? 0 : time(NULL));

  achievement_init(); // not in ANDROID

  eLand f = firstland;
  
  // initlanguage();
  initgraph();
#ifndef NOSAVE
  loadsave();
#endif
  resetGeometry();
  initcells();
  
  shmup::safety = safety;
  initgame();
  restartGraph();

  if(!shmup::on) {
    restoreGolems(items[itOrbLife], moGolem); items[itOrbLife] = 0;
    restoreGolems(items[itOrbFriend], moTameBomberbird); items[itOrbFriend] = 0;
    restoreGolems(kills[moPrincessMoved], moPrincess, princess::saveHP); kills[moPrincessMoved] = 0;
    restoreGolems(kills[moPrincessArmedMoved], moPrincessArmed, princess::saveArmedHP); kills[moPrincessArmedMoved] = 0;
    }
  
  firstland = f;
  }

void finishAll() {
  achievement_final(!items[itOrbSafety]);
  
#ifndef NOSAVE
  saveStats();
#endif
  offscreen.clear();
  clearMemory();
  cleargraph();
  
  achievement_close();  
  }


