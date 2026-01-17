#pragma once

#include <cstdint>
namespace MamontEngine
{
    namespace JobSystem
    {
        struct JobDispatchArgs
        {
            uint32_t JobIndex{0};
            uint32_t GroupID{0};
            uint32_t GroupIndex{0};
            bool     IsFirstInGroup{false};
            bool     IsLastInGroup{false};
            void    *ShareMemory{nullptr};
        };

        void Init(uint32_t inMaxThreadCount = 1);
        void Release();

        struct Context
        {
            std::atomic<uint32_t> Counter{0};
        };

        void Execute(Context &inContext, const std::function<void(JobDispatchArgs)> &&inTask);

        void Dispatch(Context &ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)> &&task, size_t shareMemorySize = 0);

        uint32_t DispatchGroupCount(uint32_t jobCount, uint32_t groupSize);

        bool IsBusy(const Context &inContext);

        void Wait(const Context &inContext);
    } // namespace JobSystem
} // namespace MamontEngine
