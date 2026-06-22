# Test Chapter

Here is a good example:

```sqf
_x = 2 + 3;
hint format ["result: %1", _x]
```

And a bad one:

```sqf
_y = undefined_func "test"
```

And one that needs game objects:

```sqf
player addEventHandler ["killed", {hint "dead"}]
```
