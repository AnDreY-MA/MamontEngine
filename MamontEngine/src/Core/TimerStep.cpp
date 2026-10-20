#include "TimeStep.h"

namespace
{
    double Duration(TimeStamp start, TimeStamp end, double timeResolution = 1.0)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() * timeResolution;
    }

    double GetElapsed(TimeStamp start, double timeResolution = 1.0)
    {
        return Duration(start, std::chrono::high_resolution_clock::now(), timeResolution);
    }
}

namespace MamontEngine
{
    TimeStep::TimeStep()
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    TimeStep::~TimeStep()
    {

    }

    void TimeStep::Update()
    {
        double currentTime = GetElapsed(m_Start);
        double maxFrameTime = -1.0;
        double dt           = currentTime - m_LastTime;

        while (dt < maxFrameTime)
        {
            currentTime = GetElapsed(m_Start);
            dt = currentTime - m_LastTime;
        }

        m_Timestep = currentTime - m_LastTime;
        m_LastTime = currentTime;
        m_Elapsed += m_Timestep;
    }
}