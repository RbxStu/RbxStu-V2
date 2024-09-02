//
// Created by Dottik on 16/8/2024.
//
#pragma once
#include <Windows.h>
#include <future>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include "Communication/Communication.hpp"
#include "Logger.hpp"
#include "Utilities.hpp"
#include "lstate.h"
#include "lua.h"

class SchedulerJob {
public:
    struct lJob {
        std::string szOperationIdentifier;
        std::string szluaCode;
    } luaJob;
    struct yJob {
        RBX::Lua::WeakThreadRef threadRef;
    } yieldJob;

    bool bIsLuaCode;
    bool bIsYieldingJob;

    SchedulerJob(const SchedulerJob &job) {
        if (job.bIsLuaCode) {
            this->bIsLuaCode = true;
            this->luaJob = {};
            this->luaJob.szluaCode = job.luaJob.szluaCode;
        } else if (job.bIsYieldingJob) {
            this->bIsLuaCode = false;
            this->bIsYieldingJob = true;
            this->callbackFuture = job.callbackFuture;
            std::memcpy(&this->yieldJob.threadRef, &job.yieldJob.threadRef, sizeof(RBX::Lua::WeakThreadRef));
        }
    };

    explicit SchedulerJob(const std::string &luaCode) {
        this->bIsLuaCode = true;
        this->bIsYieldingJob = false;
        this->luaJob = {};
        this->luaJob.szluaCode = luaCode;
        this->luaJob.szOperationIdentifier = "Anonymous";
    }

    explicit SchedulerJob(const std::string &luaCode, const std::string &jobIdentifier) {
        this->bIsLuaCode = true;
        this->bIsYieldingJob = false;
        this->luaJob = {};
        this->luaJob.szluaCode = luaCode;
        this->luaJob.szOperationIdentifier = jobIdentifier;
    }

    /// @brief The callback that is executed once the yield is completed to push the results into the lua stack for
    /// resumption.
    std::shared_future<std::function<int(lua_State *)>> *callbackFuture = nullptr;

    static std::mutex __scheduler__job__access__mutex;
    explicit SchedulerJob(
            lua_State *L,
            std::function<void(lua_State *L, std::shared_future<std::function<int(lua_State *)>> *callbackToExecute)>
                    function) {
        const auto logger = Logger::GetSingleton();
        this->bIsLuaCode = false;
        this->bIsYieldingJob = true;
        this->callbackFuture = new std::shared_future<std::function<int(lua_State *)>>();
        this->yieldJob.threadRef.thread = L;
        lua_pushthread(L);
        this->yieldJob.threadRef.thread_ref = lua_ref(L, -1);
        lua_pop(L, 1); // Reset stack
        logger->PrintInformation(RbxStu::Scheduler, "Launching callback for later resumption");
        std::thread(function, L, this->callbackFuture).detach();
    }

    ~SchedulerJob() = default;

    bool IsJobCompleted() const {
        std::scoped_lock lg{__scheduler__job__access__mutex};
        if (this->bIsLuaCode) {
            return true;
        }
        if (this->bIsYieldingJob && this->callbackFuture) {
            const auto logger = Logger::GetSingleton();
            if (!this->callbackFuture->valid()) {
                return false;
            }

            const auto ret = this->callbackFuture->wait_for(std::chrono::milliseconds(10));
            return (ret == std::future_status::ready || ret == std::future_status::deferred);
        }

        return false;
    }

    std::optional<std::function<int(lua_State *)>> GetCallback() const {
        std::scoped_lock lg{__scheduler__job__access__mutex};
        if (this->bIsLuaCode)
            return {};

        if (this->callbackFuture && this->callbackFuture->valid()) {
            const auto ret = this->callbackFuture->wait_for(
                    std::chrono::milliseconds(10)); // Lazily compute value requires us to do this lmao.

            if (this->bIsYieldingJob && ret == std::future_status::ready || ret == std::future_status::deferred)
                return this->callbackFuture->get();
        }

        return {};
    }

    void FreeResources() {
        std::scoped_lock lg{__scheduler__job__access__mutex};
        delete this->callbackFuture;
        this->callbackFuture = nullptr;
    }
};

class Scheduler final {
    /// @brief Private, Static shared pointer into the instance.
    static std::shared_ptr<Scheduler> pInstance;

    /// @brief A std::optional<lua_State *>, which represents a unique, non-array lua_State which RbxStu obtains its
    /// environment from.
    std::optional<lua_State *> m_lsInitialisedWith;
    /// @brief A std::optional<lua_State *>, which represents a unique, non-array lua_State which results from the
    /// ScriptContext's GetGlobalState.
    std::optional<lua_State *> m_lsRoblox;
    /// @brief A queue of jobs for the Scheduler to work through when stepping. May include more than one job.
    std::queue<SchedulerJob> m_qSchedulerJobs;
    /// @brief A std::optional<RBX::DataModel *>, which represents a unique, non-array RBX::DataModel obtained through
    /// hooking, which the ScriptContext that m_lsRoblox was obtained from is parented/related to.
    std::optional<RBX::DataModel *> m_pClientDataModel;

    /// @brief Internal function used to dequeue a job from the job queue.
    SchedulerJob GetSchedulerJob(bool pop);

public:
    /// @brief Obtains the shared pointer that points to the global singleton for the current class.
    /// @return Singleton for Scheduler as a std::shared_ptr<Scheduler>.
    static std::shared_ptr<Scheduler> GetSingleton();

    /// @brief Executes a scheduler job on demand.
    /// @param runOn The lua state to execute the scheduler job into
    /// @param job The scheduler job to execute on
    /// @remarks This is an exposed internal function. Calling it may result in undefined behaviour.
    bool ExecuteSchedulerJob(lua_State *runOn, SchedulerJob *job);

    /// @brief Schedules a job into the Scheduler given its Luau source code.
    /// @param job An instance of a job to enqueue on the scheduler for execution.
    void ScheduleJob(SchedulerJob job);

    /// @brief Initializes the Scheduler with the given RbxStu lua_State, global Roblox lua_State and RBX::DataModel
    /// pointer.
    /// @param L The RbxStu lua_State.
    /// @param rL The global Roblox lua_State
    /// @param dataModel A pointer to the RBX::DataModel that rL's ScriptContext originates from.
    /// @remarks This function may result in undefined behaviour if any of the pointers is nullptr.
    void InitializeWith(lua_State *L, lua_State *rL, RBX::DataModel *dataModel);

    /// @brief Obtains whether the instance is initialized to completion.
    /// @return True if the scheduler has its L, rL and DataModel with populated, valid values.
    [[nodiscard]] bool IsInitialized() const;

    /// @brief Used to reset the scheduler if it is deemed unfit for execution.
    /// @remarks Reserved for internal API use. Do not call unless explicitly required.
    void ResetScheduler();

    /// @brief Used to obtain the RbxStu lua_State the Scheduler was initialized with.
    /// @return A std::optional<lua_State *> which may or may not have a value.
    std::optional<lua_State *> GetGlobalExecutorState() const;

    /// @brief Used to obtain the global Roblox lua_State the Scheduler was initialized with.
    /// @return A std::optional<lua_State *> which may or may not have a value.
    std::optional<lua_State *> GetGlobalRobloxState() const;

    /// @brief Used to step the scheduler.
    /// @remarks DO NOT CALL INSIDE ANY CODE THAT IS NOT SYNCHRONIZED WITH THE ROBLOX'S TASK SCHEDULER!!! THIS WILL
    /// RESULT IN UNDEFINED BEHAVIOUR!
    void StepScheduler(lua_State *runner);

    void SetExecutionDataModel(RBX::DataModelType dataModel);

    RBX::DataModelType GetExecutionDataModel();
};
