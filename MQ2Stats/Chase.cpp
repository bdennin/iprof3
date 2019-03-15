
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

void Chase::LookAt(FLOAT X, FLOAT Y, FLOAT Z)
{
	if(PCHARINFO pChar = GetCharInfo())
	{
		if(pChar->pSpawn)
		{
			float angle = (atan2(X - pChar->pSpawn->X, Y - pChar->pSpawn->X) * 256.0f / (float)PI);
			if(angle >= 512.0f)
				angle -= 512.0f;
			if(angle < 0.0f)
				angle += 512.0f;
			((PSPAWNINFO)pCharSpawn)->Heading = (FLOAT)angle;
			gFaceAngle                        = 10000.0f;
			if(pChar->pSpawn->FeetWet)
			{
				float locdist              = GetDistance(pChar->pSpawn->X, pChar->pSpawn->Y, X, Y);
				pChar->pSpawn->CameraAngle = (atan2(Z + 0.0f * 0.9f - pChar->pSpawn->Z - pChar->pSpawn->AvatarHeight * 0.9f, locdist) * 256.0f / (float)PI);
			}
			else if(pChar->pSpawn->mPlayerPhysicsClient.Levitate == 2)
			{
				if(Z < pChar->pSpawn->Z - 5)
					pChar->pSpawn->CameraAngle = -64.0f;
				else if(Z > pChar->pSpawn->Z + 5)
					pChar->pSpawn->CameraAngle = 64.0f;
				else
					pChar->pSpawn->CameraAngle = 0.0f;
			}
			else
				pChar->pSpawn->CameraAngle = 0.0f;
			gLookAngle = 10000.0f;
		}
	}
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

void Chase::ClearOne(std::list<SpawnData>::iterator& CurList)
{
	std::list<SpawnData>::iterator PosList;
	PosList    = CurList;
	CurList->x = 0;
	CurList->y = 0;
	CurList->z = 0;
	CurList++;
	FollowPath.erase(PosList);
}

void Chase::AddWaypoint(long SpawnID, bool Warping)
{
	if(PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(SpawnID))
	{
		SpawnData MonitorPosition;

		MonitorPosition.x = MonitorX = pSpawn->X;
		MonitorPosition.y = MonitorY = pSpawn->Y;
		MonitorPosition.z = MonitorZ = pSpawn->Z;
		MonitorPosition.heading = MonitorHeading = pSpawn->Heading;

		FollowPath.push_back(MonitorPosition);
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
	FLOAT Angle = (FLOAT)(atan2f(X - GetCharInfo()->pSpawn->X, Y - GetCharInfo()->pSpawn->Y) * 180.0f / PI);
	if(Angle < 0)
		Angle += 360;
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

	FLOAT Angle1 = GetCharInfo()->pSpawn->Heading - Angel;
	if(Angle1 < 0)
	{
		Low = true;
		Angle1 += 512.0f;
	}

	FLOAT Angle2 = GetCharInfo()->pSpawn->Heading + Angel;
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

void Chase::FollowSpawn()
{
	if(FollowPath.size())
	{
		if(GetDistance3D(MeMonitorX, MeMonitorY, MeMonitorZ, GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z) > 50)
		{
			std::list<SpawnData>::iterator CurList = FollowPath.begin();
			std::list<SpawnData>::iterator EndList = FollowPath.end();
			do
			{
				if(CurList == EndList)
					break;
				ClearOne(CurList);
			} while(true);
			WriteChatf("[MQ2AdvPath] Warping Detected on SELF");
		}
		if(!FollowPath.size())
			return;

		std::list<SpawnData>::iterator CurList = FollowPath.begin();
		std::list<SpawnData>::iterator EndList = FollowPath.end();

		bool run = false;

		if(PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(MonitorID))
		{
			if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pSpawn->X, pSpawn->Y, pSpawn->Z) >= 50)
				run = true;
			if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pSpawn->X, pSpawn->Y, pSpawn->Z) <= FollowSpawnDistance)
			{
				DoStop();
				return;
			}
		}
		else if((FollowPath.size() * DISTANCE_BETWEN_LOG) >= 20)
			run = true;
		//		 else if( GetDistance3D(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,GetCharInfo()->pSpawn->Z,CurList->x,CurList->y,CurList->z) >= 20 ) run = true;

		//		if( ( gbInForeground && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->x,CurList->y) > DISTANCE_BETWEN_LOG ) || ( !gbInForeground && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->x,CurList->y) > (DISTANCE_BETWEN_LOG+10) ) ) {
		if(GetDistance(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, CurList->x, CurList->y) > DISTANCE_BETWEN_LOG + DistanceMod)
		{
			LookAt(CurList->x, CurList->y, CurList->z);
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
		std::list<SpawnData>::iterator CurList = FollowPath.begin();
		std::list<SpawnData>::iterator LastList;
		std::list<SpawnData>::iterator EndList = FollowPath.end();

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

//
//#include "../MQ2Plugin.h"
//
//#include <direct.h>
//#include <list>
//#include <queue>
//#include <vector>
//
//PreSetup("MQ2AdvPath");
//PLUGIN_VERSION(9.2);
//using namespace std;
//
//#define FOLLOW_OFF 0
//#define FOLLOW_FOLLOWING 1
//#define FOLLOW_PLAYING 2
//#define FOLLOW_RECORDING 3
//
//#define STATUS_OFF 0
//#define STATUS_ON 1
//#define STATUS_PAUSED 2
//
//#define DISTANCE_BETWEN_LOG 5
//#define DISTANCE_OPEN_DOOR_CLOSE 10
//#define ANGEL_OPEN_DOOR_CLOSE 50.0
//#define DISTANCE_OPEN_DOOR_LONG 15
//#define ANGEL_OPEN_DOOR_LONG 95.0
//
//#define ZONE_TIME 6000
//
//// Timer Structure
//struct SpawnData
//{
//	FLOAT X;
//	FLOAT Y;
//	FLOAT Z;
//	FLOAT Heading;
//	char CheckPoint[MAX_STRING];
//	bool Warping;
//	bool Eval;
//} pSpawnData;
//
////struct PathInfo {
////	char Path[MAX_STRING];
////    bool PlaySmart;
////    bool PlayEval;
////    int  PlayDirection;				// Play Direction - replaces PlayReverse
////    long PlayWaypoint;
////} pPathInfo;
//
//long FollowState = FOLLOW_OFF; // Active?
//long StatusState = STATUS_OFF; // Active?
//
//long FollowSpawnDistance = 10; // Active?
//
//long FollowIdle    = 0; // FollowIdle time when Follow on?
//long NextClickDoor = 0; // NextClickDoor when Follow on?
//long PauseDoor     = 0; // PauseDoor paused Follow on and near door?
//bool AutoOpenDoor  = true;
//
//bool PlayOne      = false; // Play one frame only - used for debugging
//bool PlaySlow     = false; // Play Slow - turns on walk and doesn't skip points when in background
//bool PlaySmart    = false;
//bool PlayEval     = true;
//int PlayDirection = 1; // Play Direction - replaces PlayReverse
//					   //bool PlayReverse   = false;			// Play Reversed ?
//bool PlayLoop = false; // Play Loop? a->b a->b a->b a->b ....
//					   //bool PlayReturn  = false;			// Play Return? a->b b->a
//long PlayWaypoint  = 0;
//bool PlayZone      = false;
//long PlayZoneTick  = 0;
//long unPauseTick   = 0;
//bool AdvPathStatus = true;
//
//// Pulling Mode Variables Here
//bool PullMode       = false;                                                      // Mode for running paths when pulling.
//bool IFleeing       = false;                                                      // Setting this will play back pathing in reverse direction
//bool SaveMapPath    = false;                                                      // to control clearing the map path
//CHAR advFlag[11]    = { 'y', 'y', 'y', 'y', 'y', 'y', 'y', 'y', 'y', 'y', '\0' }; // Flags that can be used for controlling pathing.
//bool StopState      = false;
//CHAR custSearch[64] = { NULL };
//long begWayPoint    = 0;
//long endWayPoint    = 0;
//
//long MonitorID       = 0;     // Spawn To Monitor and follow
//FLOAT MonitorX       = 0;     // Spawn To MonitorX
//FLOAT MonitorY       = 0;     // Spawn To MonitorY
//FLOAT MonitorZ       = 0;     // Spawn To MonitorZ
//FLOAT MonitorHeading = 0;     // Spawn To MonitorHeading
//bool MonitorWarp     = false; // Spawn To Monitor has warped
//
//FLOAT MeMonitorX = 0; // MeMonitorX monitor your self
//FLOAT MeMonitorY = 0; // MeMonitorY monitor your self
//FLOAT MeMonitorZ = 0; // MeMonitorZ monitor your self
//
//CHAR Buffer[MAX_STRING]       = { 0 };    // Buffer for String manipulatsion
//CHAR SavePathName[MAX_STRING] = { NULL }; // Buffer for Save Path Name
//CHAR SavePathZone[MAX_STRING] = { NULL }; // Buffer for Save Zone Name
//long MaxChkPointDist          = 0;
//
//list<SpawnData> FollowPath; // FollowPath
//queue<string> PathList;
//
//class MQ2AdvPathType* pAdvPathType = 0;
//
//VOID MQRecordCommand(PSPAWNINFO pChar, PCHAR szLine);
//VOID MQPlayCommand(PSPAWNINFO pChar, PCHAR szLine);
//VOID MQFollowCommand(PSPAWNINFO pChar, PCHAR szLine);
//VOID MQAdvPathCommand(PSPAWNINFO pChar, PCHAR szLine);
//void ReleaseKeys();
//void DoWalk(bool walk = false);
//void DoFwd(bool hold, bool walk = false);
//void DoBck(bool hold);
//void DoLft(bool hold);
//void DoRgt(bool hold);
//void DoStop();
//void LookAt(FLOAT X, FLOAT Y, FLOAT Z);
//void NextPath(void);
//VOID SetPathEval(bool Eval);
//VOID ClearAll();
//VOID ClearOne(list<SpawnData>::iterator& CurList);
//VOID AddWaypoint(long SpawnID, bool Warping = false);
//PDOOR ClosestDoor();
//bool IsOpenDoor(PDOOR pDoor);
//VOID OpenDoor(bool force = false);
//bool InFront(float X, float Y, float Angel, bool Reverse = false);
//VOID SavePath(PCHAR PathName, PCHAR PathZone);
//VOID LoadPath(PCHAR PathName);
//VOID FollowSpawn();
//VOID FollowWaypoints();
//VOID ClearLag();
//VOID RecordingWaypoints();
//void __stdcall WarpingDetect(unsigned int ID, void* pData, PBLECHVALUE pValues);
//VOID ReplaceWaypointWith(char* s);
//VOID NextWaypoint();
//VOID ShowHelp(int helpFlag);
//
//vector<PMAPLINE> pFollowPath;
//
//unsigned long thisClock = clock();
//unsigned long lastClock = clock();
//long DistanceMod        = 0;
//
///*
////	Ingame commands:
////	/afollow    # Follow's your Target
//*/
//VOID MQFollowCommand(PSPAWNINFO pChar, PCHAR szLine)
//{
//	DebugSpewAlways("MQ2AdvPath::MQFollowCommand()");
//	if(!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus)
//		return;
//	bool doFollow       = false;
//	bool doAutoOpenDoor = true;
//	long MyTarget       = (pTarget) ? ((long)((PSPAWNINFO)pTarget)->SpawnID) : 0;
//
//	if(szLine[0] == 0)
//	{
//		if(MonitorID || FollowPath.size())
//		{
//			ClearAll();
//			return;
//		}
//		else
//		{
//			doFollow = true;
//		}
//	}
//	else
//	{
//		long iParm = 0;
//		do
//		{
//			GetArg(Buffer, szLine, ++iParm);
//			if(Buffer[0] == 0)
//				break;
//			if(!_strnicmp(Buffer, "on", 2))
//			{
//				doFollow = true;
//			}
//			else if(!_strnicmp(Buffer, "off", 3))
//			{
//				ClearAll();
//				return;
//			}
//			else if(!_strnicmp(Buffer, "pause", 5))
//			{
//				WriteChatf("[MQ2AdvPath] Follow Paused");
//				DoStop();
//				StatusState = STATUS_PAUSED;
//				return;
//			}
//			else if(!_strnicmp(Buffer, "unpause", 7))
//			{
//				WriteChatf("[MQ2AdvPath] Follow UnPaused");
//				StatusState = STATUS_ON;
//				return;
//			}
//			else if(!_strnicmp(Buffer, "spawn", 5))
//			{
//				GetArg(Buffer, szLine, ++iParm);
//				MyTarget = atol(Buffer);
//				doFollow = true;
//			}
//			else if(!_strnicmp(Buffer, "nodoor", 6))
//			{
//				doAutoOpenDoor = false;
//				doFollow       = true;
//			}
//			else if(!_strnicmp(Buffer, "door", 4))
//			{
//				doAutoOpenDoor = true;
//				doFollow       = true;
//			}
//			else if(!_strnicmp(Buffer, "help", 4))
//			{
//				ShowHelp(4);
//				return;
//			}
//			else
//			{
//				if(atol(Buffer))
//				{
//					if(atol(Buffer) < 1)
//						FollowSpawnDistance = 1;
//					else
//						FollowSpawnDistance = atol(Buffer);
//				}
//			}
//		} while(true);
//	}
//	if(doFollow)
//	{
//		ClearAll();
//		if(MyTarget == GetCharInfo()->pSpawn->SpawnID)
//			MonitorID = 0;
//		else
//			MonitorID = MyTarget;
//
//		if(PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(MonitorID))
//		{
//			AutoOpenDoor = doAutoOpenDoor;
//			AddWaypoint(MonitorID);
//			FollowState = FOLLOW_FOLLOWING;
//			StatusState = STATUS_ON;
//			WriteChatf("[MQ2AdvPath] Following %s", pSpawn->Name);
//
//			MeMonitorX = GetCharInfo()->pSpawn->X; // MeMonitorX monitor your self
//			MeMonitorY = GetCharInfo()->pSpawn->Y; // MeMonitorY monitor your self
//			MeMonitorZ = GetCharInfo()->pSpawn->Z; // MeMonitorZ monitor your self
//		}
//	}
//}
////Movement Related Functions
//void ReleaseKeys()
//{
//	DoWalk(false);
//	DoFwd(false);
//	DoBck(false);
//	DoRgt(false);
//	DoLft(false);
//}
//
//void DoWalk(bool walk)
//{
//	if(GetGameState() == GAMESTATE_INGAME && pLocalPlayer)
//	{
//		bool state_walking    = (*EQADDR_RUNWALKSTATE) ? false : true;
//		float SpeedMultiplier = *((float*)&(((PSPAWNINFO)pLocalPlayer)->SpeedMultiplier));
//		if(SpeedMultiplier < 0)
//			walk = false; // we're snared, dont go into walk mode no matter what
//		if((walk && !state_walking) || (!walk && state_walking))
//		{
//			MQ2Globals::ExecuteCmd(FindMappableCommand("run_walk"), 1, 0);
//			MQ2Globals::ExecuteCmd(FindMappableCommand("run_walk"), 0, 0);
//		}
//	}
//}
//
//void DoFwd(bool hold, bool walk)
//{
//	static bool held = false;
//	if(hold)
//	{
//		DoWalk(walk);
//		DoBck(false);
//		//if( !GetCharInfo()->pSpawn->SpeedRun || GetCharInfo()->pSpawn->PossiblyStuck || !((((gbMoving) && GetCharInfo()->pSpawn->SpeedRun==0.0f) && (GetCharInfo()->pSpawn->Mount ==  NULL )) || (fabs(FindSpeed((PSPAWNINFO)pCharSpawn)) > 0.0f )) )
//		if(!GetCharInfo()->pSpawn->SpeedRun && held)
//			held = false;
//		if(!held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("forward"), 1, 0);
//		held = true;
//	}
//	else
//	{
//		DoWalk(false);
//		if(held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("forward"), 0, 0);
//		held = false;
//	}
//}
//
//void DoBck(bool hold)
//{
//	static bool held = false;
//	if(hold)
//	{
//		DoFwd(false);
//		if(!held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("back"), 1, 0);
//		held = true;
//	}
//	else
//	{
//		if(held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("back"), 0, 0);
//		held = false;
//	}
//}
//
//void DoLft(bool hold)
//{
//	static bool held = false;
//	if(hold)
//	{
//		DoRgt(false);
//		if(!held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_left"), 1, 0);
//		held = true;
//	}
//	else
//	{
//		if(held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_left"), 0, 0);
//		held = false;
//	}
//}
//
//void DoRgt(bool hold)
//{
//	static bool held = false;
//	if(hold)
//	{
//		DoLft(false);
//		if(!held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_right"), 1, 0);
//		held = true;
//	}
//	else
//	{
//		if(held)
//			MQ2Globals::ExecuteCmd(FindMappableCommand("strafe_right"), 0, 0);
//		held = false;
//	}
//}
//
//void DoStop()
//{
//	if(!FollowIdle)
//		FollowIdle = (long)clock();
//	DoBck(true);
//	ReleaseKeys();
//}
//
//void LookAt(FLOAT X, FLOAT Y, FLOAT Z)
//{
//	if(PCHARINFO pChar = GetCharInfo())
//	{
//		if(pChar->pSpawn)
//		{
//			float angle = (atan2(X - pChar->pSpawn->X, Y - pChar->pSpawn->Y) * 256.0f / (float)PI);
//			if(angle >= 512.0f)
//				angle -= 512.0f;
//			if(angle < 0.0f)
//				angle += 512.0f;
//			((PSPAWNINFO)pCharSpawn)->Heading = (FLOAT)angle;
//			gFaceAngle                        = 10000.0f;
//			if(pChar->pSpawn->FeetWet)
//			{
//				float locdist              = GetDistance(pChar->pSpawn->X, pChar->pSpawn->Y, X, Y);
//				pChar->pSpawn->CameraAngle = (atan2(Z + 0.0f * 0.9f - pChar->pSpawn->Z - pChar->pSpawn->AvatarHeight * 0.9f, locdist) * 256.0f / (float)PI);
//			}
//			else if(pChar->pSpawn->mPlayerPhysicsClient.Levitate == 2)
//			{
//				if(Z < pChar->pSpawn->Z - 5)
//					pChar->pSpawn->CameraAngle = -64.0f;
//				else if(Z > pChar->pSpawn->Z + 5)
//					pChar->pSpawn->CameraAngle = 64.0f;
//				else
//					pChar->pSpawn->CameraAngle = 0.0f;
//			}
//			else
//				pChar->pSpawn->CameraAngle = 0.0f;
//			gLookAngle = 10000.0f;
//		}
//	}
//}
//
//VOID ClearAll()
//{
//	if(MonitorID || FollowPath.size())
//		WriteChatf("[MQ2AdvPath] Stopped");
//	FollowPath.clear();
//	unPauseTick = NextClickDoor = PauseDoor = FollowIdle = MonitorID = 0;
//	DoStop();
//
//	if(!StopState)
//		FollowState = FOLLOW_OFF; // Active?
//	StatusState = STATUS_OFF;     // Active?
//	PlayOne = MonitorWarp = PlayLoop = false;
//
//	SavePathZone[0] = NULL;
//	SavePathName[0] = NULL;
//}
//
//VOID ClearOne(list<SpawnData>::iterator& CurList)
//{
//	list<SpawnData>::iterator PosList;
//	PosList    = CurList;
//	CurList->x = 0;
//	CurList->y = 0;
//	CurList->z = 0;
//	CurList++;
//	FollowPath.erase(PosList);
//}
//
//VOID AddWaypoint(long SpawnID, bool Warping)
//{
//	if(PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(SpawnID))
//	{
//		SpawnData MonitorSpawnData;
//
//		MonitorSpawnData.Warping = Warping;
//
//		MonitorSpawnData.x = MonitorX = pSpawn->X;
//		MonitorSpawnData.y = MonitorY = pSpawn->Y;
//		MonitorSpawnData.z = MonitorZ = pSpawn->Z;
//		MonitorSpawnData.Heading = MonitorHeading = pSpawn->Heading;
//		strcpy_s(MonitorSpawnData.CheckPoint, "");
//		MonitorSpawnData.Eval = 0;
//
//		FollowPath.push_back(MonitorSpawnData);
//	}
//}
//
//PDOOR ClosestDoor()
//{
//	PDOORTABLE pDoorTable = (PDOORTABLE)pSwitchMgr;
//	FLOAT Distance        = 100000.00;
//	PDOOR pDoor           = NULL;
//	for(DWORD Count = 0; Count < pDoorTable->NumEntries; Count++)
//	{
//		if(Distance > GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoorTable->pDoor[Count]->DefaultX, pDoorTable->pDoor[Count]->DefaultY, pDoorTable->pDoor[Count]->DefaultZ))
//		{
//			Distance = GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoorTable->pDoor[Count]->DefaultX, pDoorTable->pDoor[Count]->DefaultY, pDoorTable->pDoor[Count]->DefaultZ);
//			pDoor    = pDoorTable->pDoor[Count];
//		}
//	}
//	return pDoor;
//}
//
//bool IsOpenDoor(PDOOR pDoor)
//{
//	//if(pDoor->DefaultHeading!=pDoor->Heading || pDoor->y!=pDoor->DefaultY || pDoor->x!=pDoor->DefaultX  || pDoor->z!=pDoor->DefaultZ )return true;
//	if(pDoor->State == 1 || pDoor->State == 2)
//		return true;
//	return false;
//}
//
//VOID OpenDoor(bool force)
//{
//	if(!AutoOpenDoor)
//		return;
//	if(force)
//	{
//		DoCommand(GetCharInfo()->pSpawn, "/click left door");
//	}
//	else if(PDOOR pDoor = (PDOOR)ClosestDoor())
//	{
//		//DoCommand(GetCharInfo()->pSpawn,"/doortarget id ");
//		if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoor->DefaultX, pDoor->DefaultY, pDoor->DefaultZ) < DISTANCE_OPEN_DOOR_CLOSE)
//		{
//			if(InFront(pDoor->x, pDoor->y, ANGEL_OPEN_DOOR_CLOSE, false) && !IsOpenDoor(pDoor) && NextClickDoor < (long)clock())
//			{
//				CHAR szTemp[MAX_STRING] = { 0 };
//				sprintf_s(szTemp, "id %d", pDoor->ID);
//				DoorTarget(GetCharInfo()->pSpawn, szTemp);
//				DoCommand(GetCharInfo()->pSpawn, "/click left door");
//				NextClickDoor = (long)clock() + 100;
//				if((PauseDoor - (long)clock()) < 0)
//				{
//					PauseDoor = (long)clock() + 1000;
//					DoStop();
//				}
//			}
//		}
//		else if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoor->DefaultX, pDoor->DefaultY, pDoor->DefaultZ) < DISTANCE_OPEN_DOOR_LONG)
//		{
//			if(InFront(pDoor->x, pDoor->y, ANGEL_OPEN_DOOR_LONG, false) && !IsOpenDoor(pDoor) && NextClickDoor < (long)clock())
//			{
//				CHAR szTemp[MAX_STRING] = { 0 };
//				sprintf_s(szTemp, "id %d", pDoor->ID);
//				DoorTarget(GetCharInfo()->pSpawn, szTemp);
//				DoCommand(GetCharInfo()->pSpawn, "/click left door");
//				NextClickDoor = (long)clock() + 100;
//			}
//		}
//	}
//}
//
//bool InFront(float X, float Y, float Angel, bool Reverse)
//{
//	FLOAT Angle = (FLOAT)((atan2f(X - GetCharInfo()->pSpawn->X, Y - GetCharInfo()->pSpawn->Y) * 180.0f / PI));
//	if(Angle < 0)
//		Angle += 360;
//	Angle = Angle * 1.42f;
//
//	if(Reverse)
//	{
//		if(Angle + 256 > 512)
//		{
//			Angle = Angle - 256;
//		}
//		else if(Angle - 256 < 0)
//		{
//			Angle = Angle + 256;
//		}
//	}
//
//	bool Low  = false;
//	bool High = false;
//
//	FLOAT Angle1 = GetCharInfo()->pSpawn->Heading - Angel;
//	if(Angle1 < 0)
//	{
//		Low = true;
//		Angle1 += 512.0f;
//	}
//
//	FLOAT Angle2 = GetCharInfo()->pSpawn->Heading + Angel;
//	if(Angle2 > 512.0f)
//	{
//		High = true;
//		Angle2 -= 512.0f;
//	}
//
//	if(Low)
//	{
//		if(Angle1 < (Angle + 512.0f) && Angle2 > Angle)
//			return true;
//	}
//	else if(High)
//	{
//		if(Angle1 < Angle && Angle2 > (Angle - 512.0f))
//			return true;
//	}
//	else if(Angle1 < Angle && Angle2 > Angle)
//	{
//		return true;
//	}
//
//	//	if( Angle1 < Angle && Angle2 > Angle ) return true;
//	return false;
//}
//
//VOID FollowSpawn()
//{
//	if(FollowPath.size())
//	{
//		if(GetDistance3D(MeMonitorX, MeMonitorY, MeMonitorZ, GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z) > 50)
//		{
//			list<SpawnData>::iterator CurList = FollowPath.begin();
//			list<SpawnData>::iterator EndList = FollowPath.end();
//			do
//			{
//				if(CurList == EndList)
//					break;
//				if(CurList->Warping)
//					break;
//				ClearOne(CurList);
//			} while(true);
//			WriteChatf("[MQ2AdvPath] Warping Detected on SELF");
//		}
//		if(!FollowPath.size())
//			return;
//
//		list<SpawnData>::iterator CurList = FollowPath.begin();
//		list<SpawnData>::iterator EndList = FollowPath.end();
//
//		bool run = false;
//
//		if(CurList->Warping && GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, CurList->x, CurList->y, CurList->z) > 50)
//		{
//			if(!MonitorWarp)
//			{
//				WriteChatf("[MQ2AdvPath] Warping Wating");
//				DoStop();
//			}
//			MonitorWarp = true;
//			return;
//		}
//		MonitorWarp = false;
//
//		if(PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(MonitorID))
//		{
//			if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pSpawn->X, pSpawn->Y, pSpawn->Z) >= 50)
//				run = true;
//			if(GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pSpawn->X, pSpawn->Y, pSpawn->Z) <= FollowSpawnDistance)
//			{
//				DoStop();
//				return;
//			}
//		}
//		else if((FollowPath.size() * DISTANCE_BETWEN_LOG) >= 20)
//			run = true;
//		//		 else if( GetDistance3D(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,GetCharInfo()->pSpawn->Z,CurList->x,CurList->y,CurList->z) >= 20 ) run = true;
//
//		//		if( ( gbInForeground && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->x,CurList->y) > DISTANCE_BETWEN_LOG ) || ( !gbInForeground && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->x,CurList->y) > (DISTANCE_BETWEN_LOG+10) ) ) {
//		if(GetDistance(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, CurList->x, CurList->y) > DISTANCE_BETWEN_LOG + DistanceMod)
//		{
//			LookAt(CurList->x, CurList->y, CurList->z);
//			if((PauseDoor - (long)clock()) < 300)
//			{
//				if(run)
//					DoFwd(true);
//				else
//					DoFwd(true, true);
//			}
//
//			OpenDoor();
//			FollowIdle = 0;
//		}
//		else
//		{
//			ClearOne(CurList);
//			if(CurList != EndList)
//			{
//				if(CurList->Warping)
//					return;
//				// Clean up lag
//				ClearLag();
//			}
//			else
//			{
//				DoStop();
//				if(!MonitorID)
//				{
//					OpenDoor(true);
//					ClearAll();
//				}
//			}
//		}
//	}
//}
//
//
//VOID ClearLag()
//{
//	if(FollowPath.size())
//	{
//		list<SpawnData>::iterator CurList = FollowPath.begin();
//		list<SpawnData>::iterator LastList;
//		list<SpawnData>::iterator EndList = FollowPath.end();
//
//		if(CurList != EndList)
//		{
//			if(CurList->Warping)
//				return;
//			if(InFront(CurList->x, CurList->y, 100, true))
//			{
//				CurList++;
//				for(int LagCount = 0; LagCount < 15; LagCount++)
//				{
//					if(CurList != EndList)
//					{
//						if((InFront(CurList->x, CurList->y, 100, false) && LagCount < 10) || (InFront(CurList->x, CurList->y, 50, false) && LagCount >= 10))
//						{
//							DebugSpewAlways("MQ2AdvPath::Removing lag() %d", LagCount);
//							CurList = FollowPath.begin();
//							for(int LagCount2 = 0; LagCount2 <= LagCount; LagCount2++)
//								ClearOne(CurList);
//							return;
//						}
//					}
//					else
//					{
//						return;
//					}
//					CurList++;
//				}
//			}
//		}
//	}
//}
//
//void FillInPath(char* PathName, list<SpawnData>& thepath, std::map<std::string, std::list<SpawnData>>& themap)
//{
//	CHAR INIFileNameTemp[MAX_STRING] = { 0 };
//	sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gszINIPath, GetShortZone(GetCharInfo()->zoneId));
//	char szTemp[MAX_STRING]  = { 0 };
//	char szTemp3[MAX_STRING] = { 0 };
//	int i                    = 1;
//	do
//	{
//		char szTemp2[MAX_STRING] = { 0 };
//		char szTemp3[MAX_STRING] = { 0 };
//		sprintf_s(szTemp, "%d", i);
//		GetPrivateProfileString(PathName, szTemp, NULL, szTemp2, MAX_STRING, INIFileNameTemp);
//		if(szTemp2[0] == 0)
//			break;
//		SpawnData TempSpawnData;
//
//		GetArg(szTemp3, szTemp2, 1);
//		TempSpawnData.y = (FLOAT)atof(szTemp3);
//		GetArg(szTemp3, szTemp2, 2);
//		TempSpawnData.x = (FLOAT)atof(szTemp3);
//		GetArg(szTemp3, szTemp2, 3);
//		TempSpawnData.z = (FLOAT)atof(szTemp3);
//
//		strcpy_s(TempSpawnData.CheckPoint, GetNextArg(szTemp2, 3));
//
//		TempSpawnData.Heading = 0;
//		TempSpawnData.Warping = 0;
//		TempSpawnData.Eval    = 1;
//
//		thepath.push_back(TempSpawnData);
//		i++;
//	} while(true);
//	themap[PathName] = thepath;
//}
//
//class MQ2AdvPathType : public MQ2Type
//{
//private:
//	char Temps[MAX_STRING];
//
//public:
//	enum AdvPathMembers
//	{
//		Active        = 1,
//		State         = 2,
//		Waypoints     = 3,
//		NextWaypoint  = 4,
//		Y             = 5,
//		X             = 6,
//		Z             = 7,
//		Monitor       = 8,
//		Idle          = 9,
//		Length        = 10,
//		Following     = 11,
//		Status        = 14,
//		Paused        = 15,
//	};
//	MQ2AdvPathType()
//		: MQ2Type("AdvPath")
//	{
//		TypeMember(Active);
//		TypeMember(State);
//		TypeMember(Waypoints);
//		TypeMember(NextWaypoint);
//		TypeMember(Y);
//		TypeMember(X);
//		TypeMember(Z);
//		TypeMember(Monitor);
//		TypeMember(Idle);
//		TypeMember(Length);
//		TypeMember(Following);
//		TypeMember(Status);
//		TypeMember(Paused);
//	}
//	bool MQ2AdvPathType::GETMEMBER()
//	{
//		list<SpawnData>::iterator CurList = FollowPath.begin();
//		list<SpawnData>::iterator EndList = FollowPath.end();
//		int i                            = 1;
//		float TheLength                  = 0;
//		int flag                         = 0;
//
//		//WriteChatf("[MQ2AdvPath] AdvPath Member: %s %s",Member,Index);
//
//		PMQ2TYPEMEMBER pMember = MQ2AdvPathType::FindMember(Member);
//		if(pMember)
//			switch((AdvPathMembers)pMember->ID)
//			{
//			case Active: // Plugin on and Ready
//				Dest.DWord = (gbInZone && GetCharInfo() && GetCharInfo()->pSpawn && AdvPathStatus);
//				Dest.Type  = pBoolType;
//				return true;
//			case State: // FollowState, 0 = off, 1 = Following, 2 = Playing, 3 = Recording
//				Dest.DWord = FollowState;
//				Dest.Type  = pIntType;
//				return true;
//			case Waypoints: // Number of Waypoints
//				Dest.DWord = FollowPath.size();
//				Dest.Type  = pIntType;
//				return true;
//			case NextWaypoint: // Next Waypoint
//				Dest.DWord = PlayWaypoint;
//				Dest.Type  = pIntType;
//				return true;
//			case Y: // Waypoint Y
//				while(CurList != EndList)
//				{
//					if(i == atol(Index) || (Index[0] != 0 && !_stricmp(Index, CurList->CheckPoint)))
//					{
//						Dest.Float = CurList->y;
//						Dest.Type  = pFloatType;
//						return true;
//					}
//					i++;
//					CurList++;
//				}
//				strcpy_s(Temps, "NULL");
//				Dest.Type = pStringType;
//				Dest.Ptr  = Temps;
//				return true;
//			case X: // Waypoint X
//				while(CurList != EndList)
//				{
//					if(i == atol(Index) || (Index[0] != 0 && !_stricmp(Index, CurList->CheckPoint)))
//					{
//						Dest.Float = CurList->x;
//						Dest.Type  = pFloatType;
//						return true;
//					}
//					i++;
//					CurList++;
//				}
//				strcpy_s(Temps, "NULL");
//				Dest.Type = pStringType;
//				Dest.Ptr  = Temps;
//				return true;
//			case Z: // Waypoint Z
//				while(CurList != EndList)
//				{
//					if(i == atol(Index) || (Index[0] != 0 && !_stricmp(Index, CurList->CheckPoint)))
//					{
//						Dest.Float = CurList->z;
//						Dest.Type  = pFloatType;
//						return true;
//					}
//					i++;
//					CurList++;
//				}
//				strcpy_s(Temps, "NULL");
//				Dest.Type = pStringType;
//				Dest.Ptr  = Temps;
//				return true;
//			case Monitor: // Spawn your following
//				Dest.Ptr  = (PSPAWNINFO)GetSpawnByID(MonitorID);
//				Dest.Type = pSpawnType;
//				return true;
//			case Idle: // FollowIdle time when following and not moving
//				Dest.DWord = (FollowState && FollowIdle) ? (((long)clock() - FollowIdle) / 1000) : 0;
//				Dest.Type  = pIntType;
//				return true;
//			case Length: // Estimated length off the follow path
//				if(FollowPath.size())
//				{
//					list<SpawnData>::iterator CurList = FollowPath.begin();
//					TheLength                        = GetDistance(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, CurList->x, CurList->y);
//					if(FollowPath.size() > 1)
//						TheLength = ((FollowPath.size() - 1) * DISTANCE_BETWEN_LOG) + TheLength;
//				}
//				Dest.Float = TheLength;
//				Dest.Type  = pFloatType;
//				return true;
//			case Following:
//				Dest.DWord = (FollowState == FOLLOW_FOLLOWING);
//				Dest.Type  = pBoolType;
//				return true;
//			case Status:
//				Dest.DWord = StatusState;
//				Dest.Type  = pIntType;
//				return true;
//			case Paused:
//				Dest.DWord = (StatusState == STATUS_PAUSED);
//				Dest.Type  = pBoolType;
//				return true;
//			}
//		strcpy_s(DataTypeTemp, "NULL");
//		Dest.Type = pStringType;
//		Dest.Ptr  = &DataTypeTemp[0];
//		return true;
//	}
//	bool ToString(MQ2VARPTR VarPtr, PCHAR Destination)
//	{
//		strcpy_s(Destination, MAX_STRING, "TRUE");
//		return true;
//	}
//	bool FromData(MQ2VARPTR& VarPtr, MQ2TYPEVAR& Source)
//	{
//		return false;
//	}
//	bool FromString(MQ2VARPTR& VarPtr, PCHAR Source)
//	{
//		return false;
//	}
//	~MQ2AdvPathType()
//	{
//	}
//};
//
//BOOL dataAdvPath(PCHAR szName, MQ2TYPEVAR& Dest)
//{
//	Dest.DWord = 1;
//	Dest.Type  = pAdvPathType;
//	return true;
//}
//
//VOID ShowHelp(int helpFlag)
//{
//	WriteChatColor("========= Advance Pathing Help =========", CONCOLOR_YELLOW);
//	//WriteChatf("");
//	switch(helpFlag)
//	{
//	case 1:
//		WriteChatColor("/play [pathName|off] [stop] [pause|unpause] [loop|noloop] [normal|reverse] [smart|nosmart] [flee|noflee] [door|nodoor] [fast|slow] [eval|noeval] [zone|nozone] [list] [listcustom] [show] [help] [setflag1]-[setflag9] y/n [resetflags]", CONCOLOR_GREEN);
//		WriteChatColor(" The /play command will execute each command on the line. The commands that should be used single, or at the end of a line:", CONCOLOR_GREEN);
//		WriteChatColor("   [off] [list] [listcustom] [show] [help]", CONCOLOR_GREEN);
//		WriteChatf("");
//		WriteChatColor("setflag1-setflag9 can be used anywhere in the line and uses the following format:", CONCOLOR_GREEN);
//		WriteChatColor(" /play setflag1 n pathname or /play pathname setflag n (n can be any single alpha character)", CONCOLOR_GREEN);
//		WriteChatColor(" AdvPath flags can be accessed using ${AdvPath.Flag1} - ${AdvPath.Flag9}", CONCOLOR_GREEN);
//		WriteChatColor(" /play resetflags resets all flags(1-9) to 'y'", CONCOLOR_GREEN);
//		break;
//	case 2:
//		WriteChatColor("/record ", CONCOLOR_GREEN);
//		WriteChatColor("/record save <PathName> ##", CONCOLOR_GREEN);
//		WriteChatColor("/record Checkpoint <checkpointname>", CONCOLOR_GREEN);
//		WriteChatColor("/record help", CONCOLOR_GREEN);
//		WriteChatColor("## is the distance between checkpoints to force checkpoints to be writen to the path file", CONCOLOR_GREEN);
//		break;
//	case 3:
//		WriteChatColor("/advpath [on|off] [pull|nopull] [customsearch value] [save] [help]", CONCOLOR_GREEN);
//		WriteChatColor("  For Addional help use:", CONCOLOR_YELLOW);
//		WriteChatColor("    /play help:", CONCOLOR_GREEN);
//		WriteChatColor("    /record help", CONCOLOR_GREEN);
//		WriteChatColor("    /afollow help:", CONCOLOR_GREEN);
//		break;
//	case 4:
//		WriteChatColor("/afollow [on|off] [pause|unpause] [slow|fast] ", CONCOLOR_GREEN);
//		WriteChatColor("/afollow spawn # [slow|fast] - default=fast", CONCOLOR_GREEN);
//		WriteChatColor("/afollow help", CONCOLOR_GREEN);
//	}
//	//WriteChatf("");
//	WriteChatColor("========================================", CONCOLOR_YELLOW);
//}
//// Set Active Status true or false.
//VOID MQAdvPathCommand(PSPAWNINFO pChar, PCHAR szLine)
//{
//	if(!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn)
//		return;
//	DebugSpewAlways("MQ2AdvPath::MQAdvPathCommand()");
//	long iParm = 0;
//	do
//	{
//		GetArg(Buffer, szLine, ++iParm);
//		if(Buffer[0] == 0)
//			break;
//
//		if(!_strnicmp(Buffer, "off", 3))
//		{
//			ClearAll();
//			while(!PathList.empty())
//				PathList.pop();
//			AdvPathStatus = false;
//		}
//		else if(!_strnicmp(Buffer, "on", 2))
//		{
//			AdvPathStatus = true;
//		}
//		else if(!_strnicmp(Buffer, "help", 4))
//			ShowHelp(3);
//		break;
//	} while(true);
//}
//
//// Called once, when the plugin is to initialize
//PLUGIN_API VOID InitializePlugin(VOID)
//{
//	AdvPathStatus = true;
//
//	AddCommand("/afollow", MQFollowCommand);
//	AddCommand("/advpath", MQAdvPathCommand);
//	pAdvPathType = new MQ2AdvPathType;
//	AddMQ2Data("AdvPath", dataAdvPath);
//}
//
//// Called once, when the plugin is to shutdown
//PLUGIN_API VOID ShutdownPlugin(VOID)
//{
//	RemoveCommand("/afollow");
//	RemoveCommand("/advpath");
//	delete pAdvPathType;
//	RemoveMQ2Data("AdvPath");
//	ClearAll();
//}
//
//// This is called every time MQ pulses
//PLUGIN_API VOID OnPulse(VOID)
//{
//	if(!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus)
//		return;
//
//	if(FollowState == FOLLOW_FOLLOWING && StatusState == STATUS_ON)
//		FollowSpawn();
//
//	if(unPauseTick && (long)GetTickCount() > unPauseTick)
//	{
//		unPauseTick = 0;
//		if(FollowState == FOLLOW_PLAYING && StatusState == STATUS_PAUSED)
//		{
//			WriteChatf("[MQ2AdvPath] Playing UnPaused");
//			StatusState = STATUS_ON;
//		}
//	}
//
//	MeMonitorX = GetCharInfo()->pSpawn->X; // MeMonitorX monitor your self
//	MeMonitorY = GetCharInfo()->pSpawn->Y; // MeMonitorY monitor your self
//	MeMonitorZ = GetCharInfo()->pSpawn->Z; // MeMonitorZ monitor your self
// }
//
//PLUGIN_API VOID OnEndZone(VOID)
//{
//	ClearAll();
//}
//
//PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
//{
//	if(pSpawn->SpawnID == MonitorID)
//		MonitorID = 0;
//}
//
//PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
//{
//	if(!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus)
//		return 0;
//	if(!_stricmp(Line, "You have been summoned!") && FollowState)
//	{
//		WriteChatf("[MQ2AdvPath] summon Detected");
//		ClearAll();
//	}
//	else if(!_strnicmp(Line, "You will now auto-follow", 24))
//		DoLft(true);
//	else if(!_strnicmp(Line, "You are no longer auto-follow", 29) || !_strnicmp(Line, "You must first target a group member to auto-follow.", 52))
//	{
//		if(FollowState)
//			MQFollowCommand(GetCharInfo()->pSpawn, "off");
//		else
//			MQFollowCommand(GetCharInfo()->pSpawn, "on");
//	}
//	return 0;
//}
