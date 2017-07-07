// HyperRogue map editor
// Copyright (C) 2011-2016 Zeno Rogue, see 'hyper.cpp' for details

#include <map>
#include <stdint.h>

#ifdef WINDOWS
#include "direntx.h"
#include "direntx.c"
#else
#include <dirent.h>
#endif

namespace mapeditor {
  int subcanvas;
  }


namespace mapeditor {

  bool drawplayer = true;
  char whichPattern = 0;
  char whichShape = 0;
  char whichCanvas = 0;

    int nopattern(cell *c) {
        if(isWarped(c) && !euclid) {
            int u = ishept(c)?1:0;
            int qhex = 0;
            for(int v=0; v<c->type; v++) if(c->mov[v] && !isWarped(c->mov[v])) {
                u += 2;
                if(!ishept(c->mov[v])) qhex++;
            }
            if(u == 2 && qhex == 1) return 8;
            if(u == 6 && qhex == 2) return 10;
            return u;
        }
        return ishept(c) ? 1 : ishex1(c) ? 2 : 0; // 0 to 1
    }
    
    bool reflectPatternAt(cell *c, char p = whichPattern) {
        if(p == 'p' && polarb50(c)) return true;
        if(p == 0 && nopattern(c) == 4) {
            int d = patterndir(c);
            return !isWarped(createMov(c, (d+1)%6));
        }
        return false;
    }
    
    int downdir(cell *c, cellfunction *cf = coastvalEdge) {
        cell *c2 = chosenDown(c, 1, 1, cf);
        if(!c2) return 0;
        return neighborId(c, c2);
    }
    
    int patterndir(cell *c, char w) {
        switch(w) {
            case 'z': {
                int t = zebra40(c);
                
                if(euclid) return (t*4) % 6;
                
                int t4 = t>>2, tcdir = 0;
                
                if(purehepta) tcdir = t^1;
                
                else if(t4 == 10) tcdir = t-20;
                else if(t4 >= 4 && t4 < 7) tcdir = 40 + (t&3);
                else if(t4 >= 1 && t4 < 4) tcdir = t+12;
                else if(t4 >= 7 && t4 < 10) tcdir = t-24;
                
                for(int i=0; i<c->type; i++) if(c->mov[i] && zebra40(c->mov[i]) == tcdir)
                    return i;
                
                // printf("fail to fintd %d -> %d\n", t, tcdir);
                
                return 0;
            }
                
            case 'f': {
                int t = emeraldval(c);
                if(euclid) return 0;
                int tcdir = 0, tbest = (t&3);
                for(int i=0; i<c->type; i++) {
                    cell *c2 = c->mov[i];
                    if(c2) {
                        int t2 = emeraldval(c2);
                        if((t&3) == (t2&3) && t2 > tbest)
                            tbest = t2, tcdir = i;
                    }
                }
                return tcdir;
            }
                
            case 'p': {
                int tcdir = -1, tbest = -1;
                int pa = polara50(c);
                int pb = polarb50(c);
                for(int i=0; i<c->type; i++) {
                    cell *c2 = c->mov[i];
                    if(c2 && polara50(c2) == pa && polarb50(c2) == pb) {
                        int t2 = fiftyval049(c2);
                        if(t2 > tbest) tbest = t2, tcdir = i;
                    }
                }
                return tcdir;
            }
                
            case 'H':
                return downdir(c);
                
            case 0: {
                if(euclid) return 0;
                int u = nopattern(c);
                
                if(u == 6)
                    for(int i=1; i<c->type; i+=2) if(!isWarped(createMov(c,i)))
                        return i;
                
                if(u == 2 || u == 3 || u == 8)
                    for(int i=0; i<c->type; i++) if(!isWarped(createMov(c,i)))
                        return i;
                
                if(u == 4 || u == 10)
                    for(int i=0; i<c->type; i+=2) if(!isWarped(createMov(c,i)))
                        return i;
                
                if(u == 6)
                    for(int i=1; i<c->type; i+=2) if(!isWarped(createMov(c,i))) 
                        return i;
                
                if(u == 5)
                    for(int i=0; i<c->type; i++) if(!isWarped(createMov(c,(i+3)%7)) && !isWarped(createMov(c,(i+4)%7))) 
                        return i;
                
                if(u == 9)
                    for(int i=0; i<c->type; i++) if(!isWarped(createMov(c,(i+2)%7)) && !isWarped(createMov(c,(i+5)%7))) 
                        return i;
                
                if(u == 7)
                    for(int i=0; i<c->type; i++) if(!isWarped(createMov(c,(i+1)%7)) && !isWarped(createMov(c,(i+6)%7))) 
                        return i;
            }
        }
        return 0;
    }
    
  string infix;
  
  bool hasInfix(const string &s) {
    if(infix == "") return true;
    string t = "";
    for(int i=0; i<size(s); i++) {
      char c = s[i];
      char tt = 0;
      if(c >= 'a' && c <= 'z') tt += c - 32;
      else if(c >= 'A' && c <= 'Z') tt += c;
      if(tt) t += tt;
      }
    return t.find(infix) != string::npos;
    }
  
  bool editInfix(int uni) {
    if(uni >= 'A' && uni <= 'Z') infix += uni;
    else if(uni >= 'a' && uni <= 'z') infix += uni-32;
    else if(infix != "" && uni == 8) infix = infix.substr(0, size(infix)-1);
    else if(infix != "" && uni != 0) infix = "";
    else return false;
    return true;
    }
    

  int canvasback = linf[laCanvas].color >> 2;

  int generateCanvas(cell *c) {
    if(whichCanvas == 'C') {
      using namespace fieldpattern;
      int z = fp43.getdist(fieldval(c), make_pair(0,false));
      if(z < fp43.circrad) return 0x00C000;
      int z2 = fp43.getdist(fieldval(c), make_pair(fp43.otherpole,false));
      if(z2 < fp43.disthep[fp43.otherpole] - fp43.circrad)
        return 0x3000;
      return 0x6000;
      }
    if(whichCanvas == 'D') {
      using namespace fieldpattern;
      int z = fp43.getdist(fieldval(c), make_pair(0,false));
      return 255 * (fp43.maxdist+1-z) / fp43.maxdist;
      }
    if(whichCanvas == 'N') {
      using namespace fieldpattern;
      int z = fp43.getdist(fieldval(c), make_pair(0,false));
      int z2 = fp43.getdist(fieldval(c), make_pair(fp43.otherpole,false));
      if(z < z2) return 0x00C000;
      if(z > z2) return 0xC00000;
      return 0xCCCC00;
      }
    if(whichCanvas == 'S') {
      return 0x3F1F0F * fieldpattern::subval(c).second + 0x000080;
      }
    if(whichCanvas == 'g')
      return canvasback;
    if(whichCanvas == 'r')
      return hrand(0xFFFFFF + 1);
    if(whichCanvas == 'e') {
      static unsigned int fcol[4] = { 0x404040, 0x800000, 0x008000, 0x000080 };
      int fv = emeraldval(c);
      return fcol[fv&3];
      }
    if(whichCanvas == 'a') {
      static unsigned int fcol8[8] = { 
        0x800000,
        0x503000,
        0x206000,
        0x007010,
        0x004040,
        0x001070,
        0x200060,
        0x500030
        };
        
      if(c->wall == waNone) {
        int col = fcol8[land50(c)];
        if(polara50(c)) col += 0x181818;
        return col;
        }
      }
    if(whichCanvas == 'b') {
      static unsigned int fcol[4] = { 0x404040, 0x800000, 0x008000, 0x000080 };
      return fcol[polara50(c) + 2 * polarb50(c)];
      }
    if(whichCanvas == 'z') {
      static unsigned int fcol[4] = { 0xC0C0C0, 0xE0E0E0, 0x404040, 0x606060 };
      int fv = zebra40(c);
      return fcol[fv&3];
      }
    if(whichCanvas == 't') {
      static unsigned int fcol[4] = { 0x804040, 0x408040, 0x404080, 0x808040 };
      int fv = zebra40(c);
      if(fv/4 == 4 || fv/4 == 6 || fv/4 == 5 || fv/4 == 10) fv ^= 2;
      return fcol[fv&3];
      }
    if(whichCanvas == 'x') {
      static unsigned int fcol[4] = { 0xC0C0C0, 0x800000, 0x008000, 0x000080 };
      return fcol[zebra3(c)];
      }
    if(whichCanvas == 'w') {
      static unsigned int fcol[2] = { 0x303030, 0xC0C0C0 };
      return fcol[randpattern(c, subcanvas) ? 1 : 0];
      }
    if(whichCanvas == 'l') {
      int col[4];
      bool err = false;
      for(int j=0; j<4; j++) {
        col[j] = getCdata(c, j);
        col[j] *= 3;
        col[j] %= 240;
        if(col[j] > 120) col[j] = 240 - col[j];
        if(col[j] < -120) col[j] = -240 - col[j];
        }
      return (0x808080 + col[0] + (col[1] << 8) + (col[2] << 16)) >> (err?2:0);
      }
    if(whichCanvas == 'd') {
      int col[4];
      bool err = false;
      for(int j=0; j<4; j++) {
        col[j] = getCdata(c, j);
        col[j] *= 3;
        col[j] %= 240;
        if(col[j] > 120) col[j] = 240 - col[j];
        if(col[j] < -120) col[j] = -240 - col[j];
        }
      col[0] /= 8;
      col[1] /= 8;
      col[2] /= 8;
      return (0x101010 + col[0] + (col[1] << 8) + (col[2] << 16)) >> (err?2:0);
      }
    if(whichCanvas == 'F') {
      return ishept(c) ? 0x202020 : 0xC0C0C0;
      }
    return canvasback;
    }
  }
