#include <string>

struct PriorityInfo
{
    int checkinId;
    int patientId;

    std::string patientName;
    std::string department;
    std::string checkinTime;

    int basePriority;
    int currentPriority;

    std::string lastUpdate;
};