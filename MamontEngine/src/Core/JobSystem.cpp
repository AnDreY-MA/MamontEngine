#include "Core/JobSystem.h"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include "Utils/Profile.h"
#include "Core/Log.h"

namespace MamontEngine
{
    namespace JobSystem
    {
        struct Job
        {
            Context                             *Context{nullptr};
            std::function<void(JobDispatchArgs)> Task;
            uint32_t                             GroupID{0};
            uint32_t                             GroupOffset{0};
            uint32_t                             GroupEnd{0};
            uint32_t                             SharedMemorySize{0};
        };

        struct JobQueue
        {
        private:
            std::deque<Job> Queue;
            std::mutex      Locker;

        public:
            void PushBack(const Job &job)
            {
                std::scoped_lock<std::mutex> lock(Locker);
                Queue.push_back(job);
            }

            bool PopFront(Job &job)
            {
                std::scoped_lock<std::mutex> lock(Locker);
                if (Queue.empty())
                    return false;

                job = std::move(Queue.front());
                Queue.pop_front();
                return true;
            }
        };


        struct State
        {
            uint32_t Cores{0};
            uint32_t NumThreads{0};

            std::unique_ptr<JobQueue[]> jobQueueThread;

            std::atomic_bool        Alive{true};
            std::condition_variable WakeCondition;
            std::mutex              WakeMutex;
            std::atomic<uint32_t>   NextQueue{0};

            std::vector<std::thread> Threads;

            ~State()
            {
                Alive.store(false);

                bool isWakeLoop{true};

                std::thread waker(
                        [&]
                        {
                            while (isWakeLoop)
                            {
                                WakeCondition.notify_all();
                            }
                        });

                for (auto &thread : Threads)
                {
                    if (thread.joinable())
                    {
                        thread.join();
                    }
                }
                isWakeLoop = false;
                if (waker.joinable())
                {
                    waker.join();
                }
                jobQueueThread.reset();

                //WakeCondition.notify_all();

            }
        };
        static State *s_State = nullptr;

        static void Work(uint32_t inStartingQueue)
        {
            PROFILE_FUNCTION();

            Job job;
            for (uint32_t i{0}; i < s_State->NumThreads; ++i)
            {
                JobQueue &jobQueue = s_State->jobQueueThread[inStartingQueue % s_State->NumThreads];
                while (jobQueue.PopFront(job))
                {
                    JobDispatchArgs args{.GroupID = job.GroupID};

                    for (uint32_t j = job.GroupOffset; j < job.GroupEnd; ++j)
                    {
                        args.JobIndex       = j;
                        args.GroupIndex     = j - job.GroupOffset;
                        args.IsFirstInGroup = (j == job.GroupOffset);
                        args.IsLastInGroup  = (j == job.GroupOffset - 1);
                        job.Task(args);
                    }

                    job.Context->Counter.fetch_sub(1);
                }

                inStartingQueue++;
            }
        }

        void Init(uint32_t inMaxThreadCount)
        {
            PROFILE_FUNCTION();

            if (!s_State)
            {
                s_State = new State();
            }

            s_State->Cores      = std::thread::hardware_concurrency();
            s_State->NumThreads = std::max(1u, s_State->Cores - inMaxThreadCount);

            s_State->jobQueueThread.reset(new JobQueue[s_State->NumThreads]);

            s_State->Threads.reserve(s_State->NumThreads);

            for (uint32_t threadID{0}; threadID < s_State->NumThreads; ++threadID)
            {
                std::thread &worker = s_State->Threads.emplace_back(
                        [threadID]
                        {
                            const std::string name = std::format("JobSystem_{}", threadID);
                            PROFILE_SETTHREADNAME(name.c_str());

                            if (s_State->Alive.load())
                            {
                                Work(threadID);

                                std::unique_lock<std::mutex> lock(s_State->WakeMutex);
                                s_State->WakeCondition.wait(lock);
                            }
                        });

                worker.detach();
            }

            Log::Info("Initialize JobSystem with {} cores, {} threads", s_State->Cores, s_State->NumThreads);
        }

        void Release()
        {
            delete s_State;
            s_State = nullptr;
        }

        void Execute(Context &inContext, const std::function<void(JobDispatchArgs)> &&inTask)
        {
            PROFILE_FUNCTION();

            inContext.Counter.fetch_add(1);

            const Job job = {.Context = &inContext, .Task = inTask, .GroupID = 0, .GroupOffset = 0, .GroupEnd = 1, .SharedMemorySize = 0};

            s_State->jobQueueThread[s_State->NextQueue.fetch_add(1) % s_State->NumThreads].PushBack(job);
            s_State->WakeCondition.notify_one();
        }

        void Dispatch(Context &ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>&& task, size_t shareMemorySize)
        {
            PROFILE_FUNCTION();

            if (jobCount == 0 || groupSize == 0)
                return;

            const uint32_t groupCount = DispatchGroupCount(jobCount, groupSize);

            ctx.Counter.fetch_add(groupCount);

            Job job = {.Context = &ctx, .Task = task, .SharedMemorySize = static_cast<uint32_t>(shareMemorySize)};

            for (uint32_t groupID{0}; groupID < groupCount; ++groupID)
            {
                job.GroupID     = groupID;
                job.GroupOffset = groupID * groupSize;
                job.GroupEnd    = std::min(job.GroupOffset + groupSize, jobCount);

                s_State->jobQueueThread[s_State->NextQueue.fetch_add(1) % s_State->NumThreads].PushBack(job);
            }

            s_State->WakeCondition.notify_one();
        }


        uint32_t DispatchGroupCount(uint32_t jobCount, uint32_t groupSize)
        {
            return (jobCount + groupSize - 1) / groupSize;
        }

        bool IsBusy(const Context &inContext)
        {
            return inContext.Counter.load() > 0;
        }

        void Wait(const Context &inContext)
        {
            PROFILE_FUNCTION();

            if (IsBusy(inContext))
            {
                s_State->WakeCondition.notify_all();

                Work(s_State->NextQueue.fetch_add(1) % s_State->NumThreads);

                while (IsBusy(inContext))
                {
                    std::this_thread::yield();
                }
            }
        }

    } // namespace JobSystem
} // namespace MamontEngine
