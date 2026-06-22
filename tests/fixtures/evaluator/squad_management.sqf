_squad = createGroup west;
_types = ["SyntheticOfficerWest", "SyntheticSoldierWest", "SyntheticSoldierWest", "SyntheticSoldierWest"];
_positions = [[0,0,0], [5,0,0], [10,0,0], [15,0,0]];

_i = 0;
while {_i < count _types} do {
    _type = _types select _i;
    _pos = _positions select _i;
    _unit = createUnit [_type, _pos, _squad];
    _i = _i + 1;
};

_allUnits = units _squad;
hint format ["Squad size: %1", count _allUnits];
hint format ["Leader: %1", typeOf (leader _squad)];

_roles = ["Commander", "Rifleman", "Rifleman", "MG"];
_i = 0;
while {_i < count _allUnits} do {
    (_allUnits select _i) setVariable ["role", _roles select _i];
    _i = _i + 1;
};

_cmdUnit = _allUnits select 0;
_mgUnit = _allUnits select 3;
hint format ["Unit 0 role: %1", _cmdUnit getVariable "role"];
hint format ["Unit 3 role: %1", _mgUnit getVariable "role"];

(_allUnits select 1) setDammage 0.5;
(_allUnits select 2) setDammage 1.0;

_aliveCount = _allUnits call {
    private "_count";
    _count = 0;
    _j = 0;
    while {_j < count _this} do {
        if (alive (_this select _j)) then {_count = _count + 1};
        _j = _j + 1;
    };
    _count
};

hint format ["Alive: %1 of %2", _aliveCount, count _allUnits];

_leaderUnit = leader _squad;
_lastUnit = _allUnits select ((count _allUnits) - 1);
_dist = _leaderUnit distance _lastUnit;
hint format ["Squad spread: %1m", _dist]
