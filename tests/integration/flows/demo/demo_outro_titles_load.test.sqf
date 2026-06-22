// Fizzy #22: the real demo outro uses global RscTitles DEMO1..DEMO7.
// Exercise the complete sequence against RemasterDemo data so resource or
// asset shrinkage catches missing title classes/logo textures.

triSceneReady
{ titleRsc [format ["DEMO%1", _x], "plain", 1]; triSimFrames 45; } forEach [1,2,3,4,5,6,7]
triEndTest
