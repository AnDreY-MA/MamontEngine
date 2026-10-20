#pragma once

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimeStamp;

namespace MamontEngine
{

	class TimeStep
	{
    public:
        explicit TimeStep();
        ~TimeStep();

        void Update();

        inline double GetMillis() const { return m_Timestep; }

        inline double GetSeconds() const { return m_Timestep * 0.001; }

	private:
        double m_Timestep{0.0};
        double m_LastTime{0.0};
        double m_Elapsed{0.0};

		TimeStamp m_Start;
	};
}