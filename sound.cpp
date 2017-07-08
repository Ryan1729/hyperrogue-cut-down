bool audio;
string musiclicense;
string musfname[landtypes];
int musicvolume = 60, effvolume = 60;

eLand getCurrentLandForMusic() {
  eLand id = cwt.c->land;
  if(isHaunted(id)) id = laHaunted;
  if(id == laWarpSea) id = laWarpCoast;
  return id;
  }

void resetmusic() {}

