// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

StringLiteral const TraceType("MonitoredConfigSettingsConfigStore");

// ********************************************************************************************************************

const int MonitoredConfigSettingsConfigStore::MaxRetryIntervalInMillis = 5000;
const int MonitoredConfigSettingsConfigStore::MaxRetryCount = 10;

// ********************************************************************************************************************
// MonitoredConfigSettingsConfigStore::ChangeMonitor Implementation
//
class MonitoredConfigSettingsConfigStore::ChangeMonitor : 
    public FabricComponent,
    public std::enable_shared_from_this<ChangeMonitor>
{
    DENY_COPY(ChangeMonitor);

public:

    ChangeMonitor::ChangeMonitor(ConfigStoreSPtr const & owner, wstring const & storeLocation)
        : weak_owner_(owner),
        fileMonitor_(FileChangeMonitor2::Create(storeLocation))
    {
    }

    virtual ChangeMonitor::~ChangeMonitor()
    {
    }

protected:
    ErrorCode OnOpen()
    {
        fileMonitor_->RegisterFailedEventHandler([this](ErrorCode const & error) { this->OnMonitorFailure(error); });
        // Passing a shared pointer to lamda to ensure that a reference to the current object is hold as long as the lambda is active.
        // This will prevent the parent from being freed before the lambda. 
        auto thisSPtr = shared_from_this();
        auto error = fileMonitor_->Open([thisSPtr](FileChangeMonitor2SPtr const & monitor) { thisSPtr->OnChange(monitor); } );
        if (!error.IsSuccess())
        {
            fileMonitor_->Abort();
        }

        return error;
    }

    ErrorCode OnClose()
    {
        fileMonitor_->Close();
        return ErrorCode(ErrorCodeValue::Success);
    }

    void OnAbort()
    {
        fileMonitor_->Abort();
    }

private:
    void OnChange(FileChangeMonitor2SPtr const &)
    {
        auto owner = weak_owner_.lock();
        if (owner)
        {
            auto monitoredConfigStore = static_cast<MonitoredConfigSettingsConfigStore*>(owner.get());
            monitoredConfigStore->OnFileChanged();
        }
    }

    void OnMonitorFailure(ErrorCode const & error)
    {
        fileMonitor_->Abort();

        auto msg(wformatString("FileChangeMonitor failed with {0}", error));
        TraceError(
            TraceTaskCodes::Common,
            TraceType,
            "{0}",
            msg);
        Assert::TestAssert("{0}", msg);
    }

private:
    ConfigStoreWPtr const weak_owner_;
    FileChangeMonitor2SPtr const fileMonitor_;
};

// ********************************************************************************************************************
// MonitoredConfigSettingsConfigStore Implementation
//
MonitoredConfigSettingsConfigStore::MonitoredConfigSettingsConfigStore(
     wstring const & fileToMonitor, 
     ConfigSettingsConfigStore::Preprocessor const & preprocessor)
     : fileName_(fileToMonitor),
     preprocessor_(preprocessor),
     ConfigSettingsConfigStore(ConfigSettings())
{
}

MonitoredConfigSettingsConfigStore::~MonitoredConfigSettingsConfigStore()
{
    this->DisableChangeMonitoring().ReadValue();
}

void MonitoredConfigSettingsConfigStore::Initialize()
{
    ConfigSettings settings;
    auto error = LoadConfigSettings_WithRetry(settings);
    if (error.IsSuccess())
    {
        ConfigSettingsConfigStore::Update(settings);
    }
    else
    {
        auto msg(wformatString("Loading of configuration settings from {0} failed with {1}.",
            fileName_,
            error));
        TraceError(
            TraceTaskCodes::Common,
            TraceType,
            "{0}",
            msg);
        Assert::TestAssert("{0}", msg);
    }
}

ErrorCode MonitoredConfigSettingsConfigStore::LoadConfigSettings_WithRetry(__out ConfigSettings & settings)
{
    Random rand;
    ErrorCode lastError = ErrorCode(ErrorCodeValue::OperationFailed);

    for(int i = 0; i < MonitoredConfigSettingsConfigStore::MaxRetryCount; i++)
    {
        auto error = this->LoadConfigSettings(settings);
        if (error.IsSuccess())
        {
            if (preprocessor_)
            {
                preprocessor_(settings);
            }

            return error;
        }
        else
        {
            lastError.Overwrite(error);
            ::Sleep(rand.Next(MonitoredConfigSettingsConfigStore::MaxRetryIntervalInMillis));
        }
    }

    return lastError;
}

void MonitoredConfigSettingsConfigStore::OnFileChanged()
{
    ConfigSettings settings;
    auto error = this->LoadConfigSettings_WithRetry(settings);
    if (error.IsSuccess())
    {
        ConfigSettingsConfigStore::Update(settings);
    }
    else
    {
        auto msg(wformatString("Loading of configuration settings from {0} failed with {1}.",
            fileName_,
            error));
        TraceError(
            TraceTaskCodes::Common,
            TraceType,
            "{0}",
            msg);
        Assert::TestAssert("{0}", msg);
    }
}

ErrorCode MonitoredConfigSettingsConfigStore::EnableChangeMonitoring()
{
    if (this->changeMonitor_)
    {
        this->changeMonitor_->Close().ReadValue();
    }

    this->changeMonitor_ = make_shared<ChangeMonitor>(this->shared_from_this(), this->fileName_);
    return this->changeMonitor_->Open();
}

ErrorCode MonitoredConfigSettingsConfigStore::DisableChangeMonitoring()
{
    if (this->changeMonitor_)
    {
        auto error = this->changeMonitor_->Close();
        this->changeMonitor_ = nullptr;
        return error;
    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}
