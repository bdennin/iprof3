
#include "../MQ2Plugin.h"

#include <list>
#include <vector>

#define FOLLOW_OFF 0
#define FOLLOW_FOLLOWING 1
#define FOLLOW_PLAYING 2
#define FOLLOW_RECORDING 3

#define STATUS_OFF 0
#define STATUS_ON 1
#define STATUS_PAUSED 2

#define DISTANCE_BETWEN_LOG 5
#define DISTANCE_OPEN_DOOR_CLOSE 10
#define ANGEL_OPEN_DOOR_CLOSE 50.0
#define DISTANCE_OPEN_DOOR_LONG 15
#define ANGEL_OPEN_DOOR_LONG 95.0

#define ZONE_TIME 6000

class Chase
{
public:
	struct SpawnData
	{
		std::string name;
		DWORD spawn_id;
		DWORD zone_id;
		bool zoning;
		DWORD level;
		FLOAT heading;
		FLOAT look;
		FLOAT x;
		FLOAT y;
		FLOAT z;
		DWORD hp_percent;
		DWORD hp_missing;
		DWORD mana_percent;
		DWORD aggro_percent;
	};

	Chase();
	~Chase();

	void ReleaseKeys();
	void DoWalk(bool walk = false);
	void DoFwd(bool hold, bool walk = false);
	void DoBck(bool hold);
	void DoLft(bool hold);
	void DoRgt(bool hold);
	void DoStop();
	void LookAt(FLOAT X, FLOAT Y, FLOAT Z);
	void ClearAll();
	void ClearOne(std::list<SpawnData>::iterator& CurList);
	void AddWaypoint(long SpawnID, bool Warping = false);
	PDOOR ClosestDoor();
	bool IsOpenDoor(PDOOR pDoor);
	void OpenDoor(bool force = false);
	bool InFront(float X, float Y, float Angel, bool Reverse = false);
	void FollowSpawn();
	void ClearLag();
	
    FLOAT MonitorX;
	FLOAT MonitorY;
	FLOAT MonitorZ;

private:
	long FollowState;
	long StatusState;
	long FollowSpawnDistance;
	long FollowIdle;
	long NextClickDoor;
	long PauseDoor;
	bool AutoOpenDoor;
	bool AdvPathStatus;
	bool StopState;
	unsigned long thisClock;
	unsigned long lastClock;
	long DistanceMod;
	long MonitorID;

	FLOAT MonitorHeading;
	bool MonitorWarp;
	FLOAT MeMonitorX;
	FLOAT MeMonitorY;
	FLOAT MeMonitorZ;
	long MaxChkPointDist;

	CHAR Buffer[MAX_STRING]       = { 0 };    // Buffer for String manipulatsion
	CHAR SavePathName[MAX_STRING] = { NULL }; // Buffer for Save Path Name
	CHAR SavePathZone[MAX_STRING] = { NULL }; // Buffer for Save Zone Name

	std::list<SpawnData> FollowPath; // FollowPath
	SpawnData* m_p_spawn;
};

//	DebugSpewAlways("MQ2AdvPath::MQFollowCommand()");
//	//if(!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus)
//		//return;
//	bool doFollow       = false;
//	bool doAutoOpenDoor = true;
//	long MyTarget       = (pTarget) ? ((long)((PSPAWNINFO)pTarget)->SpawnID) : 0;

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

//		if(PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(MonitorID))
//		{
//			AutoOpenDoor = doAutoOpenDoor;
//			AddWaypoint(MonitorID);
//			FollowState = FOLLOW_FOLLOWING;
//			StatusState = STATUS_ON;
//			WriteChatf("[MQ2AdvPath] Following %s", pSpawn->Name);

//			MeMonitorX = GetCharInfo()->pSpawn->X; // MeMonitorX monitor your self
//			MeMonitorY = GetCharInfo()->pSpawn->Y; // MeMonitorY monitor your self
//			MeMonitorZ = GetCharInfo()->pSpawn->Z; // MeMonitorZ monitor your self
//		}
//	})