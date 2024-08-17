#include <Scheduler.hpp>
#include <iostream>
#include <shared_mutex>

#include "Luau/Compiler.h"
#include "RobloxManager.hpp"
#include "Security.hpp"
#include "lstate.h"

std::shared_ptr<Scheduler> Scheduler::pInstance;

std::shared_ptr<Scheduler> Scheduler::GetSingleton() {
    if (Scheduler::pInstance == nullptr)
        Scheduler::pInstance = std::make_shared<Scheduler>();

    return Scheduler::pInstance;
}

void Scheduler::ScheduleJob(const std::string &source) {
    SchedulerJob job(source);
    this->m_qSchedulerJobs.emplace(job);
}

SchedulerJob Scheduler::DequeueSchedulerJob() {
    if (this->m_qSchedulerJobs.empty())
        return SchedulerJob("");
    auto job = this->m_qSchedulerJobs.back();
    this->m_qSchedulerJobs.pop();
    return job;
}

void Scheduler::ExecuteSchedulerJob(lua_State *runOn, std::unique_ptr<SchedulerJob> job) {
    if (job->bIsLuaCode) {
        if (job->luaJob.szluaCode.empty())
            return;
        const auto logger = Logger::GetSingleton();
        const auto robloxManager = RobloxManager::GetSingleton();
        const auto security = Security::GetSingleton();

        logger->PrintInformation(RbxStu::Scheduler, "Compiling Bytecode...");

        auto opts = Luau::CompileOptions{};
        opts.debugLevel = 1;
        opts.optimizationLevel = 2;
        auto bytecode = Luau::compile(job->luaJob.szluaCode, opts);

        logger->PrintInformation(RbxStu::Scheduler, "Compiled Bytecode!");

        // WARNING: This code will have to be run ONLY if you decide to use luaE_newthread, as lua_newthread will
        // execute the callback, which will trigger it and initialize the RobloxExtraSpace correctly.
        // runOn->global->cb.userthread(runOn, L);
        auto L = lua_newthread(runOn);
        lua_pop(runOn, 1);

        security->SetThreadSecurity(L);

        logger->PrintInformation(RbxStu::Scheduler, "Set Thread identity & capabilities");

        if (luau_load(L, "RbxStuV2", bytecode.c_str(), bytecode.size(), 0) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            logger->PrintError(RbxStu::Scheduler, err);
            lua_pop(L, 1);
            return;
        }
        logger->PrintWarning(RbxStu::Scheduler,
                             std::format("Execution Lua State = {:#x}", reinterpret_cast<std::uintptr_t>(L)));

        auto *pClosure = const_cast<Closure *>(static_cast<const Closure *>(lua_topointer(L, -1)));

        security->SetLuaClosureSecurity(pClosure);

        if (robloxManager->GetRobloxTaskDefer().has_value()) {
            const auto defer = robloxManager->GetRobloxTaskDefer().value();
            defer(L);
        } else if (robloxManager->GetRobloxTaskSpawn().has_value()) {
            const auto spawn = robloxManager->GetRobloxTaskSpawn().value();
            spawn(L);
        } else {
            logger->PrintError(RbxStu::Scheduler,
                               "Execution attempt failed. There is no function that can run the code through Roblox's "
                               "scheduler! Reason: task.defer and task.spawn were not found on the sigging step.");

            throw std::exception("Cannot run Scheduler job!");
        }
    }
};

std::shared_mutex __scheduler_init;

std::optional<lua_State *> Scheduler::GetGlobalExecutorState() const {
    std::lock_guard g{__scheduler_init};
    return this->m_lsInitialisedWith;
}

std::optional<lua_State *> Scheduler::GetGlobalRobloxState() const {
    std::lock_guard g{__scheduler_init};
    return this->m_lsRoblox;
}

void Scheduler::StepScheduler(lua_State *runner) {
    // Here we will check if the DataModel obtained is correct, as in, our data model is successful!
    const auto robloxManager = RobloxManager::GetSingleton();
    const auto logger = Logger::GetSingleton();
    const auto dataModel = robloxManager->GetCurrentDataModel(RBX::DataModelType_PlayClient);
    if (!dataModel.has_value() || this->m_pClientDataModel != dataModel.value()) {
        logger->PrintWarning(RbxStu::Scheduler, "The task scheduler's internal state is out of date! Reason: DataModel "
                                                "pointer is invalid! Executing reinitialization sub-routine!");
        this->ResetScheduler();
        return;
    }
    if (runner != this->m_lsInitialisedWith) {
        logger->PrintWarning(
                RbxStu::Scheduler,
                "The task scheduler's RbxStu state is different than the provided one to execute operations on. This "
                "may result in wrong behaviour inside of Luau and could potentially leak the elevated RbxStu "
                "environment into segments which are not supposed to have such elevated access!");
    }
    auto job = this->DequeueSchedulerJob();
    this->ExecuteSchedulerJob(runner, std::make_unique<SchedulerJob>(job));
}

void Scheduler::InitializeWith(lua_State *L, lua_State *rL, RBX::DataModel *dataModel) {
    std::lock_guard g{__scheduler_init};
    this->m_pClientDataModel = dataModel;
    this->m_lsRoblox = rL;
    this->m_lsInitialisedWith = L;

    Logger::GetSingleton()->PrintInformation(
            RbxStu::Scheduler, std::format("Task Scheduler initialized!\nInternal State: \n\t- m_pClientDataModel: "
                                           "{}\n\t- m_lsRoblox: {}\n\t- m_lsInitialisedWith: {}",
                                           reinterpret_cast<void *>(this->m_pClientDataModel.value()),
                                           reinterpret_cast<void *>(this->m_lsRoblox.value()),
                                           reinterpret_cast<void *>(this->m_lsInitialisedWith.value())));
}

void Scheduler::ResetScheduler() {
    std::lock_guard g{__scheduler_init};
    const auto logger = Logger::GetSingleton();
    this->m_lsRoblox = {};
    this->m_lsInitialisedWith = {};
    this->m_pClientDataModel = {};
    logger->PrintInformation(RbxStu::Scheduler, "Scheduler reset completed. All fields set to no value.");
}

bool Scheduler::IsInitialized() const {
    std::lock_guard g{__scheduler_init};
    return this->m_lsInitialisedWith.has_value() && this->m_lsRoblox.has_value();
}
