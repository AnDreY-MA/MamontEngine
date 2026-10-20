#pragma once
#include "Event.h"
#include "InputEvent.h"

namespace MamontEngine
{
    struct EventHandle
    {
        uint64_t Value{};

        constexpr explicit EventHandle(uint64_t id = 0) : Value(id){}
        constexpr auto operator<=>(const EventHandle &) const = default;
    };

    template<typename... Args>
	class Event
	{
    public:
        using Delegate = std::function<void(Args...)>;

        EventHandle Add(Delegate inDelegate)
        {
            const EventHandle handle{m_NextHandle.Value++};
            m_Listeners[handle] = std::move(inDelegate);
            return handle;
        }

        template <typename ObjType, typename RetType>
        EventHandle Add(ObjType* obj, RetType (ObjType::*method)(Args...))
        {
            return Add([obj, method](Args... args) { 
                (obj->*method)(args...);
            });
        }

        void Remove(EventHandle inHandle)
        {
            m_Listeners.erase(inHandle);
        }

        void Invoke(Args... args)
        {
            for (const auto& [handle, delegate] : m_Listeners)
            {
                delegate(args...);
            }
        }

    private:
        std::unordered_map<EventHandle, Delegate> m_Listeners;
        EventHandle                               m_NextHandle{1};

	};
}

namespace std
{
    template <>
    struct hash<MamontEngine::EventHandle>
    {
        size_t operator()(const MamontEngine::EventHandle &handle) const noexcept
        {
            return std::hash<uint64_t>{}(handle.Value);
        }
    };
}