
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

#define DISTANCE_BETWEN_LOG 10
#define DISTANCE_OPEN_DOOR_CLOSE 10
#define ANGEL_OPEN_DOOR_CLOSE 50.0
#define DISTANCE_OPEN_DOOR_LONG 15
#define ANGEL_OPEN_DOOR_LONG 95.0

#define ZONE_TIME 6000

class Chase
{
public:
	struct Position
	{
		FLOAT heading;
		FLOAT angle;
		FLOAT x;
		FLOAT y;
		FLOAT z;
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
	void ClearOne(std::list<Position>::iterator& CurList);
	void AddWaypoint(FLOAT X, FLOAT Y, FLOAT Z, FLOAT heading, FLOAT angle);
	PDOOR ClosestDoor();
	bool IsOpenDoor(PDOOR pDoor);
	void OpenDoor(bool force = false);
	bool InFront(float X, float Y, float Angel, bool Reverse = false);
	void Iterate();
	void ClearLag();

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

	bool MonitorWarp;
	FLOAT MonitorAngle;
	FLOAT MonitorHeading;
    FLOAT MonitorX;
	FLOAT MonitorY;
	FLOAT MonitorZ;

	FLOAT MeMonitorX;
	FLOAT MeMonitorY;
	FLOAT MeMonitorZ;

	std::list<Position> FollowPath; // FollowPath
};
