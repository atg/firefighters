struct Preferences {
    bool playerAngleFollowsMouse;
    
    Preferences()
      : playerAngleFollowsMouse(true) { }
};

static Preferences PREFS = Preferences();
