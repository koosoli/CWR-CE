static void CheckArcadeJoinMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeJoinSync);
}

static void CheckArcadeJoinSync(AIGroupContext* context)
{
    CheckArcadeSync(context, SArcadeJoinJoin);
}

static void ArcadeJoinJoin(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    Trace(group, "Join", "Join");

    int& index = context->_fsm->Var(0);

    AIGroup* join = FindTargetGroup(group, index);
    if (!join || join == group)
    {
        context->_fsm->SetState(SArcadeCountdown, context);
        return;
    }

    JoinGroups(group, join);
}

static void CheckArcadeJoinJoin(AIGroupContext* context)
{
    context->_fsm->SetState(SArcadeCountdown, context);
}

///////////////////////////////////////////////////////////////////////////////
// Join And Lead waypoint

static void CheckArcadeLeaderMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeLeaderSync);
}

static void CheckArcadeLeaderSync(AIGroupContext* context)
{
    CheckArcadeSync(context, SArcadeLeaderJoin);
}

static void ArcadeLeaderJoin(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    Trace(group, "Join and Lead", "Join");

    int& index = context->_fsm->Var(0);

    AIGroup* join = FindTargetGroup(group, index);
    if (!join || join == group)
    {
        context->_fsm->SetState(SArcadeCountdown, context);
        return;
    }

    JoinGroups(join, group);
}

static void CheckArcadeLeaderJoin(AIGroupContext* context)
{
    context->_fsm->SetState(SArcadeCountdown, context);
}

///////////////////////////////////////////////////////////////////////////////
// Get Out waypoint

static void CheckArcadeGetOutMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeGetOutGetOut);
}

static void ArcadeGetOutGetOut(AIGroupContext* context)
{
    STATE_PREFIX

    AllowGetIn(group, false);
}

static void CheckArcadeGetOutGetOut(AIGroupContext* context)
{
    STATE_PREFIX

    AIUnit* leader = group->Leader();

    if (!leader)
    {
        return;
    }

    if (leader->IsFreeSoldier())
    {
        context->_fsm->SetState(SArcadeSync, context);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Load waypoint

static void CheckArcadeLoadMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeLoadGetIn);
}

static void ArcadeLoadGetIn(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    for (int i = 0; i < MAX_UNITS_PER_GROUP; i++)
    {
        AIUnit* unit = group->UnitWithID(i + 1);
        if (unit)
        {
            unit->AllowGetIn(true);
            unit->OrderGetIn(true);
        }
    }
}

static void CheckArcadeLoadGetIn(AIGroupContext* context)
{
    context->_fsm->SetState(SArcadeSync, context);
}

///////////////////////////////////////////////////////////////////////////////
// Unload waypoint

static void CheckArcadeUnloadMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeUnloadGetOut);
}

static void ArcadeUnloadGetOut(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    for (int i = 0; i < MAX_UNITS_PER_GROUP; i++)
    {
        AIUnit* unit = group->UnitWithID(i + 1);
        if (!unit)
        {
            continue;
        }
        if (unit->IsInCargo())
        {
            unit->AllowGetIn(false);
        }
        else
        {
            Transport* veh = unit->VehicleAssigned();
            if (veh)
            {
                bool assignedAsCargo = false;
                for (int i = 0; i < veh->NCargoAssigned(); i++)
                {
                    if (veh->GetCargoAssigned(i) == unit)
                    {
                        assignedAsCargo = true;
                        break;
                    }
                }
                if (assignedAsCargo)
                {
                    unit->AllowGetIn(false);
                }
            }
        }
    }
}

static void CheckArcadeUnloadGetOut(AIGroupContext* context)
{
    context->_fsm->SetState(SArcadeSync, context);
}

///////////////////////////////////////////////////////////////////////////////
// Transport Unload waypoint

static void CheckArcadeTransportUnloadMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeTransportUnloadGetOut);
}

static void ArcadeTransportUnloadGetOut(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    for (int i = 0; i < group->NVehicles(); i++)
    {
        Transport* veh = group->GetVehicle(i);
        if (!veh)
        {
            continue;
        }
        const ManCargo& cargo = veh->GetManCargo();
        for (int j = cargo.Size() - 1; j >= 0; j--)
        {
            Person* man = cargo[j];
            if (!man)
            {
                continue;
            }
            AIUnit* unit = man->Brain();
            if (unit && unit->GetGroup() != group)
            {
                unit->AllowGetIn(false);
                unit->OrderGetIn(false);
                unit->UnassignVehicle();
                if (unit->IsGroupLeader() && unit->IsPlayer())
                {
                    veh->WaitForGetOut(unit);
                }
            }
        }
    }
}

static void CheckArcadeTransportUnloadGetOut(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    // check if none foreign units inside
    for (int i = 0; i < group->NVehicles(); i++)
    {
        const Transport* veh = group->GetVehicle(i);
        if (!veh)
        {
            continue;
        }
        const ManCargo& cargo = veh->GetManCargo();
        for (int j = cargo.Size() - 1; j >= 0; j--)
        {
            Person* man = cargo[j];
            if (!man)
            {
                continue;
            }
            AIUnit* unit = man->Brain();
            if (unit && unit->GetGroup() != group)
            {
                return;
            }
        }
    }

    context->_fsm->SetState(SArcadeSync, context);
}

///////////////////////////////////////////////////////////////////////////////
// Hold waypoint

const Target* FindHoldTarget(AIGroupContext* context)
{
    const Target* bestEnemy = nullptr;
    const Target* bestUnknown = nullptr;
    float dist2Enemy = FLT_MAX;
    float dist2Unknown = FLT_MAX;

    AIGroup* group = context->_group;
    AI_ERROR(group);
    AICenter* center = group->GetCenter();
    AI_ERROR(center);
    AIUnit* leader = group->Leader();
    Vector3Val posL = leader->Position();

    const TargetList& list = group->GetTargetList();
    for (int i = 0; i < list.Size(); i++)
    {
        const Target& info = *list[i];
        if (!info.IsKnownBy(leader))
        {
            continue;
        }
        if (info.type->IsKindOf(GLOB_WORLD->Preloaded(VTypeStatic)))
        {
            continue;
        }
        if (info.side == TSideUnknown)
        {
            float dist2 = info.position.Distance2(posL);
            if (dist2 < dist2Unknown)
            {
                dist2Unknown = dist2;
                bestUnknown = &info;
            }
        }
        else if (center->IsEnemy(info.side))
        {
            float dist2 = info.position.Distance2(posL);
            if (dist2 < dist2Enemy)
            {
                dist2Enemy = dist2;
                bestEnemy = &info;
            }
        }
    }

    if (bestEnemy)
    {
        return bestEnemy;
    }
    else
    {
        return bestUnknown;
    }
}

enum CheckOverlookResult
{
    CORDone,
    CORIdentified,
    CORContinue,
};

static CheckOverlookResult CheckOverlook(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        return CORDone;
    }
    if (group->GetAllDone())
    {
        return CORDone;
    }

    // check if target is identify
    TargetType* target = group->GetOverlookTarget();
    if (!target)
    {
        return CORContinue;
    }
    Target* info = group->FindTarget(target);
    if (!info)
    {
        return CORContinue;
    }

    if (info->side != TSideUnknown)
    {
        return CORIdentified;
    }

    return CORContinue;
}

/*
static void CheckOverlook(AIGroupContext *context, ArcadeFSMStates next)
{
    AIGroup *group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(next, context);
        return;
    }

    if (group->GetAllDone())
    {
        context->_fsm->SetState(next, context);
        return;
    }

    // check if target is identify
    TargetId target = group->GetOverlookTarget();
    if (!target) return;
    Target *info = group->FindTarget(target);
    if (!info) return;
    if (info->side != TSideUnknown)
    {
        context->_fsm->SetState(next, context);
        return;
    }
}
*/

static void CheckArcadeHoldMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeHoldWait);
}

static void CheckArcadeHoldWait(AIGroupContext* context)
{
    STATE_PREFIX
    if (group->IsPlayerGroup())
    {
        return;
    }
    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    const Target* info = FindHoldTarget(context);
    if (IsTargetValid(info, leader) && info->side == TSideUnknown)
    {
        // do not overlook too far
        Vector3Val posI = info->position;
        Vector3Val posD = mission->_destination;
        float dist = (posI - posD).SizeXZ();

        float threshold = group->UpdateAndGetThreshold();

        const float distMin = 400.0f;
        const float distMax = 800.0f;
        const float invDist = 1.0f / (distMax - distMin);
        float value = (dist - distMin) * invDist;
        if (value >= threshold)
        {
            return;
        }

        // avoid duplicity of FindHoldTarget
        context->_fsm->SetState(SArcadeHoldOverlook, context);
        group->SetOverlookTarget(info->idExact);

        Command cmd;
        cmd._message = Command::Move;
        cmd._destination = posI;
        cmd._discretion = Command::Undefined;
        cmd._context = Command::CtxMission;
        group->SendCommand(cmd);
        return;
    }
    else
    {
        // no target or enemy target
        Vector3Val posL = leader->Position();
        Vector3Val posD = mission->_destination;
        float prec = MOVE_BACK_COEF * leader->GetVehicle()->GetPrecision();
        saturateMax(prec, 5);
        if ((posD - posL).SquareSizeXZ() > Square(prec))
        {
            // move back
            context->_fsm->SetState(SArcadeHoldMove, context);
            return;
        }
    }
}

static void ArcadeHoldOverlook(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeHoldOverlook(AIGroupContext* context)
{
    STATE_PREFIX

    switch (CheckOverlook(context))
    {
        case CORDone:
        {
            TargetType* id = group->GetOverlookTarget();
            if (id)
            {
                const Target* info = group->FindTarget(id);
                if (info && info->side == TSideUnknown)
                {
                    // do not overlook too far
                    Vector3Val posI = info->position;
                    Vector3Val posD = mission->_destination;
                    float dist = (posI - posD).SizeXZ();

                    float threshold = group->UpdateAndGetThreshold();

                    const float distMin = 600.0f;
                    const float distMax = 1000.0f;
                    const float invDist = 1.0f / (distMax - distMin);

                    float value = (dist - distMin) * invDist;
                    if (value < threshold)
                    {
                        // avoid duplicity of FindHoldTarget
                        Command cmd;
                        cmd._message = Command::Move;
                        cmd._destination = posI;
                        cmd._discretion = Command::Undefined;
                        cmd._context = Command::CtxMission;
                        group->SendCommand(cmd);
                        return;
                    }
                }
            }
            // do not continue with overlook
            context->_fsm->SetState(SArcadeHoldWait, context);
        }
            return;
        case CORIdentified:
            context->_fsm->SetState(SArcadeHoldWait, context);
            return;
        case CORContinue:
            return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Sentry waypoint

static void CheckArcadeSentryMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeSentryWait);
}

static void CheckArcadeSentryWait(AIGroupContext* context)
{
    STATE_PREFIX
    if (group->IsPlayerGroup())
    {
        return;
    }
    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    const Target* info = FindHoldTarget(context);
    if (!IsTargetValid(info, leader))
    {
        Vector3Val posL = leader->Position();
        Vector3Val posD = mission->_destination;
        float prec = MOVE_BACK_COEF * leader->GetVehicle()->GetPrecision();
        saturateMax(prec, MOVE_BACK_MINIMUM);
        if ((posD - posL).SquareSizeXZ() > Square(prec))
        {
            // move back
            context->_fsm->SetState(SArcadeSentryMove, context);
        }
        return;
    }
    else if (info->side == TSideUnknown)
    {
        // do not overlook too far
        Vector3Val posI = info->position;
        Vector3Val posD = mission->_destination;
        float dist = (posI - posD).SizeXZ();

        float threshold = group->UpdateAndGetThreshold();

        const float distMin = 400.0f;
        const float distMax = 1600.0f;
        const float invDist = 1.0f / (distMax - distMin);
        float value = (dist - distMin) * invDist;
        if (value >= threshold)
        {
            return;
        }

        // avoid duplicity of FindHoldTarget
        context->_fsm->SetState(SArcadeSentryOverlook, context);
        group->SetOverlookTarget(info->idExact);

        Command cmd;
        cmd._message = Command::Move;
        cmd._destination = posI;
        cmd._discretion = Command::Undefined;
        cmd._context = Command::CtxMission;
        group->SendCommand(cmd);
        return;
    }
    else
    {
        // enemy target
        context->_fsm->SetState(SArcadeSync, context);
        return;
    }

    /*
        if (CheckHoldTarget(context, SArcadeSentryOverlook, SArcadeSentryMove))
        {
            // enemy detected
            // continue
            context->_fsm->SetState(SArcadeSync, context);
            return;
        }
    */
}

static void ArcadeSentryOverlook(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeSentryOverlook(AIGroupContext* context)
{
    STATE_PREFIX

    switch (CheckOverlook(context))
    {
        case CORDone:
        {
            TargetType* id = group->GetOverlookTarget();
            if (id)
            {
                const Target* info = group->FindTarget(id);
                if (!info)
                {
                    // target disappeared - try to find it with Brown movement
                    int& counter = context->_fsm->Var(5);
                    counter = 1;

                    Vector3 posL = group->Leader()->Position();
                    group->SetGuardPosition(posL);

                    const float maxTime = 4.0f;
                    const float maxSpeed = group->Leader()->GetVehicle()->GetType()->GetMaxSpeedMs();
                    float maxDist = maxTime * maxSpeed;
                    Vector3 posD = posL + Vector3(GRandGen.PlusMinus(0, maxDist), 0, GRandGen.PlusMinus(0, maxDist));
                    posD[1] = GLandscape->RoadSurfaceY(posD[0], posD[2]);

                    Command cmd;
                    cmd._message = Command::Move;
                    cmd._destination = posD;
                    cmd._discretion = Command::Undefined;
                    cmd._context = Command::CtxMission;
                    group->SendCommand(cmd);

                    context->_fsm->SetState(SArcadeSentryBrown, context);
                    return;
                }
                else if (info->side == TSideUnknown)
                {
                    // do not overlook too far
                    Vector3Val posI = info->position;
                    Vector3Val posD = mission->_destination;
                    float dist = (posI - posD).SizeXZ();

                    float threshold = group->UpdateAndGetThreshold();

                    const float distMin = 600.0f;
                    const float distMax = 2000.0f;
                    const float invDist = 1.0f / (distMax - distMin);

                    float value = (dist - distMin) * invDist;
                    if (value < threshold)
                    {
                        // avoid duplicity of FindHoldTarget
                        Command cmd;
                        cmd._message = Command::Move;
                        cmd._destination = posI;
                        cmd._discretion = Command::Undefined;
                        cmd._context = Command::CtxMission;
                        group->SendCommand(cmd);
                        return;
                    }
                }
            }
            // do not continue with overlook
            context->_fsm->SetState(SArcadeSentryWait, context);
        }
            return;
        case CORIdentified:
            context->_fsm->SetState(SArcadeSentryWait, context);
            return;
        case CORContinue:
            return;
    }
    /*
        CheckOverlook(context, SArcadeSentryWait);
    */
}

static void ArcadeSentryBrown(AIGroupContext* context) {}

static void CheckArcadeSentryBrown(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeSentryWait, context);
        return;
    }

    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    const Target* info = FindHoldTarget(context);
    if (IsTargetValid(info, leader))
    {
        context->_fsm->SetState(SArcadeSentryWait, context);
        return;
    }

    if (group->GetAllDone())
    {
        int& counter = context->_fsm->Var(5);
        counter++;

        if (counter >= 5)
        {
            // cannot find it - move back
            context->_fsm->SetState(SArcadeSentryMove, context);
        }
        else
        {
            const float maxTime = 4.0f;
            const float maxSpeed = group->Leader()->GetVehicle()->GetType()->GetMaxSpeedMs();
            float maxDist = maxTime * maxSpeed;
            Vector3 posD =
                group->GetGuardPosition() + Vector3(GRandGen.PlusMinus(0, maxDist), 0, GRandGen.PlusMinus(0, maxDist));
            posD[1] = GLandscape->RoadSurfaceY(posD[0], posD[2]);

            Command cmd;
            cmd._message = Command::Move;
            cmd._destination = posD;
            cmd._discretion = Command::Undefined;
            cmd._context = Command::CtxMission;
            group->SendCommand(cmd);
        }
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Seek And Destroy waypoint

static void CheckArcadeSeekAndDestroyMove(AIGroupContext* context)
{
    CheckArcadeMove(context, SArcadeSeekAndDestroyCheck);
}

static void ArcadeSeekAndDestroyCheck(AIGroupContext* context)
{
    context->_group->SetCheckTime(Glob.time + 15.0);
    int& counter = context->_fsm->Var(5);
    counter = 0;
}

static void CheckArcadeSeekAndDestroyCheck(AIGroupContext* context)
{
    context->_fsm->SetState(SArcadeSeekAndDestroyWait, context);
}

static void CheckArcadeSeekAndDestroyWait(AIGroupContext* context)
{
    STATE_PREFIX

    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    if (group->IsPlayerGroup())
    {
        // another processing for player group
        const Target* info = FindHoldTarget(context);
        if (IsTargetValid(info, leader) && info->side != TSideUnknown)
        {
            // enemy detected
            context->_fsm->SetState(SArcadeSeekAndDestroyCheck, context);
        }
        else if (Glob.time >= group->GetCheckTime())
        {
            // check time expired
            context->_fsm->SetState(SArcadeSync, context);
        }
        return;
    }

    const Target* info = FindHoldTarget(context);
    if (!IsTargetValid(info, leader))
    {
        int& counter = context->_fsm->Var(5);
        if (counter >= 5)
        {
            context->_fsm->SetState(SArcadeSync, context);
        }
        else
        {
            context->_fsm->SetState(SArcadeSeekAndDestroyBrown, context);
        }
        return;
    }
    else if (info->side == TSideUnknown)
    {
        // do not overlook too far
        Vector3Val posI = info->position;
        Vector3Val posD = mission->_destination;
        float dist = (posI - posD).SizeXZ();

        float threshold = group->UpdateAndGetThreshold();

        const float distMin = 400.0f;
        const float distMax = 1600.0f;
        const float invDist = 1.0f / (distMax - distMin);
        float value = (dist - distMin) * invDist;
        if (value >= threshold)
        {
            return;
        }

        // avoid duplicity of FindHoldTarget
        context->_fsm->SetState(SArcadeSeekAndDestroyOverlook, context);
        group->SetOverlookTarget(info->idExact);

        Command cmd;
        cmd._message = Command::Move;
        cmd._destination = posI;
        cmd._discretion = Command::Undefined;
        cmd._context = Command::CtxMission;
        group->SendCommand(cmd);
        return;
    }
    else
    {
        // enemy target
        context->_fsm->SetState(SArcadeSeekAndDestroyCheck, context);
        return;
    }

    /*
        if (CheckHoldTarget(context, SArcadeSeekAndDestroyOverlook, SArcadeSeekAndDestroyMove))
        {
            // enemy detected
            context->_fsm->SetState(SArcadeSeekAndDestroyCheck, context);
            return;
        }

        AIGroup *group = context->_group;
        AI_ERROR(group);

        if
        (
            context->_fsm->GetState() == SArcadeSeekAndDestroyWait &&
            Glob.time >= group->GetCheckTime())
        {
            // state did not changed && check time expired
            context->_fsm->SetState(SArcadeSync, context);
            return;
        }
    */
}

static void ArcadeSeekAndDestroyOverlook(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeSeekAndDestroyOverlook(AIGroupContext* context)
{
    STATE_PREFIX

    switch (CheckOverlook(context))
    {
        case CORDone:
        {
            TargetType* id = group->GetOverlookTarget();
            if (id)
            {
                const Target* info = group->FindTarget(id);
                if (info && info->side == TSideUnknown)
                {
                    // do not overlook too far
                    Vector3Val posI = info->position;
                    Vector3Val posD = mission->_destination;
                    float dist = (posI - posD).SizeXZ();

                    float threshold = group->UpdateAndGetThreshold();

                    const float distMin = 600.0f;
                    const float distMax = 2000.0f;
                    const float invDist = 1.0f / (distMax - distMin);

                    float value = (dist - distMin) * invDist;
                    if (value < threshold)
                    {
                        // avoid duplicity of FindHoldTarget
                        Command cmd;
                        cmd._message = Command::Move;
                        cmd._destination = posI;
                        cmd._discretion = Command::Undefined;
                        cmd._context = Command::CtxMission;
                        group->SendCommand(cmd);
                        return;
                    }
                }
            }
            // do not continue with overlook - move back
            context->_fsm->SetState(SArcadeSeekAndDestroyMove, context);
        }
            return;
        case CORIdentified:
            context->_fsm->SetState(SArcadeSeekAndDestroyCheck, context);
            return;
        case CORContinue:
            return;
    }
    /*
        CheckOverlook(context, SArcadeSeekAndDestroyCheck);
    */
}

static void ArcadeSeekAndDestroyBrown(AIGroupContext* context)
{
    STATE_PREFIX

    const float maxTime = 6.0f;
    const float maxSpeed = group->Leader()->GetVehicle()->GetType()->GetMaxSpeedMs();
    float maxDist = maxTime * maxSpeed;
    Vector3 posD = mission->_destination + Vector3(GRandGen.PlusMinus(0, maxDist), 0, GRandGen.PlusMinus(0, maxDist));
    posD[1] = GLandscape->RoadSurfaceY(posD[0], posD[2]);

    Command cmd;
    cmd._message = Command::Move;
    cmd._destination = posD;
    cmd._discretion = Command::Undefined;
    cmd._context = Command::CtxMission;
    group->SendCommand(cmd);
}

static void CheckArcadeSeekAndDestroyBrown(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeSeekAndDestroyWait, context);
        return;
    }

    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    const Target* info = FindHoldTarget(context);
    if (IsTargetValid(info, leader))
    {
        context->_fsm->SetState(SArcadeSeekAndDestroyWait, context);
        return;
    }

    if (group->GetAllDone())
    {
        context->_fsm->SetState(SArcadeSeekAndDestroyWait, context);
        int& counter = context->_fsm->Var(5);
        counter++;
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Guard waypoint

bool IsGuardSynchronized(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    int& index = context->_fsm->Var(0);
    ArcadeWaypointInfo& wInfo = group->GetWaypoint(index);

    for (int j = 0; j < wInfo.synchronizations.Size(); j++)
    {
        int sync = wInfo.synchronizations[j];
        AI_ERROR(sync >= 0);
        if (synchronized[sync].sensors.Size() > 0)
        {
            return true;
        }
    }
    return false;
}

const AITargetInfo* FindGuardTarget(AIGroupContext* context)
{
    const AITargetInfo* bestEnemy = nullptr;
    const AITargetInfo* bestUnknown = nullptr;
    float dist2Enemy = FLT_MAX;
    float dist2Unknown = FLT_MAX;

    AIGroup* group = context->_group;
    AI_ERROR(group);
    AICenter* center = group->GetCenter();
    AI_ERROR(center);
    int& index = context->_fsm->Var(0);
    ArcadeWaypointInfo& wInfo = group->GetWaypoint(index);
    Vector3Val posL = group->Leader()->Position();

    bool found = false;
    for (int j = 0; j < wInfo.synchronizations.Size(); j++)
    {
        int sync = wInfo.synchronizations[j];
        AI_ERROR(sync >= 0);
        int n = synchronized[sync].sensors.Size();
        if (n > 0)
        {
            found = true;
            for (int i = 0; i < n; i++)
            {
                SynchronizedSensor& sensor = synchronized[sync].sensors[i];
                Vehicle* detVeh = sensor.sensor;
                if (!detVeh)
                {
                    continue;
                }
                Detector* det = dyn_cast<Detector>(detVeh);
                if (!det)
                {
                    continue;
                }
                int o = det->NVehicles();
                for (int k = 0; k < o; k++)
                {
                    TargetType* obj = det->GetVehicle(k);
                    if (!obj)
                    {
                        continue;
                    }
                    const AITargetInfo* info = center->FindTargetInfo(obj);
                    if (!info)
                    {
                        continue;
                    }
                    if (info->_side == TSideUnknown)
                    {
                        float dist2 = info->_realPos.Distance2(posL);
                        if (dist2 < dist2Unknown)
                        {
                            dist2Unknown = dist2;
                            bestUnknown = info;
                        }
                    }
                    else if (center->IsEnemy(info->_side))
                    {
                        float dist2 = info->_realPos.Distance2(posL);
                        if (dist2 < dist2Enemy)
                        {
                            dist2Enemy = dist2;
                            bestEnemy = info;
                        }
                    }
                }
            }
        }
    }
    AI_ERROR(found);

    if (bestEnemy)
    {
        return bestEnemy;
    }
    else
    {
        return bestUnknown;
    }
}

Vector3Val GetSensorPosition(AIGroupContext* context)
{
    const Object* bestDetector = nullptr;
    float dist2Min = FLT_MAX;

    AIGroup* group = context->_group;
    AI_ERROR(group);
    AI_ERROR(group->GetCenter());
    int& index = context->_fsm->Var(0);
    ArcadeWaypointInfo& wInfo = group->GetWaypoint(index);
    Vector3Val posL = group->Leader()->Position();

    for (int j = 0; j < wInfo.synchronizations.Size(); j++)
    {
        int sync = wInfo.synchronizations[j];
        AI_ERROR(sync >= 0);
        int n = synchronized[sync].sensors.Size();
        if (n > 0)
        {
            for (int i = 0; i < n; i++)
            {
                SynchronizedSensor& sensor = synchronized[sync].sensors[i];
                Vehicle* detVeh = sensor.sensor;
                if (!detVeh)
                {
                    continue;
                }
                float dist2 = (detVeh->Position() - posL).SquareSizeXZ();
                if (dist2 < dist2Min)
                {
                    dist2Min = dist2;
                    bestDetector = detVeh;
                }
            }
        }
    }

    AI_ERROR(bestDetector);
    if (!bestDetector)
    {
        return posL;
    }

    return bestDetector->Position();
}

static void CheckArcadeGuardMove(AIGroupContext* context)
{
    ArcadeFSMStates state = IsGuardSynchronized(context) ? SArcadeGuardWait : SArcadeGravonWait;

    if (CheckArcadeMove(context, state))
    {
        Time limit = Glob.time + GRandGen.PlusMinus(450.0f, 150.0f);
        Time& time = context->_fsm->VarTime(0);
        time = limit;
    }
}

static void CheckArcadeGuardWait(AIGroupContext* context)
{
    STATE_PREFIX
    if (group->IsPlayerGroup())
    {
        return;
    }
    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    const AITargetInfo* info = FindGuardTarget(context);
    if (info)
    {
        const Target* vInfo = group->FindTarget(info->_idExact);
        if (!vInfo || vInfo->side == TSideUnknown)
        {
            Trace(group, "GUARD", "OVERLOOK");

            context->_fsm->SetState(SArcadeGuardOverlook, context);
            group->SetOverlookTarget(info->_idExact);
            // avoid duplicity of FindGuardTarget
            Command cmd;
            cmd._message = Command::Move;
            cmd._destination = info->_realPos;
            cmd._discretion = Command::Undefined;
            cmd._context = Command::CtxMission;
            group->SendCommand(cmd);
            AllowGetIn(group);
            return;
        }
        else
        {
            Trace(group, "GUARD", "ATTACK");

            // enemy
            context->_fsm->SetState(SArcadeGuardAttack, context);
            // avoid duplicity of FindGuardTarget
            Command cmd;
            cmd._message = Command::AttackAndFire;
            cmd._destination = leader->Position(); // target is known
            cmd._target = info->_idExact;
            cmd._discretion = Command::Undefined;
            cmd._context = Command::CtxMission;
            group->SendCommand(cmd);
            AllowGetIn(group);
            return;
        }
    }

    Vector3Val posL = leader->Position();
    Vector3Val posD = mission->_destination;
    float prec = MOVE_BACK_COEF * leader->GetVehicle()->GetPrecision();
    saturateMax(prec, MOVE_BACK_MINIMUM);
    float dist2 = (posD - posL).SquareSizeXZ();
    if (dist2 > Square(prec))
    {
        Trace(group, "GUARD", "MOVE BACK");

        // move back
        context->_fsm->SetState(SArcadeGuardMove, context);
        prec = 50.0f;
        if (leader->VehicleAssigned())
        {
            prec = MOVE_BACK_COEF * leader->VehicleAssigned()->GetPrecision();
        }
        saturateMax(prec, MOVE_BACK_MINIMUM);
        if (dist2 > Square(prec))
        {
            AllowGetIn(group);
        }
        return;
    }

    Time& time = context->_fsm->VarTime(0);
    if (Glob.time >= time)
    {
        // look at sensor
        int& counter = context->_fsm->Var(5);
        counter = 1;

        Vector3 posS = GetSensorPosition(context);
        group->SetGuardPosition(posS);

        const float maxTime = 4.0f;
        const float maxSpeed = group->Leader()->GetVehicle()->GetType()->GetMaxSpeedMs();
        float maxDist = maxTime * maxSpeed;
        Vector3 posD = posS + Vector3(GRandGen.PlusMinus(0, maxDist), 0, GRandGen.PlusMinus(0, maxDist));
        posD[1] = GLandscape->RoadSurfaceY(posD[0], posD[2]);

        Command cmd;
        cmd._message = Command::Move;
        cmd._destination = posD;
        cmd._discretion = Command::Undefined;
        cmd._context = Command::CtxMission;
        group->SendCommand(cmd);
        context->_fsm->SetState(SArcadeGuardBrown, context);
        AllowGetIn(group);
        return;
    }

    // idle state - get out from vehicles
    AllowGetIn(group, false);
}

static void ArcadeGuardAttack(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeGuardAttack(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeGuardWait, context);
        return;
    }

    if (group->GetAllDone())
    {
        context->_fsm->SetState(SArcadeGuardWait, context);
        return;
    }
}

static void ArcadeGuardOverlook(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeGuardOverlook(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeGuardWait, context);
        return;
    }

    if (group->GetAllDone())
    {
        // target was not found - try to find it with brown movement
        int& counter = context->_fsm->Var(5);
        counter = 1;

        Vector3 posL = group->Leader()->Position();
        group->SetGuardPosition(posL);

        const float maxTime = 4.0f;
        const float maxSpeed = group->Leader()->GetVehicle()->GetType()->GetMaxSpeedMs();
        float maxDist = maxTime * maxSpeed;
        Vector3 posD = posL + Vector3(GRandGen.PlusMinus(0, maxDist), 0, GRandGen.PlusMinus(0, maxDist));
        posD[1] = GLandscape->RoadSurfaceY(posD[0], posD[2]);

        Command cmd;
        cmd._message = Command::Move;
        cmd._destination = posD;
        cmd._discretion = Command::Undefined;
        cmd._context = Command::CtxMission;
        group->SendCommand(cmd);

        context->_fsm->SetState(SArcadeGuardBrownTarget, context);
        return;
    }

    // check if target is identify
    TargetType* target = group->GetOverlookTarget();
    if (!target)
    {
        return;
    }
    const Target* vInfo = group->FindTarget(target);
    if (vInfo && vInfo->side != TSideUnknown)
    {
        context->_fsm->SetState(SArcadeGuardWait, context);
        return;
    }

    /*
        AICenter *center = group->GetCenter();
        AI_ERROR(center);
        const AITargetInfo *info = center->FindTargetInfo(target);
        if (!info) return;
        if (info->_side != TSideUnknown)
        {
            context->_fsm->SetState(SArcadeGuardWait, context);
            return;
        }
    */
}

static void ArcadeGuardBrown(AIGroupContext* context)
{
    // do nothing
}

static void CheckArcadeGuardBrown(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeGuardWait, context);
        return;
    }

    const AITargetInfo* info = FindGuardTarget(context);
    if (info)
    {
        // something found
        context->_fsm->SetState(SArcadeGuardWait, context);
    }

    if (group->GetAllDone())
    {
        int& counter = context->_fsm->Var(5);
        counter++;

        if (counter >= 5)
        {
            // cannot find anything - move back
            context->_fsm->SetState(SArcadeGuardMove, context);
        }
        else
        {
            const float maxTime = 4.0f;
            const float maxSpeed = group->Leader()->GetVehicle()->GetType()->GetMaxSpeedMs();
            float maxDist = maxTime * maxSpeed;
            Vector3 posD =
                group->GetGuardPosition() + Vector3(GRandGen.PlusMinus(0, maxDist), 0, GRandGen.PlusMinus(0, maxDist));
            posD[1] = GLandscape->RoadSurfaceY(posD[0], posD[2]);

            Command cmd;
            cmd._message = Command::Move;
            cmd._destination = posD;
            cmd._discretion = Command::Undefined;
            cmd._context = Command::CtxMission;
            group->SendCommand(cmd);
        }
        return;
    }
}

static void ArcadeGuardBrownTarget(AIGroupContext* context)
{
    // do nothing
}

static void CheckArcadeGuardBrownTarget(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeGuardWait, context);
        return;
    }

    // check if target is identify
    TargetType* target = group->GetOverlookTarget();
    if (target)
    {
        const Target* vInfo = group->FindTarget(target);
        if (vInfo && vInfo->side != TSideUnknown)
        {
            context->_fsm->SetState(SArcadeGuardWait, context);
            return;
        }
    }

    if (group->GetAllDone())
    {
        int& counter = context->_fsm->Var(5);
        counter++;

        if (counter >= 5)
        {
            // cannot find anything - move back
            context->_fsm->SetState(SArcadeGuardMove, context);
        }
        else
        {
            const float maxTime = 4.0f;
            const float maxSpeed = group->Leader()->GetVehicle()->GetType()->GetMaxSpeedMs();
            float maxDist = maxTime * maxSpeed;
            Vector3 posD =
                group->GetGuardPosition() + Vector3(GRandGen.PlusMinus(0, maxDist), 0, GRandGen.PlusMinus(0, maxDist));
            posD[1] = GLandscape->RoadSurfaceY(posD[0], posD[2]);

            Command cmd;
            cmd._message = Command::Move;
            cmd._destination = posD;
            cmd._discretion = Command::Undefined;
            cmd._context = Command::CtxMission;
            group->SendCommand(cmd);
        }
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Guard waypoint - Gravon subdiagram

static void CheckArcadeGravonWait(AIGroupContext* context)
{
    STATE_PREFIX
    if (group->IsPlayerGroup())
    {
        return;
    }

    AICenter* center = group->GetCenter();
    AI_ERROR(center);
    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    AIGuardTarget tgt = center->GetGuardTarget(group);
    switch (tgt.type)
    {
        case GTTNothing:
        {
            Vector3Val posL = leader->Position();
            Vector3Val posD = mission->_destination;
            float prec = MOVE_BACK_COEF * leader->GetVehicle()->GetPrecision();
            saturateMax(prec, MOVE_BACK_MINIMUM);
            if ((posD - posL).SquareSizeXZ() > Square(prec))
            {
                // move back
                context->_fsm->SetState(SArcadeGuardMove, context);
            }
        }
        break;
        case GTTVehicle:
        {
            const AITargetInfo* info = center->FindTargetInfo(tgt.vehicle);
            AI_ERROR(info);
            if (!info)
            {
                return;
            }
            Target* target = group->FindTarget(tgt.vehicle);
            if (target)
            {
                if (target->destroyed)
                {
                    return;
                }
            }
            else
            {
                Vector3Val posL = leader->Position();
                float prec = MOVE_BACK_COEF * leader->GetVehicle()->GetPrecision();
                saturateMax(prec, 10.0);
                if (info->_realPos.Distance2(posL) < Square(prec))
                {
                    return;
                }
            }
            group->SetOverlookTarget(tgt.vehicle);
            if (center->IsEnemy(info->_side))
            {
                context->_fsm->SetState(SArcadeGravonAttack, context);

                Command cmd;
                cmd._message = Command::AttackAndFire;
                cmd._destination = info->_realPos; // leader->Position();
                cmd._target = tgt.vehicle;
                cmd._discretion = Command::Undefined;
                cmd._context = Command::CtxMission;
                group->SendCommand(cmd);
            }
            else if (info->_side == TSideUnknown)
            {
                context->_fsm->SetState(SArcadeGravonOverlook, context);

                Command cmd;
                cmd._message = Command::Move;
                cmd._destination = info->_realPos;
                cmd._discretion = Command::Undefined;
                cmd._context = Command::CtxMission;
                group->SendCommand(cmd);
            }
            else // friendly - ignore
            {
                LOG_DEBUG(AI, "Avoid attacking friendly.");
            }
        }
        break;
        case GTTPoint:
        {
            Vector3Val posL = leader->Position();
            Vector3Val posD = tgt.position;
            group->SetGuardPosition(posD);
            float prec = MOVE_BACK_COEF * leader->GetVehicle()->GetPrecision();
            saturateMax(prec, MOVE_BACK_MINIMUM);
            if ((posD - posL).SquareSizeXZ() > Square(prec))
            {
                context->_fsm->SetState(SArcadeGravonMove, context);

                Command cmd;
                cmd._message = Command::Move;
                cmd._destination = posD;
                cmd._discretion = Command::Undefined;
                cmd._context = Command::CtxMission;
                group->SendCommand(cmd);
            }
        }
        break;
        default:
            Fail("Guard target type");
            break;
    }
}

static void ArcadeGravonAttack(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeGravonAttack(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    AICenter* center = group->GetCenter();
    AI_ERROR(center);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }

    AIGuardTarget tgt = center->GetGuardTarget(group);
    if (tgt.type != GTTVehicle || tgt.vehicle != group->GetOverlookTarget())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }

    // if all attacks are completed, we can pause
    if (group->GetAllDone())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }
}

static void ArcadeGravonOverlook(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeGravonOverlook(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    AICenter* center = group->GetCenter();
    AI_ERROR(center);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }

    AIGuardTarget tgt = center->GetGuardTarget(group);
    if (tgt.type != GTTVehicle || tgt.vehicle != group->GetOverlookTarget())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }

    if (group->GetAllDone())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }

    // check if target is identify
    TargetType* target = group->GetOverlookTarget();
    if (!target)
    {
        return;
    }
    const AITargetInfo* info = center->FindTargetInfo(target);
    if (!info)
    {
        return;
    }
    if (info->_side != TSideUnknown)
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }
}

static void ArcadeGravonMove(AIGroupContext* context)
{
    // nothing to do
}

static void CheckArcadeGravonMove(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    AICenter* center = group->GetCenter();
    AI_ERROR(center);

    if (group->IsPlayerGroup())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }

    // check if tgt doesn't changed
    // if there is some target, go immediatelly to wait state
    AIGuardTarget tgt = center->GetGuardTarget(group);
    if (tgt.type != GTTPoint || (tgt.position - group->GetGuardPosition()).SquareSizeXZ() > 1.0)
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }

    if (group->GetAllDone())
    {
        context->_fsm->SetState(SArcadeGravonWait, context);
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Support

static void CheckArcadeSupportMove(AIGroupContext* context)
{
    if (CheckArcadeMove(context, SArcadeSupportWait))
    {
        AIGroup* group = context->_group;
        AI_ERROR(group);
        AICenter* center = group->GetCenter();
        AI_ERROR(center);

        center->ReadyForSupport(group);
        AllowGetIn(group, false);
    }
}

static void CheckArcadeSupportWait(AIGroupContext* context)
{
    STATE_PREFIX

    if (group->GetSupportedGroup())
    {
        mission->_destination = group->GetSupportPos();
        context->_fsm->SetState(SArcadeSupportTransport, context);
        AllowGetIn(group, true);
    }
}

static void SupportConfirmed(AIGroup* group, Vector3Val pos)
{
    AICenter* center = group->GetCenter();
    center->GetRadio().Transmit(new RadioMessageSupportConfirm(group), center->GetLanguage());

    if (group->IsPlayerGroup())
    {
        return;
    }

    group->Move(group->MainSubgroup(), // who
                pos,                   // where
                Command::Undefined     // how
    );
}

static void ArcadeSupportTransport(AIGroupContext* context)
{
    STATE_PREFIX
    SupportConfirmed(group, mission->_destination);
}

static void CheckArcadeSupportTransport(AIGroupContext* context)
{
    STATE_PREFIX

    if (!group->GetSupportedGroup())
    {
        context->_fsm->SetState(SArcadeSupportWait, context);
        AllowGetIn(group, false);
        return;
    }
    else if (mission->_destination.Distance2(group->GetSupportPos()) > Square(20))
    {
        // support position changed
        mission->_destination = group->GetSupportPos();
        SupportConfirmed(group, mission->_destination);
        return;
    }
    CheckArcadeMove(context, SArcadeSupportSupply);
}

static void ArcadeSupportSupply(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);

    AllowGetIn(group, false);

    AICenter* center = group->GetCenter();
    center->GetRadio().Transmit(new RadioMessageSupportReady(group), center->GetLanguage());
}

static void CheckArcadeSupportSupply(AIGroupContext* context)
{
    STATE_PREFIX

    if (!group->GetSupportedGroup())
    {
        context->_fsm->SetState(SArcadeSupportWait, context);
        return;
    }
    else if (mission->_destination.Distance2(group->GetSupportPos()) > Square(20))
    {
        // support position changed
        AllowGetIn(group);
        mission->_destination = group->GetSupportPos();
        context->_fsm->SetState(SArcadeSupportTransport, context);
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Logic subdiagram

static void ArcadeLogic(AIGroupContext* context)
{
    STATE_PREFIX
    AI_ERROR(group->GetCenter()->GetSide() == TLogic)

        int& index = context->_fsm->Var(0);
    ArcadeWaypointInfo& wInfo = group->GetWaypoint(index);
    group->Leader()->GetVehicle()->Move(wInfo.position);

    for (int j = 0; j < wInfo.synchronizations.Size(); j++)
    {
        int sync = wInfo.synchronizations[j];
        AI_ERROR(sync >= 0);
        synchronized[sync].SetActive(group, false);
        GetNetworkManager().GroupSynchronization(group, sync, false);
    }

    int& waiting = context->_fsm->Var(4);
    waiting = TRUE;
}

static void CheckArcadeLogic(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    int& index = context->_fsm->Var(0);
    ArcadeWaypointInfo& wInfo = group->GetWaypoint(index);

    int& type = context->_fsm->Var(1);
    if (!IsSyncActive(group, wInfo, type))
    {
        int& waiting = context->_fsm->Var(4);
        waiting = FALSE;

        context->_fsm->SetState(SArcadeCountdown, context);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Flee subdiagram

Vector3 FindFleePoint(AIGroup* group, bool& forced)
{
    forced = false;

    AI_ERROR(!group->IsPlayerGroup());
    AI_ERROR(group->NWaypoints() > 0); // waypoint 0 is initial position
    AI_ERROR(group->Leader());
    AICenter* center = group->GetCenter();
    AI_ERROR(center);

    Vector3Val posL = group->Leader()->Position();
    float minCost = FLT_MAX;
    const AITargetInfo* bestInfo = nullptr;
    const float costPerDist = 0.1f;

    for (int i = 0; i < center->NTargets(); i++)
    {
        const AITargetInfo& info = center->GetTarget(i);
        if (!info._idExact)
        {
            continue;
        }
        if (!(info._side == TCivilian) && !(center->IsFriendly(info._side)))
        {
            continue;
        }

        VehicleSupply* veh = dyn_cast<Transport, Object>(info._idExact);
        if (!veh)
        {
            continue;
        }

        // check if it is my group vehicle
        AIGroup* infoGroup = info._idExact->GetGroup();
        if (infoGroup == group)
        {
            continue;
        }
        // not in my group - may be flee target
        float coef = 1;
        if (info._type->IsAttendant())
        {
            coef = 1 / 2.5f;
        }
        else if (info._type->GetMaxRepairCargo() > 0)
        {
            if (veh->GetRepairCargo() <= 0)
            {
                coef = 1 / 0.1f;
            }
            else
            {
                coef = 1 / 2.0f;
            }
        }
        else if (info._type->GetMaxAmmoCargo() > 0)
        {
            if (veh->GetAmmoCargo() <= 0)
            {
                coef = 1 / 0.1f;
            }
            else
            {
                coef = 1 / 1.1f;
            }
        }
        else if (info._type->GetMagazineCargo().Size() > 0)
        {
            // assume some ammo is there
            coef = 1 / 1.1f;
        }
        else if (info._type->GetMaxFuelCargo() > 0)
        {
            if (veh->GetFuelCargo() <= 0)
            {
                coef = 1 / 0.1f;
            }
            else
            {
                coef = 1 / 0.6f;
            }
        }
        else
        {
            continue;
        }

        float cost = center->GetExposurePessimistic(info._realPos);

        float dist = (posL - info._realPos).SizeXZ();

        cost += dist * costPerDist;
        cost *= coef;
        if (cost < minCost)
        {
            minCost = cost;
            bestInfo = &info;
        }
    }
    // check first waypoint cost
    float coef = 1 / 0.1f;

    const ArcadeWaypointInfo& wInfo = group->GetWaypoint(0);
    float cost = center->GetExposurePessimistic(wInfo.position);
    float dist = (posL - wInfo.position).SizeXZ();
    cost += dist * costPerDist;

    cost *= coef;

    if (minCost < cost)
    {
        forced = true;
        return bestInfo->_realPos;
    }
    else
    {
        return wInfo.position;
    }
}

static void ArcadeFlee(AIGroupContext* context)
{
    STATE_PREFIX

    AIUnit* leader = group->Leader();
    if (!leader)
    {
        return;
    }

    bool forced;
    mission->_destination = FindFleePoint(context->_group, forced);

    for (int i = 0; i < MAX_UNITS_PER_GROUP; i++)
    {
        AIUnit* unit = group->UnitWithID(i + 1);
        if (!unit)
        {
            continue;
        }
        if (!unit->IsFreeSoldier())
        {
            unit->SetSemaphore(AI::SemaphoreYellow);
        }
        else
        {
            unit->SetSemaphore(AI::SemaphoreGreen);
        }
    }

    Vector3 normal = VUp;
    leader->FindFreePosition(mission->_destination, normal);

    group->SetSemaphore(AI::SemaphoreGreen);
    group->MainSubgroup()->SetSpeedMode(SpeedFull);

    group->Move(group->MainSubgroup(), // who
                mission->_destination, // where
                Command::Undefined     // how
    );
}

static void CheckArcadeFlee(AIGroupContext* context)
{
    AIGroup* group = context->_group;
    AI_ERROR(group);
    if (group->IsPlayerGroup())
    {
        return;
    }

    if (group->GetAllDone())
    {
        for (int i = 0; i < MAX_UNITS_PER_GROUP; i++)
        {
            AIUnit* unit = group->UnitWithID(i + 1);
            if (unit)
            {
                unit->SetSemaphore(AI::SemaphoreYellow);
            }
        }
        group->SetSemaphore(AI::SemaphoreYellow);
        group->MainSubgroup()->SetSpeedMode(SpeedNormal);
        group->CalculateMaximalStrength();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Registration of FSMs

