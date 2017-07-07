#define ISANDROID 0
#define ISMOBILE 1
#define ISIOS 0
// error : inserted by coan: "#define MOBILE" contradicts -U or --implicit at /home/ryan/hyperrogue-cut-down/fake-mobile.cpp(4)
#define MOBPAR_FORMAL int
#define MOBPAR_ACTUAL 0
#define FAKEMOBILE
#define ANDROIDSHARE
#define MIX_MAX_VOLUME 128

const char *scorefile = "fakemobile_score.txt";
const char *conffile = "fakemobile_config.txt";

#include <SDL/SDL.h>

#include <GL/gl.h>
#include "init.cpp"

#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>

#undef main

void playSound(cell *, const string &s, int vol) {
  printf("play sound: %s vol %d\n", s.c_str(), vol);
  }

SDL_Surface *s;

int gdpos = 0;

int gdpop() { return graphdata[gdpos++]; }

TTF_Font *font[256];

bool rawdisplaystr(int x, int y, int shift, int size, const char *str, int color, int align) {

  if(strlen(str) == 0) return false;

  if(size <= 0 || size > 255) {
    return false;
    }
  SDL_Color col;
  col.r = (color >> 16) & 255;
  col.g = (color >> 8 ) & 255;
  col.b = (color >> 0 ) & 255;
  
  col.r >>= darken; col.g >>= darken; col.b >>= darken;

  if(!font[size])
    font[size] = TTF_OpenFont("VeraBd.ttf", size);

  SDL_Surface *txt = TTF_RenderText_Solid(font[size], str, col);
  
  if(txt == NULL) return false;

  SDL_Rect rect;

  rect.w = txt->w;
  rect.h = txt->h;

  rect.x = x - rect.w * align / 16;
  rect.y = y - rect.h/2;
  
  bool clicked = (mousex >= rect.x && mousey >= rect.y && mousex <= rect.x+rect.w && mousey <= rect.y+rect.h);
  
  SDL_BlitSurface(txt, NULL, s,&rect); 
  SDL_FreeSurface(txt);
  
  return clicked;
  }

int textwidth(int siz, const string &str) {
  if(size(str) == 0) return 0;
  
  if(!font[siz]) font[siz] = TTF_OpenFont("VeraBd.ttf", siz);
  
  int w, h;
  TTF_SizeUTF8(font[siz], str.c_str(), &w, &h);
  // printf("width = %d [%d]\n", w, size(str));
  return w;
  }

char action;

int getticks() { return SDL_GetTicks(); }

bool currentlyConnecting() { return false; }
bool currentlyConnected() { return false; }
void viewAchievements() { printf("view Achievements\n"); }
void viewLeaderboard(string id) { printf("view Leaderboard :: %s\n", id.c_str()); }
void switchGoogleConnection() { printf("sgc\n"); }

void shareScore(int) {
  printf("share\n");
  }

int main(int argc, char **argv) {

  initAll();
  
  vid.xres = 800; vid.yres = 450;
  vid.usingGL = false;
  // 450; vid.yres = 600;

  s= SDL_SetVideoMode(vid.xres, vid.yres, 32, 0);
  
  if(TTF_Init() != 0) {
    printf("Failed to initialize TTF.\n");
    exit(2);
    }

  int mx = 0; int my = 0; bool _clicked = false;
  int action = 0;
  
  firstland = laMinefield;
  
  items[itGreenStone] = 100;
  items[itDiamond] = 50;
  for(int i=1; i<10; i++) kills[i] = 5;
  
  while(true) {
   
    SDL_LockSurface(s);
    memset(s->pixels, 0, vid.xres * vid.yres * 4);
    SDL_UnlockSurface(s);
    
    mousex = mx;
    mousey = my;
    clicked = _clicked;
    
    mobile_draw(MOBPAR_ACTUAL);
    action = 0;
    
    gdpos = 0;
    while(gdpos < graphdata.size()) {
      switch(gdpop()) {
        case 2: {
          int x = gdpop(), y = gdpop(), al = gdpop();
          int color = gdpop();
          int size = gdpop();
          int b = gdpop();
          int n = gdpop();
          string s;
          for(int i=0; i<n; i++) s += char(gdpop());
          rawdisplaystr(x, y, 0, size, s.c_str(), color, al);
          break;
          }
        
        case 1: {
          int col = gdpop();
          int otl = gdpop();
          int num = gdpop();
          
          Sint16 xpox[6000], xpoy[6000];
          
          // printf("%4d polygon %d\n", gdpos, num);
          for(int i=0; i<num; i++) xpox[i] = gdpop(), xpoy[i] = gdpop();

          filledPolygonColor(s, xpox, xpoy, num, col);
          aapolygonColor(s, xpox, xpoy, num, otl);
          break;
          }

        case 3: {
          int col = gdpop();
          int num = gdpop();

          for(int i=0; i<num; i++) polyx[i] = gdpop(), polyy[i] = gdpop();
          
          for(int i=0; i<num-1; i++)
            aalineColor(s, polyx[i], polyy[i], polyx[i+1], polyy[i+1], col);
          }

        case 4: {
          int col = gdpop();
          int x = gdpop(), y = gdpop(), rad = gdpop();
          
          aacircleColor(s, x, y, rad, (col << 8) + 0xFF);
          }

        }
      }
    SDL_UpdateRect(s, 0, 0, vid.xres, vid.yres);

    SDL_Event ev;
    while(SDL_PollEvent(&ev)) {
    
      if(ev.type == SDL_MOUSEBUTTONDOWN) {
        mx = ev.button.x;
        my = ev.button.y;
        _clicked = true;
        }

      if(ev.type == SDL_MOUSEBUTTONUP) {
        _clicked = false;
        }

      if(ev.type == SDL_MOUSEMOTION) {
        mx = ev.motion.x;
        my = ev.motion.y;
        }
      
      if(ev.type == SDL_KEYDOWN) {
        int sym = ev.key.keysym.sym;
/*        if(sym == '1') {
  printf("Number of cells explored, by distance from the player:\n"); 
  for(int i=0; i<10; i++) printf(" %d", explore[i]); printf("\n");
          return 0;
          }
        else if(sym == '2') {
          items[rand() % ittypes] += 3;
          kills[rand() % motypes] += 3;
          }
        else if(sym == '3') {
          items[itHell] = 0;
          items[itGreenStone] = 100;
          }
        action = sym; */
        extra ex;
        mousing = false;
        handlekey(sym, sym, ex);
        }
        
      if(ev.type == SDL_QUIT) {
        SDL_Quit();
        return 0;
        }
      }
    }

  SDL_Quit();
  
  clearMemory();
  }

void openURL() {
  printf("< openURL > \n");
  }
