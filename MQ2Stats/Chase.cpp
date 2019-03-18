
#include "Chase.h"

Chase::Chase()
	: FollowState(FOLLOW_OFF)
	, StatusState(STATUS_OFF)
	, FollowSpawnDistance(10)
	, FollowIdle(0)
	, NextClickDoor(0)
	, PauseDoor(0)
	, AutoOpenDoor(true)
	, AdvPathStatus(true)
	, StopState(false)
	, thisClock(clock())
	, lastClock(clock())
	, DistanceMod(0)
	, MonitorID(0)
	, MonitorX(0)
	, MonitorY(0)
	, MonitorZ(0)
	, MonitorHeading(0)
	, MonitorWarp(false)
	, MeMonitorX(0)
	, MeMonitorY(0)
	, MeMonitorZ(0)
{
}

Chase::~Chase()
{
	ClearAll();
}

void Chase::ReleaseKeys()
{
	DoWalk(false);
	DoFwd(false);
	DoBck(false);
	DoRgt(false);
	DoLft(false);
}

void Chase::DoWalk(bool walk)
{
	if(GetGameState() == GAMESTATE_INGAME && pLocalPlayer)
	{
		bool state_walking    = (*EQADDR_RUNWALKSTATE) ? false : true;
		float SpeedMultiplier = *((float*)&(((PSPAWNINFO)pLocalPlayer)->SpeedMultiplier));
		if(SpeedMultiplier < 0)
			walk = false; // we're snared, dont go into walk mode no matter what
		if((walk && !state_walking) || (!walk && state_walking))
		{
			MQ2Globals::ExecuteCmd(FindMappableCommand("run_walk"), 1, 0);
			MQ2Globals::ExecuteCmd(FindMappableCommand("run_walk"), 0, 0);
		}
	}
}

void Chase::DoFwd(bool hold, bool walk)
{
	static bool held = false;
	if(hold)
	{
		DoWalk(walk);
		DoBck(false);
		//if( !GetCharInfo()->pSpawn->SpeedRun || GetCharInfo()->pSpawn->PossiblyStuck || !((((gbMoving) && GetCharInfo()->pSpawn->SpeedRun==0.0f) && (GetCharInfo()->pSpawn->Mount ==  NULL )) || (fabs(FindSpeed((PSPAWNINFO)pCharSpawn)) > 0.0f )) )
		if(!GetCharInfo()->pSpawn->SpeedRun && held)
			held = false;
		if(!held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("forward"), 1, 0);
		held = true;
	}
	else
	{
		DoWalk(false);
		if(held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("forward"), 0, 0);
		held = false;
	}
}

void Chase::DoBck(bool hold)
{
	static bool held = false;
	if(hold)
	{
		DoFwd(false);
		if(!held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("back"), 1, 0);
		held = true;
	}
	else
	{
		if(held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("back"), 0, 0);
		held = false;
	}
}

void Chase::DoLft(bool hold)
{
	static bool held = false;
	if(hold)
	{
		DoRgt(false);
		if(!held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_left"), 1, 0);
		held = true;
	}
	else
	{
		if(held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_left"), 0, 0);
		held = false;
	}
}

void Chase::DoRgt(bool hold)
{
	static bool held = false;
	if(hold)
	{
		DoLft(false);
		if(!held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_right"), 1, 0);
		held = true;
	}
	else
	{
		if(held)
			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_right"), 0, 0);
		held = false;
	}
}

void Chase::DoStop()
{
	if(!FollowIdle)
		FollowIdle = (long)clock();
	DoBck(true);
	ReleaseKeys();
}

void Chase::LookAt(FLOAT x, FLOAT y, FLOAT z)
{
	char buf[MAX_STRING] = { 0 };
	PSPAWNINFO p_self    = GetCharInfo()->pSpawn;

    snprintf(buf, MAX_STRING, "/face fast loc %d, %d", (int32_t)y, (int32_t)x);

	DoCommand(p_self, buf);
	/*
	gFaceAngle = (atan2(x - p_self->X, y - p_self->Y) * 256.0f / PI);

	if(gFaceAngle >= 512.0f)
		gFaceAngle -= 512.0f;

	if(gFaceAngle < 0.0f)
		gFaceAngle += 512.0f;

	p_self->Heading = (FLOAT)gFaceAngle;

	gFaceAngle = 10000.0f;

    if(p_self->UnderWater == 5 || p_self->FeetWet == 5)
    {
		p_self->CameraAngle = (FLOAT)(atan2(z - p_self->Z, (FLOAT)GetDistance(p_self->X, p_self->Y, x, y)) * 256.0f / PI);
    }
	else if(p_self->mPlayerPhysicsClient.Levitate == 2)
	{
        if(z < p_self->Z - 5)
        {
			p_self->CameraAngle = -45.0f;
        }
        else if(z > p_self->Z + 5)
        {
			p_self->CameraAngle = 45.0f;
        }
        else
        {
			p_self->CameraAngle = 0.0f;
        }
	}
    else
    {
		p_self->CameraAngle = 0.0f;
    }

	gLookAngle */
	//= 10000.0f;

	//PSPAWNINFO p_self = GetCharInfo()->pSpawn;

	//if(nullptr != p_self)
	//{
	//	float angle = (atan2(x - p_self->X, y - p_self->Y) * 256.0f / (float)PI);

	//	if(angle >= 512.0f)
	//	{
	//		angle -= 512.0f;
	//	}
	//	else if(angle < 0.0f)
	//	{
	//		angle += 512.0f;
	//	}

	//	p_self->Heading = (FLOAT)angle;
	//	gFaceAngle      = 10000.0f;

	//	if(p_self->FeetWet)
	//	{
	//		float locdist       = GetDistance(p_self->X, p_self->Y, x, y);
	//		p_self->CameraAngle = (atan2(z + 0.0f * 0.9f - p_self->Z - p_self->AvatarHeight * 0.9f, locdist) * 256.0f / (float)PI);
	//	}
	//	else if(p_self->mPlayerPhysicsClient.Levitate == 2)
	//	{
	//		if(z < p_self->Z - 5)
	//		{
	//			p_self->CameraAngle = -64.0f;
	//		}
	//		else if(z > p_self->Z + 5)
	//		{
	//			p_self->CameraAngle = 64.0f;
	//		}
	//		else
	//		{
	//			p_self->CameraAngle = 0.0f;
	//		}
	//	}
	//	else
	//	{
	//		p_self->CameraAngle = 0.0f;
	//	}

	//	gLookAngle = 10000.0f;
	//}
}

void Chase::ClearAll()
{
	if(MonitorID || FollowPath.size())
		WriteChatf("[MQ2AdvPath] Stopped");
	FollowPath.clear();
	NextClickDoor = PauseDoor = FollowIdle = MonitorID = 0;
	DoStop();

	if(!StopState)
		FollowState = FOLLOW_OFF; // Active?
	StatusState = STATUS_OFF;     // Active?
}

void Chase::ClearOne(std::list<Position>::iterator& CurList)
{
	std::list<Position>::iterator PosList;
	PosList    = CurList;
	CurList->x = 0;
	CurList->y = 0;
	CurList->z = 0;
	CurList++;
	FollowPath.erase(PosList);
}

void Chase::AddWaypoint(FLOAT x, FLOAT y, FLOAT z, FLOAT heading, FLOAT angle)
{

	if(GetDistance3D(x, y, z, MonitorX, MonitorY, MonitorZ) > DISTANCE_BETWEN_LOG)
	{
		WriteChatf("Added waypoint %d %d %d, m position %d %d %d", (int32_t)x, (int32_t)y, (int32_t)z, (int32_t)GetCharInfo()->pSpawn->X, (int32_t)GetCharInfo()->pSpawn->Y, (int32_t)GetCharInfo()->pSpawn->Z);
		Position pos;

		pos.x = MonitorX = x;
		pos.y = MonitorY = y;
		pos.z = MonitorZ = z;
		pos.heading = MonitorHeading = heading;
		pos.angle = MonitorAngle = angle;

		FollowPath.push_back(pos);
	}
}

PDOOR Chase::ClosestDoor()
{
	PDOORTABLE pDoorTable = (PDOORTABLE)pSwitchMgr;
	FLOAT Distance        = 100000.00;
	PDOOR pDoor           = NULL;
	for(DWORD Count = 0; Count < pDoorTable->NumEntries; Count++)
	{
		if(Distance > GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoorTable->pDoor[Count]->DefaultX, pDoorTable->pDoor[Count]->DefaultY, pDoorTable->pDoor[Count]->DefaultZ))
		{
			Distance = GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoorTable->pDoor[Count]->DefaultX, pDoorTable->pDoor[Count]->DefaultY, pDoorTable->pDoor[Count]->DefaultZ);
			pDoor    = pDoorTable->pDoor[Count];
		}
	}
	return pDoor;
}

bool Chase::IsOpenDoor(PDOOR pDoor)
{
	//if(pDoor->DefaultHeading!=pDoor->Heading || pDoor->y!=pDoor->DefaultY || pDoor->x!=pDoor->DefaultX  || pDoor->z!=pDoor->DefaultZ )return true;
	if(pDoor->State == 1 || pDoor->State == 2)
		return true;
	return false;
}

void Chase::OpenDoor(bool force)
{
	if(!AutoOpenDoor)
		return;
	if(force)
	{
		DoCommand(GetCharInfo()->pSpawn, "/click left door");
	}
	else if(PDOOR pDoor = (PDOOR)ClosestDoor())
	{
		//DoCommand(GetCharInfo()->pSpawn,"/doortarget id ");
		if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoor->DefaultX, pDoor->DefaultY, pDoor->DefaultZ) < DISTANCE_OPEN_DOOR_CLOSE)
		{
			if(InFront(pDoor->X, pDoor->Y, ANGEL_OPEN_DOOR_CLOSE, false) && !IsOpenDoor(pDoor) && NextClickDoor < (long)clock())
			{
				CHAR szTemp[MAX_STRING] = { 0 };
				sprintf_s(szTemp, "id %d", pDoor->ID);
				DoorTarget(GetCharInfo()->pSpawn, szTemp);
				DoCommand(GetCharInfo()->pSpawn, "/click left door");
				NextClickDoor = (long)clock() + 100;
				if((PauseDoor - (long)clock()) < 0)
				{
					PauseDoor = (long)clock() + 1000;
					DoStop();
				}
			}
		}
		else if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoor->DefaultX, pDoor->DefaultY, pDoor->DefaultZ) < DISTANCE_OPEN_DOOR_LONG)
		{
			if(InFront(pDoor->X, pDoor->Y, ANGEL_OPEN_DOOR_LONG, false) && !IsOpenDoor(pDoor) && NextClickDoor < (long)clock())
			{
				CHAR szTemp[MAX_STRING] = { 0 };
				sprintf_s(szTemp, "id %d", pDoor->ID);
				DoorTarget(GetCharInfo()->pSpawn, szTemp);
				DoCommand(GetCharInfo()->pSpawn, "/click left door");
				NextClickDoor = (long)clock() + 100;
			}
		}
	}
}

bool Chase::InFront(float X, float Y, float Angel, bool Reverse)
{
	PSPAWNINFO p_self = GetCharInfo()->pSpawn;

	FLOAT Angle = (FLOAT)(atan2f(X - p_self->X, Y - p_self->Y) * 180.0f / PI);

	if(Angle < 0)
	{
		Angle += 360;
	}

	Angle = Angle * 1.42f;

	if(Reverse)
	{
		if(Angle + 256 > 512)
		{
			Angle = Angle - 256;
		}
		else if(Angle - 256 < 0)
		{
			Angle = Angle + 256;
		}
	}

	bool Low  = false;
	bool High = false;

	FLOAT Angle1 = p_self->Heading - Angel;
	if(Angle1 < 0)
	{
		Low = true;
		Angle1 += 512.0f;
	}

	FLOAT Angle2 = p_self->Heading + Angel;
	if(Angle2 > 512.0f)
	{
		High = true;
		Angle2 -= 512.0f;
	}

	if(Low)
	{
		if(Angle1 < (Angle + 512.0f) && Angle2 > Angle)
			return true;
	}
	else if(High)
	{
		if(Angle1 < Angle && Angle2 > (Angle - 512.0f))
			return true;
	}
	else if(Angle1 < Angle && Angle2 > Angle)
	{
		return true;
	}

	//	if( Angle1 < Angle && Angle2 > Angle ) return true;
	return false;
}

void Chase::Iterate()
{
	if(FollowPath.size())
	{
		std::list<Position>::iterator CurList = FollowPath.begin();
		std::list<Position>::iterator EndList = FollowPath.end();

		Position first = *CurList;
		Position last  = FollowPath.back();

		PSPAWNINFO p_self  = GetCharInfo()->pSpawn;
		FLOAT dist_to_last = GetDistance3D(p_self->X, p_self->Y, p_self->Z, last.x, last.y, last.z);

		bool run = false;

		if((dist_to_last >= 50) || (FollowPath.size() * DISTANCE_BETWEN_LOG >= 20))
		{
			run = true;
		}
		else if(dist_to_last <= FollowSpawnDistance)
		{
			DoStop();
			return;
		}

		if(GetDistance(p_self->X, p_self->Y, first.x, first.y) > DISTANCE_BETWEN_LOG + DistanceMod)
		{
			LookAt(first.x, first.y, first.z);

			if((PauseDoor - (long)clock()) < 300)
			{
				if(run)
					DoFwd(true);
				else
					DoFwd(true, true);
			}

			OpenDoor();
			FollowIdle = 0;
		}
		else
		{
			ClearOne(CurList);
			if(CurList != EndList)
			{
				// Clean up lag
				ClearLag();
			}
			else
			{
				DoStop();

				if(!MonitorID)
				{
					OpenDoor(true);
					ClearAll();
				}
			}
		}
	}
}

void Chase::ClearLag()
{
	if(FollowPath.size())
	{
		std::list<Position>::iterator CurList = FollowPath.begin();
		std::list<Position>::iterator LastList;
		std::list<Position>::iterator EndList = FollowPath.end();

		if(CurList != EndList)
		{
			if(InFront(CurList->x, CurList->y, 100, true))
			{
				CurList++;
				for(int LagCount = 0; LagCount < 15; LagCount++)
				{
					if(CurList != EndList)
					{
						if((InFront(CurList->x, CurList->y, 100, false) && LagCount < 10) || (InFront(CurList->x, CurList->y, 50, false) && LagCount >= 10))
						{
							DebugSpewAlways("MQ2AdvPath::Removing lag() %d", LagCount);
							CurList = FollowPath.begin();
							for(int LagCount2 = 0; LagCount2 <= LagCount; LagCount2++)
								ClearOne(CurList);
							return;
						}
					}
					else
					{
						return;
					}
					CurList++;
				}
			}
		}
	}
}
