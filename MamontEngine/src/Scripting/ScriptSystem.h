#pragma once

namespace sol
{
    class state;
}

namespace MamontEngine
{
    class Scene;

    class ScriptSystem
    {
    public:
        ScriptSystem();
        ~ScriptSystem();

        inline sol::state &GetState() { return *m_State; }

    private:
        sol::state *m_State{nullptr};
    };
}