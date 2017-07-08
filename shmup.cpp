// Hyperbolic Rogue

// Copyright (C) 2011-2016 Zeno Rogue, see 'hyper.cpp' for details

// implementation of the shoot'em up mode

#define SHMUPTITLE "shoot'em up and multiplayer"

#include <map>

extern int mousex, mousey;
extern bool clicked;

// joysticks for controlling the mobile shmup mode
namespace shmupballs {
  int xmove, xfire, yb, rad;

  void calc() {
    rad = vid.radius * (vid.mobilecompasssize ? vid.mobilecompasssize : 14) / 100;
    xmove = max(vid.xcenter - vid.radius - rad, rad);
    xfire = min(vid.xcenter + vid.radius + rad, vid.xres - rad);
    yb = vid.ycenter + vid.radius - rad;
    }
  }

namespace multi {

  int players = 1;
  cellwalker player[MAXPLAYER];
  vector<int> revive_queue; // queue for revival
  
  cell *origpos[MAXPLAYER], *origtarget[MAXPLAYER];

  bool flipped[MAXPLAYER];
  
  int treasures[MAXPLAYER], kills[MAXPLAYER], deaths[MAXPLAYER];
  
  bool alwaysuse = false;

  void recall() {
    for(int i=0; i<numplayers(); i++) {
        int idir = (3 * i) % cwt.c->type;
        cell *c2 = cwt.c->mov[idir];
        makeEmpty(c2);
        if(!passable(c2, NULL, P_ISPLAYER)) c2 = cwt.c;
        multi::player[i].c = c2;
        multi::player[i].spin = 0;
        
        multi::flipped[i] = true;
        multi::whereto[i].d = MD_UNDECIDED;
        }
    }
  
  transmatrix whereis[MAXPLAYER];
  transmatrix crosscenter[MAXPLAYER];
  double ccdist[MAXPLAYER];
  cell *ccat[MAXPLAYER];
  
  bool combo[MAXPLAYER];

  int cpid; // player id -- an extra parameter for player-related functions
  int cpid_edit; // cpid currently being edited
  
  movedir whereto[MAXPLAYER]; // player's target cell  

  double mdx[MAXPLAYER], mdy[MAXPLAYER]; // movement vector for the next move

  const char* playercmds_shmup[15] = {
    "forward", "backward", "turn left", "turn right",
    "move up", "move right", "move down", "move left", 
    "throw a knife", "face the pointer", "throw at the pointer", 
    "drop Dead Orb", "center the map on me", "Orb power (target: mouse)",
    "Orb power (target: facing)"
    };
  
  const char* playercmds_turn[15] = {
    "move up-right", "move up-left", "move down-right", "move down-left", 
    "move up", "move right", "move down", "move left", 
    "stay in place (left + right)", "cancel move", "leave the game", 
    "drop Dead Orb (up + down)", "center the map on me", "",
    ""
    };
  
  const char* pancmds[7] = {
    "pan up", "pan right", "pan down", "pan left",
    "rotate left", "rotate right", "home"
    };

#define SHMUPAXES_BASE 4
#define SHMUPAXES ((SHMUPAXES_BASE) + 4 * (MAXPLAYER))

const char* axemodes[SHMUPAXES] = {
  "do nothing", 
  "rotate view",
  "panning X",
  "panning Y",
  "player 1 X", 
  "player 1 Y", 
  "player 1 go", 
  "player 1 spin", 
  "player 2 X", 
  "player 2 Y", 
  "player 2 go", 
  "player 2 spin",
  "player 3 X", 
  "player 3 Y", 
  "player 3 go", 
  "player 3 spin",
  "player 4 X", 
  "player 4 Y", 
  "player 4 go", 
  "player 4 spin",
  "player 5 X", 
  "player 5 Y", 
  "player 5 go", 
  "player 5 spin",
  "player 6 X", 
  "player 6 Y", 
  "player 6 go", 
  "player 6 spin",
  "player 7 X", 
  "player 7 Y", 
  "player 7 go", 
  "player 7 spin"
  };

int centerplayer = -1;


int shmupnumkeys;
const char** shmupcmdtable;

char* axeconfigs[24]; int numaxeconfigs;
int* dzconfigs[24];

string listkeys(int id) {
  string lk = "";
  for(int i=0; i<512; i++)
    if(vid.scfg.keyaction[i] == id)
      lk = lk + " " + SDL_GetKeyName(SDLKey(i));
  for(int i=0; i<numsticks; i++) for(int k=0; k<SDL_JoystickNumButtons(sticks[i]) && k<MAXBUTTON; k++)
    if(vid.scfg.joyaction[i][k] == id) {
      lk = lk + " " + cts('A'+i)+"-B"+its(k);
      }
  for(int i=0; i<numsticks; i++) for(int k=0; k<SDL_JoystickNumHats(sticks[i]) && k<MAXHAT; k++)
    for(int d=0; d<4; d++)
      if(vid.scfg.hataction[i][k][d] == id) {
        lk = lk + " " + cts('A'+i)+"-"+"URDL"[d];
        }
  return lk;
  }

#define SCJOY 16

string dsc(int id) {
  char buf[64];
  sprintf(buf, " (%d $$$, %d kills, %d deaths)", 
    multi::treasures[id],
    multi::kills[id],
    multi::deaths[id]
    );
  return buf;
  }

void resetScores() {
  for(int i=0; i<MAXPLAYER; i++)
    multi::treasures[i] = multi::kills[i] = multi::deaths[i] = 0;
  }
 
bool shmupcfg;

bool configdead;

void handleConfig(int sym, int uni) {
  if(!vid.scfg.setwhat) dialog::handleNavigation(sym, uni);
  int sc = vid.scfg.subconfig;
  if(sc == 0) {
    if(uni == '1') vid.scfg.subconfig = 1;
    else if(uni == '2') vid.scfg.subconfig = 2;
    else if(uni == 'p') vid.scfg.subconfig = 3;
    else if(uni == '3') vid.scfg.subconfig = 4;
    else if(uni == '4') vid.scfg.subconfig = 5;
    else if(uni == '5') vid.scfg.subconfig = 6;
    else if(uni == '6') vid.scfg.subconfig = 7;
    else if(uni == '7') vid.scfg.subconfig = 8;
    else if(uni == 'j') vid.scfg.subconfig = SCJOY;
    else if(uni == 'a') multi::alwaysuse = !multi::alwaysuse;
    else if(uni == 'b') cmode = emJoyConfig;
    else if(uni == 'r') 
      for(int i=0; i<MAXPLAYER; i++) 
        kills[i] = deaths[i] = treasures[i] = 0;
    else if(uni == 's' || uni == 't') 
      shmupcfg = !shmupcfg;
    else if(uni == 'n' || uni == 'N') {
      vid.scfg.players += shiftmul > 0 ? 1 : -1;
      vid.scfg.players %= MAXPLAYER;
      if(vid.scfg.players <= 0) vid.scfg.players += MAXPLAYER;
      }
    else if(sym == SDLK_F1 || uni == '?' || uni == 'h') {
      lastmode = cmode;
      cmode = emHelp;

      /* help = 
        "In the shmup (shoot'em up) mode, you can play a hyperbolic shoot'em up "
        "game. The game is based on the usual turn-based grid-based HyperRogue, "
        "but there are some changes. You fight by throwing knives, and you "
        "have three extra lives. There are no friendly monsters, so Orbs of "
        "Life and Friendship give you extra lives instead. Some other rules have been "
        "adapted too.\n\n"
        "It is possible for two players to play the shmup mode cooperatively "
        "(locally). When playing together, lives, orbs, and treasures are shared, "
        "knives recharge slower, orbs drain faster, and player characters are not "
        "allowed to separate."; */

      help = 
      XLAT(
"Shmup (shoot'em up) mode: You can play a hyperbolic shoot'em up game. The game is based "
"on the usual turn-based grid-based HyperRogue, but there are some changes. You fight by "
"throwing knives, and you have three extra lives. There are no allies, so all Orbs "
"related to allies give you extra lives instead (up to 5). Some other rules have been "
"adapted too.\n\n");

help += XLAT(
"Multiplayer: Play cooperatively (locally); treasures, kills, and deaths are calculated "
"for each player too, for more competitive play. Orbs and treasures are shared, orbs drain "
"faster, knives recharge slower, and player characters are not allowed to separate.\n\n");

help += XLAT(
"Turn-based multiplayer: Turns are executed in parallel. A player can leave the game "
"by pressing a designated key (useful when about to get killed or lost). The following "
"Orbs work to bring such players back: ");

help += XLATN(iinf[itOrbLife].name); help += ", ";
help += XLATN(iinf[itOrbFriend].name); help += ", ";
help += XLATN(iinf[itOrbUndeath].name); help += ", ";
help += XLATN(iinf[itOrbTeleport].name); help += ", ";
help += XLATN(iinf[itOrbSafety].name); help += "\n\n";

help += XLAT("This menu can be also used to configure keys.\n\n");
        
      }
    else if(uni || sym == SDLK_F10) {
      cmode = emNormal;
      if(shmup::on != shmupcfg) { restartGame('s'); resetScores(); }
      else if(vid.scfg.players != players) { restartGame(); resetScores(); }
      }
    }
  else if(sc == SCJOY) {
    if(sym) {
      char xuni = uni | 96;
      if(xuni >= 'a' && xuni < 'a' + numaxeconfigs) {
        if(configdead) {
          int& dz = (*dzconfigs[xuni - 'a']);
          dz += int(shiftmul * 100);
          if(dz < 0) dz = 0;
          if(dz > 65000) dz = 65000;
          }
        else {
          int v = (*axeconfigs[xuni - 'a']);
          v += (shiftmul>0?1:-1);
          v += SHMUPAXES;
          v %= SHMUPAXES;
          (*axeconfigs[xuni - 'a']) = v;
          }
        }
      else if(xuni == 'z')
        configdead = !configdead;
      else if(uni || sym == SDLK_F10)
        vid.scfg.subconfig = 0;
      }
    }
  else {
    if(sym) {
      if(vid.scfg.setwhat) {
        vid.scfg.keyaction[sym] = vid.scfg.setwhat;
        vid.scfg.setwhat = 0;
        }
      else if(uni >= 'a' && uni < 'a' + shmupnumkeys && shmupcmdtable[uni-'a'][0])
        vid.scfg.setwhat = 16*sc+uni - 'a';
      else if(uni == 'z')
        vid.scfg.setwhat = 1;
      else if(uni || sym == SDLK_F10)
        vid.scfg.subconfig = 0;
      }
    }
  }

#define NUMACT 128

enum pcmds {
  pcForward, pcBackward, pcTurnLeft, pcTurnRight,
  pcMoveUp, pcMoveRight, pcMoveDown, pcMoveLeft,
  pcFire, pcFace, pcFaceFire,
  pcDrop, pcCenter, pcOrbPower, pcOrbKey
  };
  
int actionspressed[NUMACT], axespressed[SHMUPAXES], lactionpressed[NUMACT];

void pressaction(int id) {
  if(id >= 0 && id < NUMACT)
    actionspressed[id]++;
  }

void handleInput(int delta) {
  double d = delta / 500.;

  Uint8 *keystate = SDL_GetKeyState(NULL);

  for(int i=0; i<NUMACT; i++) 
    lactionpressed[i] = actionspressed[i],
    actionspressed[i] = 0;

  for(int i=0; i<SHMUPAXES; i++) axespressed[i] = 0;
  
  for(int i=0; i<SDLK_LAST; i++) if(keystate[i]) 
    pressaction(vid.scfg.keyaction[i]);
  
  for(int j=0; j<numsticks; j++) {

    for(int b=0; b<SDL_JoystickNumButtons(sticks[j]) && b<MAXBUTTON; b++)
      if(SDL_JoystickGetButton(sticks[j], b))
        pressaction(vid.scfg.joyaction[j][b]);

    for(int b=0; b<SDL_JoystickNumHats(sticks[j]) && b<MAXHAT; b++) {
      int stat = SDL_JoystickGetHat(sticks[j], b);
      if(stat & SDL_HAT_UP) pressaction(vid.scfg.hataction[j][b][0]);
      if(stat & SDL_HAT_RIGHT) pressaction(vid.scfg.hataction[j][b][1]);
      if(stat & SDL_HAT_DOWN) pressaction(vid.scfg.hataction[j][b][2]);
      if(stat & SDL_HAT_LEFT) pressaction(vid.scfg.hataction[j][b][3]);
      }
    
    for(int b=0; b<SDL_JoystickNumAxes(sticks[j]) && b<MAXAXE; b++) {
      int value = SDL_JoystickGetAxis(sticks[j], b);
      int dz = vid.scfg.deadzoneval[j][b];
      if(value > dz) value -= dz; else if(value < -dz) value += dz;
      else value = 0;
      axespressed[vid.scfg.axeaction[j][b] % SHMUPAXES] += value;
      }
    }

  if(keystate[SDLK_LCTRL] || keystate[SDLK_RCTRL]) d /= 5;
  
  double panx = 
    actionspressed[49] - actionspressed[51] + axespressed[2] / 32000.0;
  double pany = 
    actionspressed[50] - actionspressed[48] + axespressed[3] / 20000.0;
    
  double panspin = actionspressed[52] - actionspressed[53] + axespressed[1] / 20000.0;
  
  if(actionspressed[54]) { centerplayer = -1, playermoved = true; centerpc(100); }
  
  if(panx || pany || panspin) {
    View = xpush(-panx * d) * ypush(-pany * d) * spin(panspin * d) * View;
    playermoved = false;
    }
  }

int tableid[7] = {1, 2, 4, 5, 6, 7, 8};

void initConfig() {
  vid.scfg.players = 1;
  
  char* t = vid.scfg.keyaction;
  
  t['w'] = 16 + 0;
  t['s'] = 16 + 1;
  t['a'] = 16 + 2;
  t['d'] = 16 + 3;

  t[SDLK_KP8] = 16 + 4;
  t[SDLK_KP6] = 16 + 5;
  t[SDLK_KP2] = 16 + 6;
  t[SDLK_KP4] = 16 + 7;

  t['f'] = 16 + pcFire;
  t['g'] = 16 + pcFace;
  t['h'] = 16 + pcFaceFire;
  t['r'] = 16 + pcDrop;
  t['t'] = 16 + pcOrbPower;
  t['y'] = 16 + pcCenter;

  t['i'] = 32 + 0;
  t['k'] = 32 + 1;
  t['j'] = 32 + 2;
  t['l'] = 32 + 3;
  t[';'] = 32 + 8;
  t['\''] = 32 + 9;
  t['p'] = 32 + 10;
  t['['] = 32 + pcCenter;

  t[SDLK_UP] = 48 ;
  t[SDLK_RIGHT] = 48 + 1;
  t[SDLK_DOWN] = 48 + 2;
  t[SDLK_LEFT] = 48 + 3;
  t[SDLK_PAGEUP] = 48 + 4;
  t[SDLK_PAGEDOWN] = 48 + 5;
  t[SDLK_HOME] = 48 + 6;

  vid.scfg.joyaction[0][0] = 16 + pcFire;
  vid.scfg.joyaction[0][1] = 16 + pcOrbPower;
  vid.scfg.joyaction[0][2] = 16 + pcDrop;
  vid.scfg.joyaction[0][3] = 16 + pcCenter;
  vid.scfg.joyaction[0][4] = 16 + pcFace;
  vid.scfg.joyaction[0][5] = 16 + pcFaceFire;

  vid.scfg.joyaction[1][0] = 32 + pcFire;
  vid.scfg.joyaction[1][1] = 32 + pcOrbPower;
  vid.scfg.joyaction[1][2] = 32 + pcDrop;
  vid.scfg.joyaction[1][3] = 32 + pcCenter;
  vid.scfg.joyaction[1][4] = 32 + pcFace;
  vid.scfg.joyaction[1][5] = 32 + pcFaceFire;

  vid.scfg.axeaction[0][0] = 4;
  vid.scfg.axeaction[0][1] = 5;
  vid.scfg.axeaction[0][3] = 2;
  vid.scfg.axeaction[0][4] = 3;

  vid.scfg.axeaction[1][0] = 8;
  vid.scfg.axeaction[1][1] = 9;
  
  // ULRD
  vid.scfg.hataction[0][0][0] = 16 + 0;
  vid.scfg.hataction[0][0][1] = 16 + 3;
  vid.scfg.hataction[0][0][2] = 16 + 1;
  vid.scfg.hataction[0][0][3] = 16 + 2;
  vid.scfg.hataction[0][1][0] = 16 + 4;
  vid.scfg.hataction[0][1][1] = 16 + 7;
  vid.scfg.hataction[0][1][2] = 16 + 5;
  vid.scfg.hataction[0][1][3] = 16 + 6;

  vid.scfg.hataction[1][0][0] = 32 + 0;
  vid.scfg.hataction[1][0][1] = 32 + 3;
  vid.scfg.hataction[1][0][2] = 32 + 1;
  vid.scfg.hataction[1][0][3] = 32 + 2;
  vid.scfg.hataction[1][1][0] = 32 + 4;
  vid.scfg.hataction[1][1][1] = 32 + 7;
  vid.scfg.hataction[1][1][2] = 32 + 5;
  vid.scfg.hataction[1][1][3] = 32 + 6;

  int charidtable[MAXPLAYER] = {0, 1, 4, 6, 2, 3, 0};
    
  for(int i=0; i<MAXPLAYER; i++) {
    initcs(multi::scs[i]); 
    multi::scs[i].charid = charidtable[i];
    }
  
  multi::scs[0].uicolor = 0xC00000FF;
  multi::scs[1].uicolor = 0x00C000FF;
  multi::scs[2].uicolor = 0x0000C0FF;
  multi::scs[3].uicolor = 0xC0C000FF;
  multi::scs[4].uicolor = 0xC000C0FF;
  multi::scs[5].uicolor = 0x00C0C0FF;
  multi::scs[6].uicolor = 0xC0C0C0FF;
  }

  void leaveGame(int i) {
    multi::player[i].c = NULL;
    multi::deaths[i]++;
    revive_queue.push_back(i);
    checklastmove();
    }

  bool playerActive(int p) {
    if(multi::players == 1 || shmup::on) return true;
    return player[p].c;
    }
  
  int activePlayers() {
    int q = 0;
    for(int i=0; i<players; i++) if(playerActive(i)) q++;
    return q;
    }
  
  cell *multiPlayerTarget(int i) {
    cellwalker cwti = multi::player[i];
    if(!cwti.c) return NULL;
    int dir = multi::whereto[i].d;
    if(dir == MD_UNDECIDED) return NULL;
    if(dir == MD_USE_ORB) return multi::whereto[i].tgt;
    if(dir >= 0) { cwspin(cwti, dir); cwstep(cwti); }
    return cwti.c;
    }
  
  void checklastmove() {
    for(int i=0; i<numplayers(); i++) if(playerActive(i)) {
      multi::cpid = i;
      cwt = multi::player[i]; break;
      }
    if(multi::activePlayers() == 1) {
      multi::checkonly = true;
      checkmove();
      multi::checkonly = false;
      }
    }

  bool needinput = true;
  
  void handleMulti(int delta) {
    multi::handleInput(delta);
    
    transmatrix bcwtV = cwtV;
    cellwalker bcwt = cwt;
    
    bool alldecided = !needinput;
    
    if(multi::players == 1) {
      multi::cpid = 0;
      multi::whereis[0] = cwtV;
      multi::player[0] = cwt;
      }
    
    for(int i=0; i<numplayers(); i++) if(playerActive(i)) {
    
      using namespace multi;
      
  // todo refactor
  
      cpid = i;
      
      int b = 16*tableid[cpid];
      for(int ik=0; ik<8; ik++) if(actionspressed[b+ik]) playermoved = true;
      for(int ik=0; ik<16; ik++) if(actionspressed[b+ik] && !lactionpressed[b+ik]) 
        multi::combo[i] = false;
          
      bool anypressed = false;
      
      int jb = 4*tableid[cpid];
      for(int ik=0; ik<4; ik++) 
        if(axespressed[jb+ik]) 
          anypressed = true, playermoved = true, multi::combo[i] = false;
      
      double mdx = 
        (actionspressed[b+0] + actionspressed[b+2] - actionspressed[b+1] - actionspressed[b+3]) * .7 +
        actionspressed[b+pcMoveRight] - actionspressed[b+pcMoveLeft] + axespressed[jb]/30000.;
      double mdy = 
        (actionspressed[b+3] + actionspressed[b+2] - actionspressed[b+1] - actionspressed[b+0]) * .7 +
        actionspressed[b+pcMoveDown] - actionspressed[b+pcMoveUp] + axespressed[jb+1]/30000.;
      
      if((actionspressed[b+pcMoveRight] && actionspressed[b+pcMoveLeft]) ||
        (actionspressed[b+pcMoveUp] && actionspressed[b+pcMoveDown]))
          multi::mdx[i] = multi::mdy[i] = 0;
        
      multi::mdx[i] = multi::mdx[i] * (1 - delta / 1000.) + mdx * delta / 2000.;
      multi::mdy[i] = multi::mdy[i] * (1 - delta / 1000.) + mdy * delta / 2000.;
  
      if(mdx != 0 || mdy != 0) if(!multi::combo[i]) {
        cwtV = multi::whereis[i]; cwt = multi::player[i];
        flipplayer = multi::flipped[i];
        multi::whereto[i] = vectodir(hpxy(multi::mdx[i], multi::mdy[i]));
        }
      
      if(multi::actionspressed[b+pcFire] || 
        (multi::actionspressed[b+pcMoveLeft] && multi::actionspressed[b+pcMoveRight]))
        multi::combo[i] = true, multi::whereto[i].d = MD_WAIT;
  
      if(multi::actionspressed[b+pcFace])
        multi::whereto[i].d = MD_UNDECIDED;
      
      cwt.c = multi::player[i].c;      
      if(multi::ccat[i] && !multi::combo[i] && targetRangedOrb(multi::ccat[i], roMultiCheck)) {
        multi::whereto[i].d = MD_USE_ORB;
        multi::whereto[i].tgt = multi::ccat[i];
        }

      if(multi::actionspressed[b+pcFaceFire] && activePlayers() > 1) {
        addMessage(XLAT("Left the game."));
        multi::leaveGame(i);
        }
  
      if(actionspressed[b+pcDrop] || 
        (multi::actionspressed[b+pcMoveUp] && multi::actionspressed[b+pcMoveDown]))
        multi::combo[i] = true, multi::whereto[i].d = MD_DROP;
  
      if(actionspressed[b+pcCenter]) {
        centerplayer = cpid; centerpc(100); playermoved = true; 
        }
  
      if(multi::whereto[i].d == MD_UNDECIDED) alldecided = false;
      
      for(int ik=0; ik<16; ik++) if(actionspressed[b+ik]) anypressed = true;

      if(anypressed) alldecided = false, needinput = false;
      else multi::mdx[i] = multi::mdy[i] = 0;
      }
      
    cwtV = bcwtV;
    cwt = bcwt;
    
    if(alldecided) {
      flashMessages();
      // check for crashes
      needinput = true;

      for(int i=0; i<numplayers(); i++) if(playerActive(i)) {
        origpos[i] = player[i].c;
        origtarget[i] = multiPlayerTarget(i);
        }
  
      for(int i=0; i<numplayers(); i++) if(playerActive(i))
      for(int j=0; j<numplayers(); j++) if(playerActive(j)) if(i != j) {
        if(origtarget[i] == origtarget[j]) {
          addMessage("Two players cannot move/attack the same location!");
          return;
          }
/*      if(multiPlayerTarget(i) == multi::player[j].c) {
          addMessage("Cannot move into the current location of another player!");
          return;
          }
        if(celldistance(multiPlayerTarget(i), multiPlayerTarget(j)) > 8) {
          addMessage("Players cannot get that far away!");
          return;
          } */
        }
  
      if(multi::players == 1) {
        if(movepcto(multi::whereto[0]))
          multi::whereto[0].d = MD_UNDECIDED;
        return;
        }
      
      multi::cpid = 0;
      if(multimove()) {
        multi::aftermove = false;
        monstersTurn();
        checklastmove();
        }
      }
    }
  
  void mousemovement(cell *c) {
    if(!c) return;
    int countplayers = 0;
    int countplayers_undecided = 0;
    for(int i=0; i<multi::players; i++)
      if(multi::playerActive(i) && (playerpos(i) == c || isNeighbor(c, playerpos(i)))) {
        countplayers++;
        if(multi::whereto[i].d == MD_UNDECIDED) countplayers_undecided++;
        }
  
    for(int i=0; i<multi::players; i++)
      if(multi::playerActive(i) && (playerpos(i) == c || isNeighbor(c, playerpos(i)))) {
        int& cdir = multi::whereto[i].d;
        int scdir = cdir;
        bool isUndecided = cdir == MD_UNDECIDED;
        if(countplayers_undecided > 0 && ! isUndecided) continue;
        if(playerpos(i) == c)
          multi::whereto[i].d = MD_WAIT;
        else {
          for(int d=0; d<playerpos(i)->type; d++) {
            cdir = d;
            if(multi::multiPlayerTarget(i) == c) break;
            cdir = scdir;
            }
          }
        }
    
    needinput = 
      ((countplayers == 2 && !countplayers_undecided) || countplayers_undecided >= 2);
    }
  
  }

/*
const char *lastprofile = "";
int lt = 0;

void profile(const char *buf) {
  int gt = SDL_GetTicks();
  printf("%4d %s\n", gt - lt, lastprofile);
  lt = gt;
  lastprofile = buf;
  }
*/

#define SCALE (crossf/hcrossf)
#define SCALE2 (SCALE*SCALE)

namespace shmup {

transmatrix &ggmatrix(cell *c);

using namespace multi;

eItem targetRangedOrbKey(orbAction a);
eItem keyresult[MAXPLAYER];

ld fabsl(ld x) { return x>0?x:-x; }

bool on = false, safety = false;

bool lastdead = false;

struct monster;

multimap<cell*, monster*> monstersAt;

typedef multimap<cell*, monster*>::iterator mit;

vector<monster*> active;

cell *findbaseAround(hyperpoint p, cell *around) {
  cell *best = around;
  double d0 = intval(p, gmatrix[around] * C0);
  for(int i=0; i<around->type; i++) {
    cell *c2 = around->mov[i];
    if(c2 && gmatrix.count(c2)) {
      double d1 = intval(p, gmatrix[c2] * C0);
      if(d1 < d0) { best = c2; d0 = d1; }
      }
    }
  return best;
  }

cell *findbaseAroundRepeat(hyperpoint p, cell *around) {
  while(true) {
    cell *c = findbaseAround(p, around);
    if(c == around) return c;
    around = c;
    }
  }


/* double distance(hyperpoint h) {
  h = spintox(h) * h;
  return asinh(h[2]);
  } */

struct monster {
  eMonster type;
  cell *base;
  cell *torigin; 
    // tortoises: origin
    // butterflies: last position
  transmatrix at;
  transmatrix pat;
  eMonster stk;
  bool dead;
  bool notpushed;
  bool inBoat;
  monster *parent; // who shot this missile
  eMonster parenttype; // type of the parent
  int nextshot;    // when will it be able to shot (players/flailers)
  int pid;         // player ID
  char hitpoints;
  int stunoff;
  int blowoff;
  double swordangle; // sword angle wrt at
  double vel;        // velocity, for flail balls
  double footphase;
  
  monster() { 
    dead = false; inBoat = false; parent = NULL; nextshot = 0; 
    stunoff = 0; blowoff = 0; footphase = 0;
    }

  void store() {
    monstersAt.insert(make_pair(base, this));
    }
    
  void findpat() {
    pat = gmatrix[base] * at;
    }

  cell *findbase(hyperpoint p) {
    return findbaseAround(p, base);
    }
  
  void rebasePat(transmatrix new_pat) {
    pat = new_pat;
    cell *c2 = findbase(pat*C0);
    // if(c2 != base) printf("rebase %p -> %p\n", base, c2);
    base = c2;
    at = inverse(gmatrix[c2]) * pat;
    fixmatrix(at);
    }

  void rebaseAt(transmatrix new_at) {
    rebasePat(gmatrix[base] * new_at);
    }

  bool trackroute(transmatrix goal, double spd) {
    cell *c = base;
    
    // queuepoly(goal, shGrail, 0xFFFFFFC0);

    transmatrix mat = inverse(pat) * goal;
    
    transmatrix mat2 = spintox(mat*C0) * mat;
    
    double d = 0, dist = asinh(mat2[0][2]);

    while(d < dist) {
      d += spd;
      transmatrix nat = pat * rspintox(mat * C0) * xpush(d); 

      // queuepoly(nat, shKnife, 0xFFFFFFC0);

      cell *c2 = findbaseAround(nat*C0, c);
      if(c2 != c && !passable_for(type, c2, c, P_CHAIN | P_ONPLAYER)) {
        return false;
        }
      c = c2;
      }
    return true;
    }

  bool trackrouteView(transmatrix goal, double spd) {
    cell *c = base;
    
    queuepoly(goal, shGrail, 0xFFFFFFC0);

    transmatrix mat = inverse(pat) * goal;
    
    transmatrix mat2 = spintox(mat*C0) * mat;
    
    double d = 0, dist = asinh(mat2[0][2]);

    while(d < dist) {
      d += spd;
      transmatrix nat = pat * rspintox(mat * C0) * xpush(d); 

      // queuepoly(nat, shKnife, 0xFFFFFFC0);

      cell *c2 = findbaseAround(nat*C0, c);
      if(c2 != c) {
        if(0) printf("old dist: %lf\n", (double) intval(nat*C0, gmatrix[c]*C0));
        if(0) printf("new dist: %lf\n", (double) intval(nat*C0, gmatrix[c2]*C0));
        }
      queuepoly(gmatrix[c2], shKnife, 0xFF0000FF);
      if(c2 != c && !passable_for(type, c2, c, P_CHAIN))
        return false;
      c = c2;
      }
    if(0) printf("dist = %lf, d = %lf, spd = %lf, lint = %lf, lcd = %lf\n", dist, d, spd, 
      (double) intval(pat * rspintox(mat * C0) * xpush(d)*C0, goal*C0),
      (double) intval(pat * rspintox(mat * C0) * xpush(d)*C0, gmatrix[c]*C0)
      );
    return true;
    }

  };

monster *pc[MAXPLAYER], *mousetarget, *lmousetarget;

int curtime, nextmove, nextdragon;

bool isBullet(monster *m) { 
  return isBulletType(m->type);
  }
bool isPlayer(monster *m) { return m->type == moPlayer; }
bool isMonster(monster *m) { return m->type != moPlayer && m->type != moBullet; }

void killMonster(monster* m, eMonster who_kills, int flags = 0) {
  int tk = tkills();
  if(m->dead) return;
  m->dead = true;
  if(isBullet(m) || isPlayer(m)) return;
  m->stk = m->base->monst;
  if(m->inBoat && isWatery(m->base)) {
    m->base->wall = waBoat;
    m->base->mondir = 0;
    m->inBoat = false;
    }
  else if(m->inBoat && m->base->wall == waNone && (
    (m->base->land == laOcean || m->base->land == laLivefjord))) {
    m->base->wall = waStrandedBoat;
    m->base->mondir = 0;
    m->inBoat = false;
    }
  m->base->monst = m->type;
  killMonster(m->base, who_kills, flags);
  m->base->monst = m->stk;
  if(multi::cpid >= 0)
    multi::kills[multi::cpid] += tkills() - tk;
  }

void pushmonsters() {
  for(int i=0; i<int(active.size()); i++) {
    monster *m = active[i];
    m->notpushed = isPlayer(m) || m->dead || (m->base->monst && m->base->monst != m->type);
    if(!m->notpushed) {
      m->stk = m->base->monst;
      m->base->monst = m->type;
      }
    }
  }

void popmonsters() {
  for(int i=int(active.size())-1; i>=0; i--) {
    monster *m = active[i];
    if(!m->notpushed) {
      if(m->type == m->base->monst)
        m->base->monst = m->stk;
      else {
        m->dead = true; // already killed
        // also kill all the other monsters pushed there
        for(int j=0; j<i; j++) {
          monster *m2 = active[j];
          if(m2->base == m->base && !m2->notpushed) 
            killMonster(m2, moNone);
          }
        }
      }
    }
  }

void degradeDemons() {
  for(int i=0; i<int(active.size()); i++) {
    monster *m = active[i];
    if(m->type == moGreater) m->type = moLesser;
    if(m->stk == moGreater) m->type = moLesser;
    }
  }

// we need these for the Mimics!
double playerturn[MAXPLAYER], playergo[MAXPLAYER];
bool playerfire[MAXPLAYER];

void awakenMimics(monster *m, cell *c2) {
  for(int i=0; i<int(dcal.size()); i++) {
    cell *c = dcal[i];
    if(isMimic(c->monst)) {
      // straight
      int i = 0;
      
      // if(m->type == moMirror) i++;
      if(c->monst == moMirror) i++;
      
      transmatrix mirrortrans = Id;    
      if(i == 1) mirrortrans[0][0] = -1;

      if(!gmatrix.count(c)) continue;
      monster *m2 = new monster;
      m2->type = c->monst;
      c->monst = moNone;
      m2->base = c;
      
      if(isBullet(m)) {
        m2->parenttype = m2->type;
        m2->type = m->type;
        m2->vel = m->vel;
        m2->parent = m->parent;
        m2->pid = m->pid;
        }
      
      if(m->type == moMirror) {
        if(m2->type == moMirror) m2->type = moMirage;
        else if(m2->type == moMirage) m2->type = moMirror;
        }
      
      hyperpoint H = inverse(gmatrix[c2]) * gmatrix[c] * C0;
      
      transmatrix xfer = rgpushxto0(H);

      if(i == 1) {
        hyperpoint H2 = spintox(H) * H;
        xfer = rspintox(H) * rpushxto0(H2) * mirrortrans * spintox(H);
        }

      m2->pat = gmatrix[c2] * xfer * inverse(gmatrix[c2]) * m->pat;
        
      m2->at = inverse(gmatrix[c]) * m2->pat * mirrortrans;
      m2->pid = cpid;
      
      if(isBullet(m) && i == 1) m2->at = m2->at * spin(M_PI); // no idea why this
      
      active.push_back(m2);

      // if you don't understand it, don't worry,
      // I don't understand it either
      }
    }
  }

int visibleAt;

void visibleFor(int t) {
  visibleAt = max(visibleAt, curtime + t);
  }

void shootBullet(monster *m) {
  monster* bullet = new monster;
  bullet->base = m->base;
  bullet->at = m->at;
  bullet->type = moBullet;
  bullet->parent = m;
  bullet->pid = m->pid;
  bullet->parenttype = m->type;
  active.push_back(bullet);
  
  if(markOrb(itOrbThorns)) {
    monster* bullet = new monster;
    bullet->base = m->base;
    bullet->at = m->at * spin(M_PI/2);
    bullet->type = moBullet;
    bullet->parent = m;
    bullet->pid = m->pid;
    bullet->parenttype = m->type;
    active.push_back(bullet);
    }

  if(markOrb(itOrbThorns)) {
    monster* bullet = new monster;
    bullet->base = m->base;
    bullet->at = m->at * spin(-M_PI/2);
    bullet->type = moBullet;
    bullet->parent = m;
    bullet->pid = m->pid;
    bullet->parenttype = m->type;
    active.push_back(bullet);
    }
 
  if(markOrb(itOrbDash)) {
    monster* bullet = new monster;
    bullet->base = m->base;
    bullet->at = m->at * spin(M_PI);
    bullet->type = moBullet;
    bullet->parent = m;
    bullet->parenttype = m->type;
    active.push_back(bullet);
    }
  }

monster *playerCrash(monster *who, hyperpoint where) {
  for(int j=0; j<players; j++) if(pc[j]!=who) {
    double d = intval(pc[j]->pat*C0, where);
    if(d < 0.1 * SCALE2 || d > 100) return pc[j];
    }
  return NULL;
  }

void oceanCurrents(transmatrix& nat, monster *m, int delta) {
  cell *c = m->base;
  if(c->land == laWhirlpool) {
    for(int i=0; i<c->type; i++) {
      cell *c2 = c->mov[i];
      if(!c2 || !gmatrix.count(c2)) continue;
      
      double spd = 0;
      
      if(celldistAlt(c2) < celldistAlt(c)) 
        spd = SCALE * delta / 3000.;
      else if(c2 == whirlpool::get(c, 1))
        spd = SCALE * delta / 900.;
        
      if(spd) {
        transmatrix goal = gmatrix[c2];

        // transmatrix t = spintox(H) * xpush(delta/300.) * rspintox(H);

        hyperpoint H = inverse(m->pat) * goal * C0;
        nat = nat * rspintox(H);
        nat = nat * xpush(spd);
        nat = nat * spintox(H);
        }
      }
    }
  }

void airCurrents(transmatrix& nat, monster *m, int delta) {
  cell *c = m->base;
  if(c->land == laWhirlwind) {
    whirlwind::calcdirs(c);
    for(int i=0; i<whirlwind::qdirs; i++) {
      cell *c2 = c->mov[whirlwind::dto[i]];
      if(!c2 || !gmatrix.count(c2)) continue;
      
      double spd = SCALE * delta / 900.;
        
      if(spd) {
        transmatrix goal = gmatrix[c2];

        // transmatrix t = spintox(H) * xpush(delta/300.) * rspintox(H);

        hyperpoint H = inverse(m->pat) * goal * C0;
        nat = nat * rspintox(H);
        nat = nat * xpush(spd);
        nat = nat * spintox(H);
        }
      }
    }
  }

void roseCurrents(transmatrix& nat, monster *m, int delta) {
  if(ignoresSmell(m->type)) return;
  cell *c = m->base;
  
  int qty = 0;

  for(int i=0; i<c->type; i++) {
    cell *c2 = c->mov[i];
    if(c2 && rosedist(c2) == 2) qty++;
    }
  
  for(int i=0; i<c->type; i++) {
    cell *c2 = c->mov[i];
    if(!c2 || !gmatrix.count(c2)) continue;
    if(rosedist(c2) != 2) continue;
    
    double spd = SCALE * delta / 300. / qty;
        
    if(spd) {
      transmatrix goal = gmatrix[c2];

      // transmatrix t = spintox(H) * xpush(delta/300.) * rspintox(H);

      hyperpoint H = inverse(m->pat) * goal * C0;
      nat = nat * rspintox(H);
      nat = nat * xpush(spd);
      nat = nat * spintox(H);
      }
    }
  }

hyperpoint keytarget(int i) {
  double d = 2 + sin(curtime / 350.);
  return pc[i]->pat * xpush(d) * C0;
  }

/* int charidof(int pid) {
  if(players == 1) return bak_charid;
  if(players == 2 || players == 4) return pid;
  if(players == 3) return pid < 2 ? pid : 2+(bak_charid&1);
  return 0;
  } */

double getSwordSize() { return 0.7255; }
double getHornsSize() { return 0.33; }

hyperpoint swordpos(int id, bool rev, double frac) {
  return pc[id]->pat * spin(pc[id]->swordangle) * xpush((rev?-frac:frac) * getSwordSize()) * C0;
  }

hyperpoint hornpos(int id) {
  return pc[id]->pat * tC0(xpush(getHornsSize()));
  }

#define IGO 9

double igospan[IGO] = { 0, 
  M_PI/6, -M_PI/6, 
  M_PI/4, -M_PI/4,
  M_PI/3, -M_PI/3,
  M_PI/2.1, -M_PI/2.1
  };

bool swordKills(eMonster m) {
  return 
    m != moHedge && m != moMetalBeast && m != moMetalBeast2
    && m != moTortoise && m != moGreater && m != moRoseBeauty
    && m != moReptile && !isBull(m) && m != moButterfly;
  }

bool hornKills(eMonster m) {
  return 
    m != moHedge && m != moMetalBeast && m != moMetalBeast2
    && m != moTortoise && m != moGreater && m != moSkeleton
    && m != moDraugr && m != moRoseBeauty
    && m != moReptile && !isBull(m) && m != moButterfly && !isBulletType(m)
    && m != moPalace && m != moFatGuard && m != moVizier;
  }

bool hornStuns(eMonster m) {
  return !isBulletType(m) && m != moRoseBeauty;
  }

bool noncrashable(monster *m, monster *by) {
  eMonster mt = m->type;
  if(mt == moGreater) return true;
  if(mt == moDraugr && by->type != moDraugr) return true;
  if(isBull(mt)) return true;
  if(mt == moReptile) return true;
  if(mt == moRoseBeauty && by->type != moRoseLady) return true;
  if(mt == moTortoise) return true;
  if(mt == moSkeleton) return true;
  return false;
  }

int bulltime[MAXPLAYER];

monster *getPlayer() {
  return pc[cpid];
  }

void moveMimic(monster *m) {
  transmatrix nat = m->pat;
  cpid = m->pid;
  m->footphase = getPlayer()->footphase;
  
  // no need to care about Mirror images, as they already have their 'at' matrix reversed :|
  nat = nat * spin(playerturn[cpid]) * xpush(playergo[cpid]);

  cell *c2 = m->findbase(nat*C0);  
  if(c2 != m->base && !passable(c2, m->base, P_ISPLAYER | P_MIRROR))
    killMonster(m, moNone);
  else {
    m->rebasePat(nat);
    if(playerfire[cpid]) shootBullet(m);
    }  

  if(c2->wall == waCloud) {
    mirror::createMirages(c2, 0, moMirage);
    awakenMimics(m, c2);
    c2->wall = waNone;
    }

  if(c2->wall == waMirror) {
    mirror::createMirrors(c2, 0, moMirage);
    awakenMimics(m, c2);
    c2->wall = waNone;
    }

  if(c2->cpdist >= 6)
    m->dead = true;
  }

bool isPlayerOrImage(eMonster m) {
  return isMimic(m) || m == moPlayer;
  }

monster *parentOrSelf(monster *m) {
  return m->parent ? m->parent : m;
  }

bool verifyTeleport() {
  if(!on) return true;
  if(playerCrash(pc[cpid], mouseh)) return false;
  return true;
  }

void destroyMimics() {
  for(int i=0; i<int(active.size()); i++)
    if(isMimic(active[i]->type)) 
      active[i]->dead = true;
  }

void teleported() {
  monster *m = pc[cpid];
  m->base = cwt.c;
  m->at = rgpushxto0(inverse(gmatrix[cwt.c]) * mouseh) * spin(rand() % 1000 * M_PI / 2000);
  m->findpat();
  destroyMimics();
  }

void shoot(eItem it, monster *m) {
  monster* bullet = new monster;
  bullet->base = m->base;
  bullet->at = m->at * rspintox(inverse(m->pat) * mouseh);
  bullet->type = it == itOrbDragon ? moFireball : it == itOrbAir ? moAirball : moBullet;
  bullet->parent = m;
  bullet->pid = m->pid;
  bullet->parenttype = m->type;
  items[it]--;
  active.push_back(bullet);
  }

eItem targetRangedOrbKey(orbAction a) {
  hyperpoint h = mouseh;
  cell *b = mouseover;
  monster *mt = mousetarget;

  mouseh = keytarget(cpid);
  
  mouseover = pc[cpid]->base;
  
  while(true) {
    cell *c2 = findbaseAround(mouseh, mouseover);
    if(c2 == mouseover) break;
    mouseover = c2;
    }
  mousetarget = NULL;

  for(int j=0; j<int(active.size()); j++) {
    monster* m2 = active[j];
    if(m2->dead) continue;
    if(!mousetarget || intval(mouseh, mousetarget->pat*C0) > intval(mouseh, m2->pat*C0)) 
      mousetarget = m2;
    }

  eItem r = targetRangedOrb(mouseover, a);
  // printf("A%d i %d h %p t %p ov %s => %s\n", a, cpid, mouseover, mousetarget, display(mouseh), dnameof(r));
  
  mouseh = h;
  mousetarget = mt;
  mouseover = b;
  return r;
  }

eItem targetRangedOrb(orbAction a) {
  if(!on) return itNone;
  monster *wpc = pc[cpid];
  if(a != roCheck && !wpc) return itNone;  
  
  if(items[itOrbPsi] && shmup::mousetarget && intval(mouseh, shmup::mousetarget->pat*C0) < SCALE2 * .1) {
    if(a == roCheck) return itOrbPsi;
    addMessage(XLAT("You kill %the1 with a mental blast!", mousetarget->type));
    killMonster(mousetarget, moNone);
    items[itOrbPsi] -= 30;
    if(items[itOrbPsi]<0) items[itOrbPsi] = 0;
    return itOrbPsi;
    }
  
  if(items[itOrbStunning] && shmup::mousetarget && intval(mouseh, shmup::mousetarget->pat*C0) < SCALE2 * .1) {
    if(a == roCheck) return itOrbStunning;
    mousetarget->stunoff = curtime + 1000;
    items[itOrbStunning] -= 10;
    if(items[itOrbStunning]<0) items[itOrbStunning] = 0;
    return itOrbStunning;
    }
  
  if(on && items[itOrbDragon]) {
    if(a == roCheck) return itOrbDragon;
    shoot(itOrbDragon, wpc);
    return itOrbDragon;
    }
  
  if(on && items[itOrbAir]) {
    if(a == roCheck) return itOrbAir;
    shoot(itOrbAir, wpc);
    return itOrbAir;
    }
  
  if(on && items[itOrbIllusion]) {
    if(a == roCheck) return itOrbIllusion;
    shoot(itOrbIllusion, wpc);
    return itOrbIllusion;
    }

  return itNone;
  }

int speedfactor() {
  return items[itOrbSpeed]?2:1;
  }

void moveBullet(monster *m, int delta) {
  cpid = m->pid;
  m->findpat();
  transmatrix nat0 = m->pat;
  transmatrix nat = m->pat;

  if(isReptile(m->base->wall)) m->base->wparam = reptilemax();
    
  if(m->type == moFlailBullet) {
    m->vel -= delta  / speedfactor() / 600000.0;
    if(m->vel < 0 && m->parent) {
      // return to the flailer!
      nat = nat * rspintox(inverse(m->pat) * m->parent->pat * C0) * spin(M_PI);
      }
    }
  else if(m->type == moBullet)
    m->vel = 1/300.;
  else if(m->type == moFireball)
    m->vel = 1/500.;
  else if(m->type == moAirball)
    m->vel = 1/200.;
  else if(m->type == moTongue) {
    m->vel = 1/1500.;
    if(!m->parent || intval(nat*C0, m->parent->pat*C0) > SCALE2 * 0.4)
      m->dead = true;
    }
  nat = nat * xpush(delta * SCALE * m->vel / speedfactor());
  cell *c2 = m->findbase(nat*C0);

  if(isActivable(c2)) activateActiv(c2, true);
  
  // knives break mirrors and clouds
  if(c2->wall == waCloud) {
    mirror::createMirages(c2, 0, moMirage);
    awakenMimics(m, c2);
    c2->wall = waNone;
    }

  if(c2->wall == waMirror) {
    cwt.c = c2;
    mirror::createMirrors(c2, 0, moMirage);
    awakenMimics(m, c2);
    c2->wall = waNone;
    }
  
  bool godragon = m->type == moFireball && isDragon(c2->monst);
  
  if(m->type != moTongue && !(godragon || passable(c2, m->base, P_BULLET))) {
    m->dead = true;
    if(m->type != moAirball) killMonster(c2, m->parent ? m->parent->type : moNone);
    // cell *c = m->base;
    if(m->parent && isPlayer(m->parent)) {
      if(c2->wall == waBigTree) {
        addMessage(XLAT("You start chopping down the tree."));
        c2->wall = waSmallTree;
        }
      else if(c2->wall == waSmallTree) {
        addMessage(XLAT("You chop down the tree."));
        c2->wall = waNone;
        }
      else if(isActivable(c2)) 
        activateActiv(c2, true);
      }
    if(m->type == moFireball) {
      makeflame(c2, 20, false) || makeflame(m->base, 20, false);
      }
    }
  m->rebasePat(nat);
  
  // destroy stray bullets
  for(int i=0; i<m->base->type; i++) 
    if(!m->base->mov[i] || !gmatrix.count(m->base->mov[i]))
      m->dead = true;

  // items[itOrbWinter] = 100; items[itOrbLife] = 100;
  
  for(int j=0; j<int(active.size()); j++) {
    monster* m2 = active[j];
    if(m2 == m || (m2 == m->parent && m->vel >= 0) || m2->parent == m->parent) continue;
    
    // Flailers only killable by themselves
    if(m2->type == moFlailer && m2 != m->parent) continue;
    // be nice to your images! would be too hard otherwise...
    if(isPlayerOrImage(parentOrSelf(m)->type) && isPlayerOrImage(parentOrSelf(m2)->type) &&
      m2->pid == m->pid)
      continue;
    // fireballs/airballs don't collide
    if(m->type == moFireball && m2->type == moFireball) continue;
    if(m->type == moAirball && m2->type == moAirball) continue;
    double d = intval(m2->pat*C0, m->pat*C0);
    
    if(d < SCALE2 * 0.1) {
      if(m->type == moAirball && isBlowableMonster(m2->type)) {

        if(m2->blowoff < curtime) {
          hyperpoint h = inverse(m2->pat) * nat0 * C0;
          m2->swordangle += atan2(h[1], h[0]);
          m2->rebasePat(m2->pat * rspintox(h));
          }
        m2->blowoff = curtime + 1000;
        continue;
        }
      // Hedgehog Warriors only killable outside of the 45 degree angle
      if(m2->type == moHedge) {
        hyperpoint h = inverse(m2->pat) * m->pat * C0;
        if(h[0] > fabsl(h[1])) { m->dead = true; continue; }
        }
      // 
      if((m2->type == moPalace || m2->type == moFatGuard || m2->type == moSkeleton ||
        m2->type == moVizier || isMetalBeast(m2->type) || m2->type == moTortoise ||
        m2->type == moReptile) && m2->hitpoints > 1) {
        m2->rebasePat(m2->pat * rspintox(inverse(m2->pat) * nat0 * C0));
        if(m2->type != moSkeleton && !isMetalBeast(m2->type) && m2->type != moReptile) 
          m2->hitpoints--;
        m->dead = true;
        if(m2->type == moVizier) ;
        else if(m2->type == moFatGuard)
          m2->stunoff = curtime + 600;
        else if(m2->type == moMetalBeast || m2->type == moMetalBeast2)
          m2->stunoff = curtime + 3000;
        else if(m2->type == moReptile)
          m2->stunoff = curtime + 3000;
        else if(m2->type == moTortoise)
          m2->stunoff = curtime + 3000;
        else if(m2->type == moSkeleton && m2->base->land != laPalace)
          m2->stunoff = curtime + 2100;
        else
          m2->stunoff = curtime + 900;
        continue;
        }
      // conventional missiles cannot hurt some monsters
      bool conv = (m->type == moBullet || m->type == moFlailBullet || m->type == moTongue);

      if(m2->type == moGreater && conv) {
        m->dead = true;
        continue;
        }
      if(m2->type == moRoseBeauty && conv && !markOrb(itOrbBeauty)) {
        m->dead = true;
        continue;
        }
      if(m2->type == moDraugr && conv) {
        m->dead = true;
        continue;
        }
      if(isBull(m2->type) && conv) {
        m->dead = true;
        // enrage herd bulls, awaken sleeping bulls
        m2->type = moRagingBull;
        continue;
        }
      // Knights reflect bullets
      if(m2->type == moKnight) {
        if(m->parent) {
          nat = nat * rspintox(inverse(m->pat) * m->parent->pat * C0);
          m->rebasePat(nat);
          }
        m->parent = m2;
        continue;
        }
      m->dead = true;
      if(m->type == moFireball) makeflame(m->base, 20, false);
      // Orb of Winter protects from fireballs
      if(m->type == moFireball && ((isPlayer(m2) && markOrb(itOrbWinter)) || m2->type == moWitchWinter)) 
        continue;
      killMonster(m2, m->parent ? m->parent->type : moNone);
      }
    }
  }

hyperpoint closerTo;

bool closer(monster *m1, monster *m2) {
  return intval(m1->pat*C0,  closerTo) < intval(m2->pat*C0, closerTo);
  }

bool dragonbreath(cell *dragon) {
  int randplayer = hrand(numplayers());
  monster* bullet = new monster;
  bullet->base = dragon;
  bullet->at = rspintox(inverse(gmatrix[dragon]) * pc[randplayer]->pat * C0);
  bullet->type = moFireball;
  bullet->parent = bullet;
  bullet->pid = randplayer;
  active.push_back(bullet);
  return true;
  }

#define CHARGING (-777)
#define BULLSTUN (1500)

void moveMonster(monster *m, int delta) {

  bool stunned = m->stunoff > curtime || m->blowoff > curtime;
  
  if(stunned && cellUnstable(m->base))


  if(isReptile(m->base->wall)) m->base->wparam = reptilemax();
  
  if(m->base->wall == waChasm && !survivesChasm(m->type) && m->type != moReptile)
    killMonster(m, moNone, AF_FALL);

  if(m->base->wall == waRose && !survivesThorns(m->type))
    killMonster(m, moNone);

  if(isWatery(m->base) && !survivesWater(m->type) && !m->inBoat && m->type != moReptile)
    killMonster(m, moNone);

  if(isFire(m->base) && !survivesFire(m->type))
    killMonster(m, moNone);

  if(m->base->wall == waClosedGate && !survivesWall(m->type))
    killMonster(m, moNone);

  if(m->dead) return;
    
  cell *c = m->base;
  transmatrix goal = gmatrix[c];
  
  bool direct = false; // is there a direct path to the target?
  int directi = 0; // which player has direct path (to set as pid in missiles)
  
  double step = SCALE * delta/1000.0;
  if(m->type == moWitchSpeed)
    step *= 2;
  else if(m->type == moEagle)
    step *= 1.6;
  else if(m->type == moLancer)
    step *= 1.25;
  else if(isDemon(m->type)) {
    if(m->type == moLesserM) m->type = moLesser;
    if(m->type == moGreaterM) m->type = moGreater;
    step /= 2;
    }
  else if(m->type == moMetalBeast || m->type == moMetalBeast2) 
    step /= 2;
  else if(m->type == moTortoise)
    step /= 3;
  else if(isBull(m->type))
    step *= 1.5;
  
  if(items[itOrbBeauty]) {
    bool nearplayer = false;
    for(int pid=0; pid<players; pid++) {
      double dist = intval(pc[pid]->pat*C0, m->pat*C0);
      if(dist < SCALE2) nearplayer = true;
      }
    if(nearplayer) markOrb(itOrbBeauty), step /= 2;
    }

  transmatrix nat = m->pat;

  if(stunned) {
    if(m->blowoff > curtime) {
      step = SCALE * -delta / 1000.;
      }
    else if(m->type == moFatGuard || m->type == moTortoise || m->type == moRagingBull)
      step = 0;
    else if(m->type == moReptile)
      step = SCALE * -delta / 1000. * (m->stunoff - curtime) / 3000.;
    else step = SCALE * -delta/2000.;
    }
  
  else if(m->type == moRagingBull && m->stunoff == CHARGING) ;

  else {
  
    if(m->type == moSleepBull) {  
      for(int j=0; j<int(active.size()); j++) if(active[j]!=m && active[j]->type != moBullet) {
        monster* m2 = active[j];
        double d = intval(m2->pat*C0, nat*C0);
        if(d < SCALE2*3 && m2->type == moPlayer) m->type = moRagingBull;
        }
      }
    
    if(m->type == moWitchFlash) for(int pid=0; pid<players; pid++) {
      bool okay = intval(pc[pid]->pat*C0, m->pat*C0) < 2 * SCALE2;
      for(int i=0; i<int(active.size()); i++) {
        monster *m2 = active[i];
        if(m2 != m && isWitch(m2->type) && intval(m2->pat*C0, m->pat*C0) < 2 * SCALE2)
          okay = false;
        }
      if(okay) {
        addMessage(XLAT("%The1 activates her Flash spell!", m->type));
        pushmonsters();
        activateFlashFrom(m->base, moWitchFlash, AF_MAGIC | AF_GETPLAYER | AF_MSG);
        popmonsters();
        m->type = moWitch;
        pc[pid]->dead = true;
        }
      }
    if(isBug(m->type)) {
      vector<monster*> bugtargets;
      for(int i=0; i<int(active.size()); i++)
        if(!isBullet(active[i]))
        if(active[i]->type != m->type)
        if(!isPlayer(active[i]) || !invismove)
        if(!active[i]->dead)
          bugtargets.push_back(active[i]);
      
      closerTo = m->pat * C0;
      sort(bugtargets.begin(), bugtargets.end(), closer);
  
      for(int i=0; i<int(bugtargets.size()); i++)
        if(m->trackroute(bugtargets[i]->pat, step)) {
          goal = bugtargets[i]->pat;
          direct = true;
          break;
          }
      }
    else if(m->type == moWolf) {
      cell *cnext = c;
      for(int i=0; i<c->type; i++) {
        cell *c2 = c->mov[i];
        if(c2 && gmatrix.count(c2) && HEAT(c2) > HEAT(c) && isIcyLand(c2) && passable(c2, c, 0))
          cnext = c2;
        }
      goal = gmatrix[cnext];
      direct = true;
      directi = 0;
      }
    else if(m->type == moHerdBull) {
      cell *cnext = prairie::next(c);
      if(cnext && gmatrix.count(cnext)) {
        goal = gmatrix[cnext];
        direct = true;
        directi = 0;
        }
      else m->dead = true;
      }
    else if(m->type == moButterfly) {
      int d = neighborId(m->base, m->torigin);
      cell *cnext = NULL;
      for(int u=2; u<m->base->type; u++) {
        cell *c2 = createMov(m->base, (d+u) % m->base->type);
        if(passable_for(m->type, c2, m->base, P_ONPLAYER)) {
          cnext = c2;
          break;
          }
        }

      if(cnext && gmatrix.count(cnext)) {
        goal = gmatrix[cnext];
        direct = true;
        directi = 0;
        }
      }
    else if(!direct && !invismove) {
      for(int i=0; i<players; i++) 
        if(m->trackroute(pc[i]->pat, step) && (!direct || intval(pc[i]->pat*C0, m->pat*C0) < intval(goal*C0,m->pat*C0))) {
          goal = pc[i]->pat;
          direct = true;
          directi = i;
          // m->trackrouteView(pc->pat, step);
          }
        }
  
    if(!direct) while(true) {
      if(m->trackroute(gmatrix[c], step))
        goal = gmatrix[c];
      cell *cnext = c;
      for(int i=0; i<c->type; i++) {
        cell *c2 = c->mov[i];
        if(c2 && gmatrix.count(c2) && c2->pathdist < c->pathdist &&
          passable_for(m->type, c2, c, P_CHAIN | P_ONPLAYER))
          cnext = c2;
        }
      if(cnext == c) break;
      c = cnext;
      }

    if(m->type == moHedge) {
      hyperpoint h = inverse(m->pat) * goal * C0;
      if(h[1] < 0)
        nat = nat * spin(M_PI * delta / 3000 / speedfactor());
      else
        nat = nat * spin(M_PI * -delta / 3000 / speedfactor());
      m->rebasePat(nat);
      // at most 45 degrees
      if(h[0] < fabsl(h[1])) return;
      }
    else {
      nat = nat * rspintox(inverse(m->pat) * goal * C0);
      }
    }
  
  if(m->type == moVampire) for(int i=0; i<players; i++) 
  if(intval(m->pat*C0, pc[i]->pat*C0) < SCALE2 * 2) {
    for(int i=0; i<ittypes; i++)
      if(itemclass(eItem(i)) == IC_ORB && items[i] && items[itOrbTime] && !orbused[i])
        orbused[i] = true;
    step = 0;
    }

  bool carried = false;

  if(c->land == laWhirlpool && (m->type == moShark || m->type == moCShark || m->type == moPirate))
    oceanCurrents(nat, m, delta), carried = true;

  if(m->base->land == laWhirlwind) 
    airCurrents(nat, m, delta), carried = true;
  
  if(rosedist(m->base) == 1)
    roseCurrents(nat, m, delta), carried = true;

  step /= speedfactor();
  
  int igo = 0;
  
  transmatrix nat0 = nat;
  
  igo_retry:
  
  if(igo == IGO) {
    if(m->type == moHerdBull) m->type = moRagingBull;
    return;
    }
  
  if(igo == 1 && m->type == moRagingBull && m->stunoff == CHARGING) {
    m->stunoff = curtime + BULLSTUN;
    return;
    }
  
  nat = nat0 * spin(igospan[igo]) * xpush(step) * spin(-igospan[igo]);; // * spintox(wherePC);

  if(m->type != moRagingBull)
  if(intval(nat*C0, goal*C0) >= intval(m->pat*C0, goal*C0) && !stunned && !carried) {
    igo++; goto igo_retry; }

  for(int i=0; i<multi::players; i++) for(int b=0; b<2; b++) if(sword::orbcount(b)) {  
    hyperpoint H = swordpos(i, b, 1);
    double d = intval(H, nat*C0);
    if(d < SCALE2 * 0.12) { igo++; goto igo_retry; }
    }

  m->footphase += step;

  monster* crashintomon = NULL;
  
  for(int j=0; j<int(active.size()); j++) if(active[j]!=m && active[j]->type != moBullet) {
    monster* m2 = active[j];
    double d = intval(m2->pat*C0, nat*C0);
    if(d < SCALE2 * 0.1) crashintomon = active[j];
    }
  
  for(int i=0; i<players; i++) 
    if(crashintomon == pc[i]) 
      pc[i]->dead = true;

  else if(crashintomon && isMimic(crashintomon->type)) {
    killMonster(crashintomon, m->type);
    crashintomon = NULL;
    }
  
  else if(crashintomon && (
    items[itOrbDiscord] || isBull(m->type) ||
       ((isBug(m->type) || isBug(crashintomon->type)) && m->type != crashintomon->type)) 
    && !isBullet(crashintomon)) {
    if(noncrashable(crashintomon, m)) {
      if(isBull(crashintomon->type)) crashintomon->type = moRagingBull;
      }
    else {
      killMonster(crashintomon, m->type, isBull(m->type) ? AF_BULL : 0);
      crashintomon = NULL;
      }
    }
  
  if(crashintomon) { igo++; goto igo_retry; }

  cell *c2 = m->findbase(nat*C0);

  if(m->type == moButterfly && !passable_for(m->type, c2, m->base, P_CHAIN)) {
    igo++; goto igo_retry;
    }

  if(isPlayerOn(c2)) {
    bool usetongue = false;
    if(isSlimeMover(m->type) || m->type == moWaterElemental) usetongue = true;
    if(isWatery(c2) && !survivesWater(m->type) && !m->inBoat) usetongue = true;
    if(c2->wall == waChasm && !survivesChasm(m->type)) usetongue = true;
    if(isFire(c2) && !survivesFire(m->type) && !m->inBoat) usetongue = true;
    if(isBird(m->type) && !passable_for(moEagle, c2, c, 0)) usetongue = true;
    if(usetongue) {
      if(curtime < m->nextshot) return;
      // m->nextshot = curtime + 25;
      monster* bullet = new monster;
      bullet->base = m->base;
      bullet->at = m->at;
      bullet->type = moTongue;
      bullet->parent = m;
      bullet->parenttype = m->type;
      bullet->pid = whichPlayerOn(c2);
      active.push_back(bullet);
      return;
      }
    }

  if(!ignoresPlates(m->type)) destroyWeakBranch(m->base, c2, m->type);

  if(c2 != m->base && (c2->wall == waClosePlate || c2->wall == waOpenPlate) && !ignoresPlates(m->type))
    toggleGates(c2, c2->wall, 3);
  
  if(c2 != m->base && mayExplodeMine(c2, m->type)) 
    killMonster(m, moNone);
  
  if(c2 != m->base && c2->wall == waRose && !nonAdjacent(m->base, c2) && !survivesThorns(m->type))
    killMonster(m, moNone);
  
  if(c2 != m->base && cellUnstable(m->base) && !ignoresPlates(m->type))


  if(c2 != m->base && m->type == moWitchFire) makeflame(m->base, 10, false);
  if(c2 != m->base && m->type == moFireElemental) makeflame(m->base, 20, false);
  if(c2 != m->base && m->type == moWaterElemental) placeWater(c2, m->base);
  if(c2 != m->base && m->type == moEarthElemental) {
    int d = dirfromto(m->base, c2);
    if(d >= 0) earthMove(m->base, d);
    }
  
  if(m->type == moReptile && c2 != m->base) {
    if(c2->wall == waChasm) {
      c2->wall = waReptile;
      c2->wparam = reptilemax();

      m->dead = true;
      }
    else if(isChasmy(c2) || isWatery(c2)) {
      c2->wall = waReptileBridge;
      c2->item = itNone;
      c2->wparam = reptilemax();

      m->dead = true;
      }
    }
  
  if(c2 != m->base && m->type == moNecromancer && !c2->monst) {
    for(int i=0; i<m->base->type; i++) {
      cell *c3 = m->base->mov[i];
      if(dirfromto(c3, c2) != -1 && c3->wall == waFreshGrave && gmatrix.count(c3)) {
        bool monstersNear = false;
        for(int i=0; i<int(active.size()); i++) {
          if(active[i] != m && intval(active[i]->pat*C0, gmatrix[c3]*C0) < SCALE2 * .3)
            monstersNear = true;
          if(active[i] != m && intval(active[i]->pat*C0, gmatrix[c2]*C0) < SCALE2 * .3)
            monstersNear = true;
          }
        if(!monstersNear) {

          monster* undead = new monster;
          undead->base = c2;
          undead->at = Id;
          undead->type = moZombie;
          undead->parent = m;
          undead->parenttype = m->type;
          undead->pid = 0;
          undead->findpat();
          active.push_back(undead);

          undead = new monster;
          undead->base = c3;
          undead->at = Id;
          undead->type = moGhost;
          undead->parent = m;
          undead->parenttype = m->type;
          undead->findpat();
          undead->pid = 0;
          active.push_back(undead);

          c3->wall = waAncientGrave;
          addMessage(XLAT("%The1 raises some undead!", m->type));
          }
        }
      }
    }
  if(m->type == moGreaterShark) {
    if(c2->wall == waBoat)
      c2->wall = waNone;
    if(c2->wall == waFrozenLake)
      c2->wall = waLake;
    }
  if(m->type == moDarkTroll && c2->wall == waCavefloor) {
    m->type = moTroll;
    }
  if(isLeader(m->type)) {
    if(c2 != m->base) {
      if(c2->wall == waBigStatue && canPushStatueOn(m->base)) {
        c2->wall = m->base->wall;
        if(cellUnstable(m->base))
          m->base->wall = waChasm;
        else
          m->base->wall = waBigStatue;
        animateMovement(c2, m->base, LAYER_BOAT);
        }
      if(passable_for(m->type, c2, m->base, P_CHAIN | P_ONPLAYER) && !isWatery(c2) && m->inBoat) {
        if(isWatery(m->base)) 
          m->base->wall = waBoat, m->base->mondir = dirfromto(m->base, c2);
        else if(boatStrandable(c2)) c2->wall = waStrandedBoat;
        else if(boatStrandable(m->base)) m->base->wall = waStrandedBoat;
        m->inBoat = false;
        }
      if(isWatery(c2) && isWatery(m->base) && m->inBoat)
        moveItem(m->base, c2, true);
      }
    if(c2->wall == waBoat && !m->inBoat) {
      m->inBoat = true; c2->wall = waSea;
      m->base = c2;
      }
    }
  
  if(!(m->type == moRoseBeauty && c2->land != laRose)) {
    if(stunned ? passable(c2, m->base, P_BLOW) : passable_for(m->type, c2, m->base, P_CHAIN)) {
      if(c2 != m->base && m->type == moButterfly) 
        m->torigin = m->base;
      m->rebasePat(nat);
      if(m->type == moRagingBull && step > 1e-6) m->stunoff = CHARGING;
      }
    else if(m->type == moRagingBull && m->stunoff == CHARGING)
      m->stunoff = curtime + BULLSTUN;
    }

  if(direct) {
    if((m->type == moPyroCultist || m->type == moCrystalSage) && curtime >= m->nextshot) {
      monster* bullet = new monster;
      bullet->base = m->base;
      bullet->at = m->at;
      bullet->type = moFireball;
      bullet->parent = m;
      active.push_back(bullet);
      bullet->pid = directi;
      if(m->type == moPyroCultist) 
        m->type = moCultist;
      else
        m->nextshot = curtime + 100;
      }
    if(m->type == moOutlaw && curtime >= m->nextshot) {
      monster* bullet = new monster;
      bullet->base = m->base;
      bullet->at = m->at;
      bullet->type = moBullet;
      bullet->parent = m;
      bullet->parenttype = moOutlaw;
      bullet->pid = directi;
      active.push_back(bullet);
      m->nextshot = curtime + 1500;
      }
    for(int i=0; i<players; i++)
    if((m->type == moAirElemental) && curtime >= m->nextshot && intval(m->pat*C0, pc[i]->pat*C0) < SCALE2 * 2) {
      monster* bullet = new monster;
      bullet->base = m->base;
      bullet->at = m->at;
      bullet->type = moAirball;
      bullet->parent = m;
      bullet->pid = i;
      active.push_back(bullet);
      m->nextshot = curtime + 1500;
      }
    for(int i=0; i<players; i++)
      if(m->type == moTortoise && tortoise::seek() && !tortoise::diff(getBits(m->torigin)) && intval(m->pat*C0, pc[i]->pat*C0) < SCALE2) {
        items[itBabyTortoise] += 4;
        m->dead = true;
        addMessage(XLAT(playergender() == GEN_F ? "You are now a tortoise heroine!" : "You are now a tortoise hero!"));
        }
    for(int i=0; i<players; i++)
    if(m->type == moFlailer && curtime >= m->nextshot && 
      intval(m->pat*C0, pc[i]->pat*C0) < SCALE2 * 2) {
      m->nextshot = curtime + 3500;
      monster* bullet = new monster;
      bullet->base = m->base;
      bullet->at = m->at;
      bullet->type = moFlailBullet;
      bullet->parent = m;
      bullet->vel = 1/400.0;
      bullet->pid = i;
      active.push_back(bullet);
      break;
      }
    }
  }

void recall() {
  for(int i=0; i<players; i++) {
    pc[i]->base = cwt.c;
    if(players == 1)
      pc[i]->at = Id;
    else
      pc[i]->at = spin(2*M_PI*i/players) * xpush(firstland == laMotion ? .5 : .3) * Id;
    /* ggmatrix(cwt.c);
    display(gmatrix[cwt.c]);
    pc[i]->findpat(); */
    }
  destroyMimics();
  }

void init() {

  for(int i=0; i<players; i++) pc[i] = NULL;
  
  for(int i=0; i<players; i++) {
    pc[i] = new monster;
    pc[i]->type = moPlayer;
    pc[i]->pid = i;
    if(players == 1)
      pc[i]->at = Id;
    else
      pc[i]->at = spin(2*M_PI*i/players) * xpush(firstland == laMotion ? .5 : .3) * Id;
    pc[i]->base = cwt.c;
    pc[i]->inBoat = (firstland == laCaribbean || firstland == laOcean || firstland == laLivefjord ||
      firstland == laWhirlpool);
    pc[i]->store();
    }
  
  if(!safety) {
    items[itOrbLife] = 3;
    addMessage(XLAT("Welcome to the Shoot'em Up mode!"));
    // addMessage(XLAT("F/;/Space/Enter/KP5 = fire, WASD/IJKL/Numpad = move"));
    }
  else safety = false;
  }

bool boatAt(cell *c) {
  pair<mit, mit> p = 
    monstersAt.equal_range(c);
  for(mit it = p.first; it != p.second; it++) {
    monster* m = it->second;
    if(m->inBoat) return true;
    }
  return false;
  }

bool drawMonster(const transmatrix& V, cell *c, const transmatrix*& Vboat, transmatrix& Vboat0, ld zlev) {

   pair<mit, mit> p = 
    monstersAt.equal_range(c);

  for(mit it = p.first; it != p.second; it++) {
    monster* m = it->second;
    m->pat = ggmatrix(m->base) * m->at;
    transmatrix view = V * m->at;
    
    if(!outofmap(mouseh)) {
      if(mapeditor::drawplayer || m->type != moPlayer)
      if(!mousetarget || intval(mouseh, mousetarget->pat*C0) > intval(mouseh, m->pat*C0)) 
        mousetarget = m;
      }
    
    if(m->inBoat) {
      view = m->pat;
      Vboat = &(Vboat0 = view);
      if(m->type == moPlayer && items[itOrbWater]) {
        queuepoly(m->pat, shBoatOuter, watercolor(0));
        queuepoly(m->pat, shBoatInner, 0x0060C0FF);
        }
      else {
        queuepoly(m->pat, shBoatOuter, 0xC06000FF);
        queuepoly(m->pat, shBoatInner, 0x804000FF);
        }
      }

    if(doHighlight())
      poly_outline = 
        isBullet(m) ? 0x00FFFFFF :
        (isFriendly(m->type) || m->type == moPlayer) ? 0x00FF00FF : 0xFF0000FF;

    int q = ptds.size();
    if(q != int(ptds.size()) && !m->inBoat) pushdown(c, q, view, zlev, true, false);

    switch(m->type) {
      case moPlayer: 
        cpid = m->pid; 
        drawPlayerEffects(view, c, true);
        if(m->inBoat) m->footphase = 0;
        if(mapeditor::drawplayer) drawMonsterType(moPlayer, c, view, 0xFFFFFFC0, m->footphase);
        
        if(keyresult[cpid]) {
          hyperpoint h = keytarget(cpid);
          queuechr(h, vid.fsize, '+', iinf[keyresult[cpid]].color);
          }
        break;
      case moBullet: {
        int col;
        cpid = m->pid;
        if(m->parenttype == moPlayer)
          col = getcs().swordcolor;
        else
          col = (minf[m->parenttype].color << 8) | 0xFF;
        if(getcs().charid >= 4) {
          queuepoly(mmscale(view, 1.15), shPHead, col);
          ShadowV(view, shPHead);
          }
        else {
          transmatrix t = view * spin(curtime / 50.0);
          queuepoly(mmscale(t, 1.15), shKnife, col);
          ShadowV(t, shKnife);
          }
        break;
        }
      case moTongue: {
        queuepoly(mmscale(view, 1.15), shTongue, (minf[m->parenttype].color << 8) | 0xFF);
        ShadowV(view, shTongue);
        break;
        }
      case moFireball:  case moAirball: { // case moLightningBolt:
        queuepoly(mmscale(view, 1.15), shPHead, (minf[m->type].color << 8) | 0xFF);
        ShadowV(view, shPHead);
        break;
        }
      case moFlailBullet: {
        transmatrix t = view * spin(curtime / 50.0);
        queuepoly(mmscale(t, 1.15), shFlailMissile, (minf[m->type].color << 8) | 0xFF);
        ShadowV(view, shFlailMissile);
        break;
        }


      default:
        if(m->inBoat) m->footphase = 0;
        int col = minf[m->type].color;
        if(m->type == moSlime) {
          col = winf[c->wall].color;
          col |= (col >> 1);
          }
        cpid = m->pid; 
        if(m->stunoff > curtime)
          c->stuntime = 1 + (m->stunoff - curtime-1)/300;
        if(hasHitpoints(m->type))
          c->hitpoints = m->hitpoints;
        if(m->type == moTortoise) tortoise::emap[c] = m->torigin;
        drawMonsterType(m->type, c, view, col, m->footphase);
        if(m->type == moTortoise) tortoise::emap.erase(c);
        break;
      }
    }

  return false;
  }

void clearMonsters() {
  for(mit it = monstersAt.begin(); it != monstersAt.end(); it++)
    delete(it->second);
  for(int i=0; i<int(active.size()); i++) delete active[i];
  monstersAt.clear();
  active.clear();
  }

void clearMemory() {
  clearMonsters();
  gmatrix.clear();
  curtime = 0;
  nextmove = 0;
  nextdragon = 0;
  visibleAt = 0;
  }

cell *playerpos(int i) {
  return pc[i]->base;
  }

bool playerInBoat(int i) {
  return pc[i]->inBoat;
  }

void destroyBoats(cell *c) {
  for(int i=0; i<int(active.size()); i++)
    if(active[i]->base == c && active[i]->inBoat)
      active[i]->inBoat = false;
  }

transmatrix calc_relative_matrix(cell *c, heptagon *h1) {
  transmatrix gm = Id;
  heptagon *h2 = c->master;
  transmatrix where = Id;
  for(int d=0; d<7; d++) if(h2->c7->mov[d] == c)
    where = hexmove[d];
  // always add to last!
//bool hsol = false;
//transmatrix sol;
  while(h1 != h2) {
    for(int d=0; d<7; d++) if(h2->move[d] == h1) {
      int sp = h2->spin(d);
      return gm * heptmove[sp] * spin(2*M_PI*d/7) * where;
      }
    if(h1->distance < h2->distance) {
      int sp = h2->spin(0);
      h2 = h2->move[0];
      where = heptmove[sp] * where;
      }
    else {
      int sp = h1->spin(0);
      h1 = h1->move[0];
      gm = gm * invheptmove[sp];
      }
    }
/*if(hsol) {
    transmatrix sol2 = gm * where;
    for(int i=0; i<3; i++) for(int j=0; j<3; j++)
      if(fabs(sol2[i][j]-sol[i][j] > 1e-3)) {
        printf("ERROR\n");
        display(sol);
        display(sol2);
        exit(1);
        }
    } */
  return gm * where;
  }

    transmatrix ztmp;
    
transmatrix &ggmatrix(cell *c) {
  transmatrix& t = gmatrix[c];
  if(t[2][2] == 0) {
    if(sphere && gmatrix0.count(c))
      t = gmatrix[cwt.c] * inverse(gmatrix0[cwt.c]) * gmatrix0[c];
    else if(sphere) {
      printf("error: gmatrix0 not known\n");
      exit(1);
      }
    else if(euclid) {
      eucoord xh, yh, xc, yc;
      decodeMaster(c->master, xh, yh);
      decodeMaster(centerover->master, xc, yc);
      short xd(xh-xc), yd(yh-yc);
      t = gmatrix[centerover] * eumove(xd, yd);
      /* printf("H[%d,%d] C[%d,%d] ", int(xh),int(yh), int(xc),int(yc));
      printf("%d,%d t = \n", xd, yd); display(t);
      printf("gmatrix0 = \n");
      display(gmatrix0[c]); */
      }
    else {
      t = 
        View * spin(viewctr.spin * 2 * M_PI / S7) * calc_relative_matrix(c, viewctr.h);
      if(purehepta) t = t * pispin;
      }
    }
  return t;
  }

transmatrix calc_relative_matrix_help(cell *c, heptagon *h1) {
  transmatrix gm = Id;
  heptagon *h2 = c->master;
  transmatrix where = Id;
  if(!purehepta) for(int d=0; d<7; d++) if(h2->c7->mov[d] == c)
    where = hexmove[d];
  // always add to last!
  while(h1 != h2) {
    for(int d=0; d<7; d++) if(h1->move[d] == h2) printf("(adj) ");
    if(h1->distance < h2->distance) {
      int sp = h2->spin(0);
      printf("A%d ", sp);
      h2 = h2->move[0];
      where = heptmove[sp] * where;
      }
    else {
      int sp = h1->spin(0);
      printf("B%d ", sp);
      h1 = h1->move[0];
      gm = gm * invheptmove[sp];
      }
    }
  printf("OK\n");
  display(gm * where);
  return gm * where;
  }

void virtualRebase(shmup::monster *m, bool tohex) {

  if(euclid || sphere) return;

  while(true) {
  
    if(m->base->type == 6) {
      cell *c7 = m->base->master->c7;
      for(int d=0; d<7; d++) if(c7->mov[d] == m->base)
        m->at = hexmove[d] * m->at;
      m->base = c7;
      }
    
    double currz = (m->at * C0)[2];
    
    heptagon *h = m->base->master;
    
    cell *newbase = NULL;
    
    transmatrix bestV;
    
    for(int d=0; d<7; d++) {
      heptspin hs;
      hs.h = h;
      hs.spin = d;
      heptspin hs2 = hsstep(hs, 0);
      transmatrix V2 = spin(-hs2.spin*2*M_PI/7) * invheptmove[d];
      double newz = (V2 *m->at * C0) [2];
      if(newz < currz) {
        currz = newz;
        bestV = V2;
        newbase = hs2.h->c7;
        }
      }
    
    if(!newbase) {
      if(tohex && !purehepta) for(int d=0; d<7; d++) {
        cell *c = createMov(m->base, d);
        transmatrix V2 = spin(-m->base->spn(d)*2*M_PI/6) * invhexmove[d];
        double newz = (V2 *m->at * C0) [2];
        if(newz < currz) {
          currz = newz;
          bestV = V2;
          newbase = c;
          }
        }
      if(newbase) {
        m->base = newbase;
        m->at = bestV * m->at;
        }
      break;
      }
    
    // printf("%p: rebase %p -> %p\n", m, m->base, newbase);
    m->base = newbase;
    m->at = bestV * m->at;
    }

  }

}

