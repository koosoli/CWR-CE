_distanceBetween = {
    private ["_p1", "_p2"];
    _p1 = getPos (_this select 0);
    _p2 = getPos (_this select 1);
    _dx = (_p1 select 0) - (_p2 select 0);
    _dy = (_p1 select 1) - (_p2 select 1);
    _dz = (_p1 select 2) - (_p2 select 2);
    _dx * _dx + _dy * _dy + _dz * _dz
};

_obj1 = createVehicle ["Flag", [0,0,0]];
_obj2 = createVehicle ["Flag", [3,0,4]];
_obj3 = createVehicle ["Flag", [6,0,8]];

_d12 = [_obj1, _obj2] call _distanceBetween;
_d13 = [_obj1, _obj3] call _distanceBetween;
hint format ["Dist^2 1-2: %1, 1-3: %2", _d12, _d13];

_sum = [1,2,3,4,5] call {
    private "_total";
    _total = 0;
    _i = 0;
    while {_i < count _this} do {
        _total = _total + (_this select _i);
        _i = _i + 1;
    };
    _total
};
hint format ["Sum of [1..5]: %1", _sum];

_max = [7, 3, 9, 1, 5] call {
    private "_m";
    _m = _this select 0;
    _i = 1;
    while {_i < count _this} do {
        if ((_this select _i) > _m) then {_m = _this select _i};
        _i = _i + 1;
    };
    _m
};
hint format ["Max of [7,3,9,1,5]: %1", _max];

_names = ["Alpha", "Bravo", "Charlie"];
_result = "";
_i = 0;
while {_i < count _names} do {
    if (_i > 0) then {_result = _result + ", "};
    _result = _result + (_names select _i);
    _i = _i + 1;
};
hint format ["Names: %1", _result]
