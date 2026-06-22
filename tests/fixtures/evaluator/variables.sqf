obj = createVehicle ["Car", [0,0,0]];
obj setVariable ["tag1", "hello"];
obj setVariable ["tag2", 99];
hint format ["tag1: %1", obj getVariable "tag1"];
hint format ["tag2: %1", obj getVariable "tag2"];
id1 = obj addAction ["Do Something", ""];
id2 = obj addAction ["Do More", ""];
hint format ["action ids: %1, %2", id1, id2]
