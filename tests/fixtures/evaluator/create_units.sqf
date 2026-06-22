grp = createGroup west;
u1 = createUnit ["SyntheticSoldierWest", [100,0,200], grp];
u2 = createUnit ["SyntheticOfficerWest", [110,0,200], grp];
units_arr = units grp;
hint format ["units: %1", count units_arr];
hint format ["leader type: %1", typeOf (leader grp)];
hint format ["u1 alive: %1", alive u1];
u1 setDammage 1.0;
hint format ["u1 alive after kill: %1", alive u1]
