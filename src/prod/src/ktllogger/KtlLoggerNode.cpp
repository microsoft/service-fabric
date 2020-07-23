// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace KtlLogger;
using namespace TxnReplicator;

#if defined(UDRIVER) || defined(UPASSTHROUGH)
#include "KtlLogShimKernel.h"
#endif

class KtlLoggerNode::InitializeKtlLoggerAsyncOperation : public KtlProxyAsyncOperation
{
    public:
    InitializeKtlLoggerAsyncOperation(
        KtlLoggerInitializationContext& asyncContext,
        BOOLEAN useInprocLogger,
        KtlLogManager::MemoryThrottleLimits const & memoryLimit,
        KtlLogManager::AcceleratedFlushLimits const & accelerateFlushLimit,
        KtlLogManager::SharedLogContainerSettings const & sharedLogSettings,
        std::wstring const & fabricDataRoot,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : KtlProxyAsyncOperation(&asyncContext, nullptr, callback, parent)
        , useInprocLogger_(useInprocLogger)
        , memoryLimits_(memoryLimit)
        , accelerateFlushLimits_(accelerateFlushLimit)
        , sharedLogSettings_(sharedLogSettings)
        , fabricDataRoot_(fabricDataRoot)
        , asyncContext_(&asyncContext)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisSPtr = AsyncOperation::End<InitializeKtlLoggerAsyncOperation>(operation);

        return thisSPtr->Error;
    }

    private:
    NTSTATUS StartKtlAsyncOperation(
        __in KAsyncContextBase * const parentAsyncContext,
        __in KAsyncContextBase::CompletionCallback callback)
    {
        //
        // Call into the KTL async
        //
        asyncContext_->StartInitializeKtlLogger(
            useInprocLogger_,                                               
            memoryLimits_,
            accelerateFlushLimits_,
            sharedLogSettings_,
            fabricDataRoot_.c_str(),
            parentAsyncContext,
            callback);

        return STATUS_SUCCESS;
    }

private:   
    KtlLoggerInitializationContext::SPtr asyncContext_;
    BOOLEAN useInprocLogger_;
    KtlLogManager::MemoryThrottleLimits memoryLimits_;  
    KtlLogManager::AcceleratedFlushLimits accelerateFlushLimits_;   
    KtlLogManager::SharedLogContainerSettings sharedLogSettings_;
    std::wstring fabricDataRoot_;
};

//
// KtlLoggerNode::OpenAsyncOperation
//
void KtlLoggerNode::OpenAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    NTSTATUS status;
    KtlLoggerInitializationContext::SPtr ktlLoggerInitContext;
    BOOL b;
    
#if !defined(PLATFORM_UNIX)          // TODO: Determine size of memory
    MEMORYSTATUSEX memoryStatusEx;

    memoryStatusEx.dwLength = sizeof(memoryStatusEx);
    b = GlobalMemoryStatusEx(&memoryStatusEx);
    if (!b)
    {
        Common::ErrorCode error = Common::ErrorCode::FromWin32Error(GetLastError());
        WriteError("KtlLoggerNode", "Line: {0}, Error {1}", __LINE__, error);
        TryComplete(thisSPtr, error);
        return;
    }
#endif

    if (!owner_.useGlobalKtlSystem_)
    {
        owner_.ktlSystem_->Activate(static_cast<ULONG>(timeoutHelper_.GetRemainingTime().TotalPositiveMilliseconds()));
        status = owner_.ktlSystem_->Status();
        if (! NT_SUCCESS(status))
        {
            WriteError("KtlLoggerNode", "Line: {0}, Status {1}", __LINE__, status);
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
            return;
        }

        // NOTE: This is needed for high performance of v2 replicator stack as more threads 
        // are used to perform completions of awaitables
        owner_.ktlSystem_->SetDefaultSystemThreadPoolUsage(FALSE);
    }

#if defined(UDRIVER)
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(owner_.GetAllocator(), KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        Common::ErrorCode error = Common::ErrorCode::FromNtStatus(status);

        TryComplete(thisSPtr, error);
        return;
    }

    owner_.isRegistered_ = true;
#else
    // For kernel, we assume driver already installed by InstallForCITs
#endif  

    status = KtlLoggerInitializationContext::Create(
        owner_.GetAllocator(),
        KTL_TAG_TEST,
        &owner_,
        ktlLoggerInitContext);

    if (!NT_SUCCESS(status))
    {
        Common::ErrorCode error = Common::ErrorCode::FromNtStatus(status);

        WriteError("KtlLoggerNode", "Line: {0}, Status {1}", __LINE__, status);
        TryComplete(thisSPtr, error);
        return;
    }

    //
    // Configure SharedLogSettings
    //
    BOOLEAN automaticMemoryConfiguration;
    KtlLoggerConfig& config = KtlLoggerConfig::GetConfig();
    ServiceModel::ServiceModelConfig& smConfig = ServiceModel::ServiceModelConfig::GetConfig();
    KtlLogManager::MemoryThrottleLimits memoryLimits;
    KtlLogManager::AcceleratedFlushLimits accelerateFlushLimits;

    static const LONGLONG oneKB = 1024;
    static const LONG oneKBL = 1024;
    LONGLONG value;
    LONG valueL;

    applicationSharedLogSettings_ = make_unique<KtlLogManager::SharedLogContainerSettings>();
    systemServicesSharedLogSettings_ = make_unique<KtlLogManager::SharedLogContainerSettings>();

    const LONGLONG oneMB = 1024 * 1024;
    ULONG valueU;
    bool sharedLogIdOk = true;
    Guid g;

    std::wstring sharedLogPath;
    std::wstring sharedLogId;
    LONGLONG sharedLogSizeInMB;

    //
    // NodeType settings take precidence over global settings
    //
    sharedLogPath = fabricNodeConfig_->SharedLogFilePath;
    if (sharedLogPath == L"")
    {
        sharedLogPath = smConfig.SharedLogPath;
    }

    sharedLogId = fabricNodeConfig_->SharedLogFileId;
    if (sharedLogId == L"")
    {
        sharedLogId = smConfig.SharedLogId;
    }

    sharedLogSizeInMB = fabricNodeConfig_->SharedLogFileSizeInMB;
    if (sharedLogSizeInMB == 0)
    {
        sharedLogSizeInMB = smConfig.SharedLogSizeInMB;
    }

    if (sharedLogId != L"")
    {
        HRESULT hr;
        LPCWSTR wstr = sharedLogId.c_str();
        size_t len;
        hr = StringCchLength(wstr, MAX_PATH, &len);
        if (SUCCEEDED(hr))
        {
            //
            // get rid of { and } if present
            //
            if (wstr[len - 1] == L'}')
            {
                sharedLogId.erase(len - 1, 1);
            }

            if (wstr[0] == L'{')
            {
                sharedLogId.erase(0, 1);
            }

            sharedLogIdOk = Guid::TryParse(sharedLogId, g);
        }
        else {
            // todo?: debug assert
            sharedLogIdOk = false;
        }

        if (!sharedLogIdOk)
        {
            // todo?: debug assert
            sharedLogId = Constants::NullGuidString;
        }
    }
    else {
        sharedLogId = Constants::NullGuidString;
    }
        
    Guid::TryParse(sharedLogId, g);

    if (!sharedLogIdOk)
    {
        sharedLogPath = L"";
    }

    if (sharedLogId == L"" || sharedLogId == Constants::NullGuidString)
    {
        g = Constants::DefaultApplicationSharedLogId;
    }

    if (sharedLogPath == L"")
    {
#if !defined(PLATFORM_UNIX) 
        Path::CombineInPlace(sharedLogPath, fabricDataRoot_);
#else
        // In Linux, applications will use a logger daemon, so it's okay for
        // multiple application hosts on a node to share the same shared log.
        //
        Path::CombineInPlace(sharedLogPath, fabricNodeConfig_->WorkingDir);
#endif
        Path::CombineInPlace(sharedLogPath, Constants::DefaultApplicationSharedLogSubdirectory);
        Path::CombineInPlace(sharedLogPath, Constants::DefaultApplicationSharedLogName);
    }

#if !defined(PLATFORM_UNIX) 
    if (sharedLogPath.find(KtlLoggerSharedLogSettings::WindowsPathPrefix) != 0)
    {
        sharedLogPath = wformatString("{0}{1}", *KtlLoggerSharedLogSettings::WindowsPathPrefix, sharedLogPath);
    }
#endif

    StringCchCopyW(applicationSharedLogSettings_->Path, (sizeof(applicationSharedLogSettings_->Path) / 2), sharedLogPath.c_str());

    ::GUID const& gg = g.AsGUID();
    KGuid kg = gg;
    applicationSharedLogSettings_->LogContainerId = kg;

    value = sharedLogSizeInMB;
    applicationSharedLogSettings_->LogSize = value * oneMB;

    valueL = smConfig.SharedLogNumberStreams;
    applicationSharedLogSettings_->MaximumNumberStreams = valueL;

    valueL = smConfig.SharedLogMaximumRecordSizeInKB;
    applicationSharedLogSettings_->MaximumRecordSize = (ULONG)(valueL * oneKBL);

    valueU = smConfig.SharedLogCreateFlags;
    applicationSharedLogSettings_->Flags = valueU;

    static const KGuid guidNull(0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    applicationSharedLogSettings_->DiskId = guidNull;

    {
        auto systemServicesSharedLogPath = KtlLoggerConfig::GetConfig().SystemSharedLogPath;

        if (systemServicesSharedLogPath.empty())
        {
#if !defined(PLATFORM_UNIX) 
            Path::CombineInPlace(systemServicesSharedLogPath, fabricDataRoot_);
#else
            // In Linux, system services will use an in-proc logger that has the restriction
            // that each process must have it's own shared log, so the shared log must be located
            // under the node working directory. More specifically, one in-proc logger per KTL
            // system.
            //
            // In Windows, there is a single global logger at the kernel level.
            //
            Path::CombineInPlace(systemServicesSharedLogPath, fabricNodeConfig_->WorkingDir);
#endif
            Path::CombineInPlace(systemServicesSharedLogPath, Constants::DefaultSystemServicesSharedLogSubdirectory);
            Path::CombineInPlace(systemServicesSharedLogPath, Constants::DefaultSystemServicesSharedLogName);

#if !defined(PLATFORM_UNIX) 
            if (systemServicesSharedLogPath.find(KtlLoggerSharedLogSettings::WindowsPathPrefix) != 0)
            {
                systemServicesSharedLogPath = wformatString("{0}{1}", KtlLoggerSharedLogSettings::WindowsPathPrefix, systemServicesSharedLogPath);
            }
#endif

            StringCchCopyW(systemServicesSharedLogSettings_->Path, (sizeof(systemServicesSharedLogSettings_->Path) / 2), systemServicesSharedLogPath.c_str());
        }

        // pick a default log container id
        if (KtlLoggerConfig::GetConfig().EnableHashSystemSharedLogIdFromPath)
        {
            auto guid = Guid::Test_FromStringHashCode(systemServicesSharedLogSettings_->Path);
            systemServicesSharedLogSettings_->LogContainerId = guid.ToKGuid();
        }
        else
        {
            KGuid ssKg = Constants::DefaultSystemServicesSharedLogId.AsGUID();
            systemServicesSharedLogSettings_->LogContainerId = ssKg;
        }

        // Grab all the various settings from existing configuration/defaults.
        systemServicesSharedLogSettings_->LogSize = KtlLoggerConfig::GetConfig().SystemSharedLogSizeInMB * oneMB;
        systemServicesSharedLogSettings_->MaximumNumberStreams = KtlLoggerConfig::GetConfig().SystemSharedLogNumberStreams;
        systemServicesSharedLogSettings_->MaximumRecordSize = KtlLoggerConfig::GetConfig().SystemSharedLogMaximumRecordSizeInKB * oneKB;
        systemServicesSharedLogSettings_->Flags = KtlLoggerConfig::GetConfig().SystemSharedLogCreateFlags;
        systemServicesSharedLogSettings_->DiskId = applicationSharedLogSettings_->DiskId;
    }

    //
    // Configure memory limits
    //
    value = config.AutomaticMemoryConfiguration;
    if (value == 0)
    {
        automaticMemoryConfiguration = FALSE;
    }
    else
    {
        automaticMemoryConfiguration = TRUE;
    }

#if !defined(PLATFORM_UNIX)
    if (automaticMemoryConfiguration)
    {
        LONGLONG memorySizeInBytes = memoryStatusEx.ullTotalPhys;

        memoryLimits.WriteBufferMemoryPoolMin = memorySizeInBytes / 8;
        if (memoryLimits.WriteBufferMemoryPoolMin < KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMinMin)
        {
            memoryLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMinMin;
        }

        memoryLimits.WriteBufferMemoryPoolMax = memorySizeInBytes / 6;
        if (memoryLimits.WriteBufferMemoryPoolMax < memoryLimits.WriteBufferMemoryPoolMin)
        {
            memoryLimits.WriteBufferMemoryPoolMax = memoryLimits.WriteBufferMemoryPoolMin;
        }       

        memoryLimits.WriteBufferMemoryPoolPerStream = oneKBL * oneKBL;

        //
        // Do not allow the WriteBufferMemoryPool to grow to a point
        // where it is larger than the shared log space. In this case
        // the shared log may become full before throttling on the
        // incoming writes occurs; shared log full recovery creates a
        // longer latency than throttling. Note that the usable shared
        // log size is discounted by an eighth to account for padding.
        //
        LONGLONG usablesharedlogsizeinbytes = (sharedLogSizeInMB * oneMB) - (sharedLogSizeInMB * (128 * oneKB));

        if (memoryLimits.WriteBufferMemoryPoolMin > usablesharedlogsizeinbytes)
        {
            memoryLimits.WriteBufferMemoryPoolMin = usablesharedlogsizeinbytes;
        }

        if (memoryLimits.WriteBufferMemoryPoolMax > usablesharedlogsizeinbytes)
        {
            memoryLimits.WriteBufferMemoryPoolMax = usablesharedlogsizeinbytes;
        }
        
        //
        // Allow unlimited pinned memory per node as the kernel will
        // push back when too much memory is in use
        //
        memoryLimits.PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    }
    else
#endif
    {
        value = config.WriteBufferMemoryPoolMaximumInKB;
        if (value == 0)
        {
            memoryLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        }
        else
        {
            memoryLimits.WriteBufferMemoryPoolMax = value * oneKB;
        }

        value = config.WriteBufferMemoryPoolMinimumInKB;
        if (value == 0)
        {
            memoryLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        }
        else
        {
            memoryLimits.WriteBufferMemoryPoolMin = value * oneKB;
        }

        value = config.PinnedMemoryLimitInKB;
        if (value == 0)
        {
            memoryLimits.PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        }
        else
        {
            memoryLimits.PinnedMemoryLimit = value * oneKB;
        }

        valueL = config.WriteBufferMemoryPoolPerStreamInKB;
        if (valueL == -1)
        {
            memoryLimits.WriteBufferMemoryPoolPerStream = oneKBL * oneKBL;
        }
        else
        {
            memoryLimits.WriteBufferMemoryPoolPerStream = valueL * oneKBL;
        }
    }

    value = config.MaximumDestagingWriteOutstandingInKB;
    if (value == 0)
    {
        memoryLimits.MaximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_DefaultMaximumDestagingWriteOutstanding;
    }
    else
    {
        memoryLimits.MaximumDestagingWriteOutstanding = value * oneKB;
    }

    int64 valueI64;

    valueI64 = config.PeriodicFlushTime.TotalSeconds();
    memoryLimits.PeriodicFlushTimeInSec = (ULONG)valueI64;

    valueI64 = config.PeriodicTimerInterval.TotalSeconds();
    memoryLimits.PeriodicTimerIntervalInSec = (ULONG)valueI64;

    valueI64 = config.AllocationTimeout.TotalMilliseconds();
    memoryLimits.AllocationTimeoutInMs = (ULONG)valueI64;

    value = config.SharedLogThrottleLimitInPercentUsed;
    if (value == 0)
    {
        memoryLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;
    }
    else if (value == 100) {
        memoryLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;
    }
    else if ((value < 0) || (value > 100))
    {
        memoryLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;
    }
    else 
    {
        memoryLimits.SharedLogThrottleLimit = (ULONG)value;
    }


    value = config.AccelerateFlushActiveTimerInMs;
    if ((value != KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActiveTimerInMsNoAction) &&
        ((value < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActiveTimerInMsMin) ||
         (value > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActiveTimerInMsMax)))
    {
        accelerateFlushLimits.AccelerateFlushActiveTimerInMs = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushActiveTimerInMs;
    } else {
        accelerateFlushLimits.AccelerateFlushActiveTimerInMs = (ULONG)value;
    }
    
    value = config.AccelerateFlushPassiveTimerInMs;
    if ((value < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassiveTimerInMsMin) ||
        (value > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassiveTimerInMsMax))
    {
        accelerateFlushLimits.AccelerateFlushPassiveTimerInMs = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushPassiveTimerInMs;
    } else {
        accelerateFlushLimits.AccelerateFlushPassiveTimerInMs = (ULONG)value;
    }
    
    value = config.AccelerateFlushActivePercent;
    if ((value < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActivePercentMin) ||
        (value > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActivePercentMax))
    {
        accelerateFlushLimits.AccelerateFlushActivePercent = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushActivePercent;
    } else {
        accelerateFlushLimits.AccelerateFlushActivePercent = (ULONG)value;
    }
    
    value = config.AccelerateFlushPassivePercent;
    if ((value < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassivePercentMin) ||
        (value > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassivePercentMax))
    {
        accelerateFlushLimits.AccelerateFlushPassivePercent = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushPassivePercent;
    } else {
        accelerateFlushLimits.AccelerateFlushPassivePercent = (ULONG)value;
    }   
    
    auto operation = shared_ptr<InitializeKtlLoggerAsyncOperation>(
        new(owner_.GetAllocator()) InitializeKtlLoggerAsyncOperation(
            *ktlLoggerInitContext,
#if defined(PLATFORM_UNIX)
            TRUE,
#else
            FALSE,
#endif
            memoryLimits,
            accelerateFlushLimits,
            *applicationSharedLogSettings_,
            fabricDataRoot_,
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnInitializeKtlLoggerCompleted(operation);
            },
            thisSPtr));

    operation->Start(operation);

    if (operation->CompletedSynchronously)
    {
        FinishInitializeKtlLogger(operation);
    }
}
void KtlLoggerNode::OpenAsyncOperation::OnInitializeKtlLoggerCompleted(AsyncOperationSPtr const& operation)
{
    if (! operation->CompletedSynchronously)
    {
        FinishInitializeKtlLogger(operation);
    }   
}

void KtlLoggerNode::OpenAsyncOperation::FinishInitializeKtlLogger(AsyncOperationSPtr const& operation)
{
    ErrorCode error = operation->End(operation);

    if (! error.IsSuccess())
    {
        WriteError("KtlLoggerNode", "Line: {0}, Error {1}", __LINE__, error);
    }
    TryComplete(operation->Parent, error);
}

//
// KtlLoggerNode::CloseAsyncOperation
//

void KtlLoggerNode::CloseAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    // Since KTL deactivation is being posted to unblock FabricNode close, the KTL
    // deactivation timeout doesn't need to be derived from the close operation timeout.
    // It can be whatever value we deem appropriate for debugging KTL leaks.
    //
    owner_.PostThreadDeactivateKtlLoggerAndSystem(KtlLoggerConfig::GetConfig().KtlDeactivationTimeout);

    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
}

void KtlLoggerNode::PostThreadDeactivateKtlLoggerAndSystem(TimeSpan const timeout)
{
    auto selfRoot = shared_from_this();
    Common::Threadpool::Post([this, selfRoot, timeout]
    {
        this->TryUnregister();          
        this->TryDeactivateKtlSystem(timeout);
    });
}

//
// KtlLoggerNode
//
KtlLoggerNode::KtlLoggerNode(
    Common::ComponentRoot const & root, 
    std::wstring const & fabricDataRoot,
    FabricNodeConfigSPtr const & fabricNodeConfig)
    : Common::RootedObject(root)
    , fabricDataRoot_(fabricDataRoot)
    , fabricNodeConfig_(fabricNodeConfig)
    , ktlSystem_()
    , isRegistered_(false)
    , useGlobalKtlSystem_(CommonConfig::GetConfig().UseGlobalKtlSystem)
{
    if (!useGlobalKtlSystem_)
    {
        KGuid ktlSystemGuid;
        ktlSystemGuid.CreateNew();

        ktlSystem_ = make_shared<KtlSystemBase>(ktlSystemGuid);
    }
}

KtlLoggerNode::~KtlLoggerNode()
{
}

AsyncOperationSPtr KtlLoggerNode::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        fabricDataRoot_,
        fabricNodeConfig_,
        timeout,
        callback,
        parent);    
}

ErrorCode KtlLoggerNode::OnEndOpen(__in AsyncOperationSPtr const & asyncOperation)
{
    std::unique_ptr<KtlLogManager::SharedLogContainerSettings> applicationSharedLogSettings;
    std::unique_ptr<KtlLogManager::SharedLogContainerSettings> systemServicesSharedLogSettings;

    ErrorCode e = OpenAsyncOperation::End(asyncOperation, applicationSharedLogSettings, systemServicesSharedLogSettings);
    if (e.IsSuccess())
    {
        applicationSharedLogSettings_ = make_shared<KtlLogger::SharedLogSettings>(std::move(applicationSharedLogSettings));
        systemServicesSharedLogSettings_ = make_shared<KtlLogger::SharedLogSettings>(std::move(systemServicesSharedLogSettings));
    }
    return e;
}

AsyncOperationSPtr KtlLoggerNode::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode KtlLoggerNode::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void KtlLoggerNode::OnAbort()
{
    PostThreadDeactivateKtlLoggerAndSystem(KtlLoggerConfig::GetConfig().KtlDeactivationTimeout);    
}

void KtlLoggerNode::TryUnregister()
{
#if defined(UDRIVER)
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    if (isRegistered_)
    {
        FileObjectTable::StopAndUnregisterOverlayManager(this->GetAllocator());
    }
#else
    // For kernel, we assume driver already installed by InstallForCITs
#endif  
}

NTSTATUS KtlLoggerNode::TryDeactivateKtlSystem(TimeSpan const timeout)
{
    if (!useGlobalKtlSystem_ && ktlSystem_)
    {
        NTSTATUS status;
        
        ktlSystem_->Deactivate(static_cast<ULONG>(timeout.TotalPositiveMilliseconds()));
        status = ktlSystem_->Status();
        if (! NT_SUCCESS(status))
        {
            // TODO: Use fabric tracing ???
            KTraceFailedAsyncRequest(status, NULL, 0, 0);          
        }
        return status;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

KAllocator & KtlLoggerNode::GetAllocator()
{
    if (useGlobalKtlSystem_)
    {
        return Common::GetSFDefaultNonPagedKAllocator();
    }

    return ktlSystem_->NonPagedAllocator();
}

KtlSystem & KtlLoggerNode::get_KtlSystem() const 
{
    if (useGlobalKtlSystem_)
    {
        return Common::GetSFDefaultKtlSystem();
    }

    ASSERT_IF(
        ktlSystem_ == nullptr,
        "Ktl system is null");

    return *ktlSystem_;
}

void KtlLoggerNode::WriteTraceInfo(ULONG A, ULONG B, ULONG C)
{
    WriteInfo("KtlLoggerNode", "Ktl: {0}, {1}, {2}", A, B, C);
}

void KtlLoggerNode::WriteTraceError(ULONG A, ULONG B, NTSTATUS Status)
{
    WriteError("KtlLoggerNode", "Ktl: {0}, {1}, Status {2}", A, B, Status);
}
