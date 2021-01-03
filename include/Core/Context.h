#pragma once

#include <Core/Config.h>
#include <Net/Tor/TorProcess.h>
#include <scheduler/Scheduler.h>
#include <Common/Logger.h>
#include <memory>
#include <cassert>

class Context
{
public:
    using Ptr = std::shared_ptr<Context>;

    Context(
        const Environment env,
        const ConfigPtr& pConfig,
        const std::shared_ptr<Bosma::Scheduler>& pScheduler,
        const TorProcess::Ptr& pTorProcess
    ) : m_env(env), m_pConfig(pConfig), m_pScheduler(pScheduler), m_pTorProcess(pTorProcess) { }

    ~Context() {
        LOG_INFO("Deleting node context");
        m_pScheduler.reset();
        m_pTorProcess.reset();
        m_pConfig.reset();
    }

    static Context::Ptr Create(const Environment env, const ConfigPtr& pConfig)
    {
        assert(pConfig != nullptr);

        auto pTorProcess = TorProcess::Initialize(
            pConfig->GetTorDataPath(),
            pConfig->GetSocksPort(),
            pConfig->GetControlPort()
        );
        return std::make_shared<Context>(
            env,
            pConfig,
            std::make_shared<Bosma::Scheduler>(12),
            pTorProcess
        );
    }

    Environment GetEnvironment() const noexcept { return m_env; }
    const Config& GetConfig() const noexcept { return *m_pConfig; }
    const std::shared_ptr<Bosma::Scheduler>& GetScheduler() const noexcept { return m_pScheduler; }
    const TorProcess::Ptr& GetTorProcess() const noexcept { return m_pTorProcess; }

private:
    // TODO: Include logger
    Environment m_env;
    ConfigPtr m_pConfig;
    std::shared_ptr<Bosma::Scheduler> m_pScheduler;
    TorProcess::Ptr m_pTorProcess;
};