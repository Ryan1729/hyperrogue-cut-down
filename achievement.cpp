// Hyperbolic Rogue -- achievements
// Copyright (C) 2011-2016 Zeno Rogue, see 'hyper.cpp' for details

#define NUMLEADER 69

#define SCORE_UNKNOWN (-1)
#define NO_SCORE_YET (-2)

bool offlineMode = false;

int syncstate = 0;

int currentscore[NUMLEADER];

const char* leadernames[NUMLEADER] = {
  "Score", "Diamonds", "Gold", "Spice", "Rubies", "Elixirs",
  "Shards", "Totems", "Daisies", "Statues", "Feathers", "Sapphires",
  "Hyperstones", "Time to Win-71", "Turns to Win-71",
  "Time to 10 Hyperstones-94", "Turns to 10 Hyperstones-94", "Orbs of Yendor",
  "Fern Flowers", 
  "Royal Jellies", "Powerstones", "Silver", "Wine", "Emeralds", "Grimoires",
  "Holy Grails", "Red Gems", "Pirate Treasures",
  "Shmup Score", "Shmup Time to Win", "Shmup Knife to Win",
  "Bomberbird Eggs", // 31
  "Ambers", // 32
  "Pearls", // 33
  "Hypersian Rugs", // 34
  "Garnets", // 35
  "Princess Challenge", // 36
  "Ivory Figurines", // 37
  "Elemental Gems", // 38
  "Onyxes", // 39
  "Yendor Challenge", // 40
  "Pure Tactics Mode", // 41
  "Mutant Saplings", // 42
  "Fulgurites", // 43
  "Shmup Score 2p", // 44
  "Coop Shmup Time to Win", // 45
  "Black Lotuses", // 46
  "Mutant Fruits", // 47
  "White Dove Feathers", // 48
  "Pure Tactics Mode (shmup)", // 49
  "Pure Tactics Mode (2p)", // 50
  "Corals", // 51
  "Thornless Roses", // 52
  "Chaos Mode", // 53
  "Tortoise points", // 54
  "Dragon Scales", // 55
  "Apples", // 56
  "Heptagonal Mode", // 57
  "Sunken Treasures", // 58
  "Ancient Jewelry", // 59
  "Golden Eggs", // 60
  "Multiplayer Score", // 61
  "Statistics", // 62
  "Halloween", // 63
  "Amethysts", // 64
  "Slime Molds", // 65
  "Dodecahedra", // 66
  "Green Grass", // 67
  "Spinel" // 68
  };

#define LB_STATISTICS 62
#define LB_HALLOWEEN  63

bool haveLeaderboard(int id);

void upload_score(int id, int v);

string achievementMessage[3];
int achievementTimer;
// vector<string> achievementsReceived;

bool wrongMode(char flags) {
  if(cheater) return true;
  if(flags == 'x') return false;
  if(purehepta != (flags == '7')) return true;
  if(euclid != (flags == 'e')) return true;
  if(sphere != (flags == 'E')) return true;
  if((quotient == 1) != (flags == 'q')) return true;
  if((quotient == 2) != (flags == 'Q')) return true;
  if(shmup::on != (flags == 's')) return true;
  if(randomPatternsMode) return true;
  if(yendor::on) return true;
  if(tactic::on) return true;
  if(tour::on) return true;
  if(chaosmode != (flags == 'C')) return true;
  if((numplayers() > 1) != (flags == 'm')) return true;
  return false;
  }

void achievement_log(const char* s, char flags) {

  
  if(wrongMode(flags)) return;
  
  for(int i=0; i<int(achievementsReceived.size()); i++)
    if(achievementsReceived[i] == s) return;
  achievementsReceived.push_back(s);
  
#ifndef NOSAVE
  remove_emergency_save();

  FILE *f = fopen(scorefile, "at");
  if(!f) return;
  
  int t = (int) (time(NULL) - timerstart);

  time_t timer = time(NULL);
  char buf[128]; strftime(buf, 128, "%c", localtime(&timer));

  fprintf(f, "ACHIEVEMENT %s turns: %d time: %d at: %d ver: %s c: %d date: %s\n",
    s, turncount, t, int(timerstart), VER, anticheat::certify(s, turncount, t, (int) timerstart, 0), buf);

  fclose(f);
#endif
  }

void achievement_init() {}
void achievement_close() {}
void achievement_gain(const char* s, char flags) {
  achievement_log(s, flags);
  }

void achievement_count(const string& s, int current, int prev) {
  if(cheater) return;
  if(shmup::on) return;
  if(randomPatternsMode) return;
  if(s == "GOLEM" && current >= 5)
    achievement_gain("GOLEM2");
  if(s == "GOLEM" && current >= 10)
    achievement_gain("GOLEM3");
  if(s == "STAB" && current >= 1)
    achievement_gain("STABBER1");
  if(s == "STAB" && current >= 2)
    achievement_gain("STABBER2");
  if(s == "SLASH" && current >= 2)
    achievement_gain("SLASH2");
  if(s == "STAB" && current >= 4)
    achievement_gain("STABBER3");
  if(s == "MIRRORKILL" && current-prev >= 1)
    achievement_gain("MIRRORKILL1");
  if(s == "MIRRORKILL" && current-prev >= 2)
    achievement_gain("MIRRORKILL2");
  if(s == "MIRRORKILL" && current-prev >= 3)
    achievement_gain("MIRRORKILL3");
  if(s == "FLASH" && current-prev >= 1)
    achievement_gain("FLASH1");
  if(s == "FLASH" && current-prev >= 5)
    achievement_gain("FLASH2");
  if(s == "FLASH" && current-prev >= 10)
    achievement_gain("FLASH3");
  if(s == "LIGHTNING" && current-prev >= 1)
    achievement_gain("LIGHTNING1");
  if(s == "LIGHTNING" && current-prev >= 5)
    achievement_gain("LIGHTNING2");
  if(s == "LIGHTNING" && current-prev >= 10)
    achievement_gain("LIGHTNING3");
  if(s == "MIRAGE" && current >= 35)
    achievement_gain("MIRAGE", 'e');
  if(s == "ORB" && current >= 10)
    achievement_gain("ORB3");
  if(s == "BUG" && current >= 1000)
    achievement_gain("BUG3");
  if(s == "ELEC" && current >= 10)
    achievement_gain("ELEC3");
  }

int specific_improved = 0;
int specific_what = 0;

#ifndef HAVE_ACHIEVEMENTS
void achievement_pump() {}
#endif

void achievement_display() {
  #ifdef HAVE_ACHIEVEMENTS
  if(achievementTimer) {
    int col = (ticks - achievementTimer);
    if(col > 5000) { achievementTimer = 0; return; }
    if(col > 2500) col = 5000 - col;
    col /= 10; col *= 0x10101;
    displayfr(vid.xres/2, vid.yres/4, 2, vid.fsize * 2, achievementMessage[0], col & 0xFFFF00, 8);
    int w = 2 * vid.fsize;
    while(w>3 && textwidth(w, achievementMessage[1]) > vid.xres) w--;
    displayfr(vid.xres/2, vid.yres/4 + vid.fsize*2, 2, w, achievementMessage[1], col, 8);
    w = vid.fsize;
    while(w>3 && textwidth(w, achievementMessage[2]) > vid.xres) w--;
    displayfr(vid.xres/2, vid.yres/4 + vid.fsize*4, 2, w, achievementMessage[2], col, 8);
    }
  #endif
  }

bool isAscending(int i) { 
  return i == 13 || i == 14 || i == 15 || i == 16 || i == 29 || i == 30 || i == 45;
  };

