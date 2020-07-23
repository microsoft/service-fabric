// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsExchangeOp.h"
#include "DnsService.h"
#include "NetworkParams.h"
#include "Utility.h"

void DNS::CreateDnsService(
    __out IDnsService::SPtr& spDnsService,
    __in KAllocator& allocator,
    __in const IDnsTracer::SPtr& spTracer,
    __in const IDnsParser::SPtr& spDnsParser,
    __in const INetIoManager::SPtr& spNetIoManager,
    __in const IFabricResolve::SPtr& spFabricResolve,
    __in const IDnsCache::SPtr& spDnsCache,
    __in IFabricStatelessServicePartition2& fabricHealth,
    __in const DnsServiceParams& params
)
{
    DnsService::SPtr sp;
    DnsService::Create(/*out*/sp, allocator,
        spTracer, spDnsParser, spNetIoManager, spFabricResolve, spDnsCache, fabricHealth, params);

    spDnsService = sp.RawPtr();
}

/*static*/
void DnsService::Create(
    __out DnsService::SPtr& spDnsService,
    __in KAllocator& allocator,
    __in const IDnsTracer::SPtr& spTracer,
    __in const IDnsParser::SPtr& spDnsParser,
    __in const INetIoManager::SPtr& spNetIoManager,
    __in const IFabricResolve::SPtr& spFabricResolve,
    __in const IDnsCache::SPtr& spDnsCache,
    __in IFabricStatelessServicePartition2& fabricHealth,
    __in const DnsServiceParams& params
)
{
    spDnsService = _new(TAG, allocator) DnsService(
        spTracer, spDnsParser, spNetIoManager, spFabricResolve, spDnsCache, fabricHealth, params);

    KInvariant(spDnsService != nullptr);
}

DnsService::DnsService(
    __in const IDnsTracer::SPtr& spTracer,
    __in const IDnsParser::SPtr& spDnsParser,
    __in const INetIoManager::SPtr& spNetIoManager,
    __in const IFabricResolve::SPtr& spFabricResolve,
    __in const IDnsCache::SPtr& spDnsCache,
    __in IFabricStatelessServicePartition2& fabricHealth,
    __in const DnsServiceParams& params
) : _spTracer(spTracer),
_spDnsParser(spDnsParser),
_spNetIoManager(spNetIoManager),
_spFabricResolve(spFabricResolve),
_spFabricHealth(&fabricHealth),
_spDnsCache(spDnsCache),
_arrOps(GetThisAllocator()),
_params(params),
_fActive(false)
{
}

DnsService::~DnsService()
{
    Tracer().Trace(DnsTraceLevel_Noise, "Destructing DnsService.");
}


//***************************************
// BEGIN KAsyncContext region
//***************************************

void DnsService::OnStart()
{
    _fActive = true;

    Tracer().Trace(DnsTraceLevel_Info, "Parameters: DnsPort = {0}, NumberOfConcurrentQueries = {1}, MaxMessageSizeInKB = {2}, IsRecursiveQueryEnabled = {3}",
        static_cast<ULONG>(_params.Port), _params.NumberOfConcurrentQueries, _params.MaxMessageSizeInKB, (BOOLEAN)_params.IsRecursiveQueryEnabled);
    Tracer().Trace(DnsTraceLevel_Info, "Parameters: MaxCacheSize = {0}, FabricQueryTimeoutInSeconds = {1}, RecursiveQueryTimeoutInSeconds = {2}, NDots = {3}",
        _params.MaxCacheSize, _params.FabricQueryTimeoutInSeconds, _params.RecursiveQueryTimeoutInSeconds, _params.NDots);
    Tracer().Trace(DnsTraceLevel_Info, "Parameters: AllowMultipleListeners = {0}",
        (BOOLEAN)_params.AllowMultipleListeners);

    _spNetIoManager->StartManager(this/*parent*/);

    DnsHealthMonitor::Create(/*out*/_spHealthMonitor, GetThisAllocator(), _params, *_spFabricHealth.RawPtr());
    _spHealthMonitor->StartMonitor(this);

    // Start node DNS Cache monitor. Purpose of the monitor is to flush the cache
    // on startup and monitor the cache for invalid entries.
    DnsNodeCacheMonitor::Create(/*out*/_spCacheMonitor, GetThisAllocator(), _params, Tracer(), *_spDnsCache);
    _spCacheMonitor->StartMonitor(this);

    // UDP Listener
    AcquireActivities();

    _spFabricResolve->Open(this);

    NetworkParams::SPtr spNetworkParams;
    NetworkParams::Create(/*out*/spNetworkParams, GetThisAllocator(), Tracer());
    spNetworkParams->Initialize();
    _spNetworkParams = spNetworkParams.RawPtr();

    StartExchangeOps();
}

void DnsService::OnCancel()
{
    Tracer().Trace(DnsTraceLevel_Info, "DnsService, OnCancel called.");

    _spHealthMonitor->Cancel();
    _spCacheMonitor->Cancel();

    K_LOCK_BLOCK(_lockExchangeOp)
    {
        for (ULONG i = 0; i < _arrOps.Count(); i++)
        {
            DnsExchangeOp::SPtr spExchangeOp = _arrOps[i];
            spExchangeOp->Cancel();
        }
    }

    _spFabricResolve->CloseAsync();

    _spUdpListener->CloseAsync();

    _spNetIoManager->CloseAsync();

    Complete(STATUS_CANCELLED);
}

void DnsService::OnCompleted()
{
    Tracer().Trace(DnsTraceLevel_Info, "DnsService, OnCompleted called.");

    _arrOps.Clear();
    _completionCallback(STATUS_SUCCESS);
}

//***************************************
// END KAsyncContext region
//***************************************


bool DnsService::Open(
    __inout USHORT& port,
    __in DnsServiceCallback callback
)
{
    Tracer().TracesDnsServiceStartingOpen(
        static_cast<ULONG>(port)
    );

    _completionCallback = callback;

    _spNetIoManager->CreateUdpListener(/*out*/_spUdpListener, _params.AllowMultipleListeners);

    // UdpListener does not have a parent set because this async operation is not
    // started yet at this point. We want UdpListener to be started before Open returns.
    // Starting of UdpListener is the only operation that can prevent the start of DnsService.
    // Activity count is incremented OnStart. 
    // TODO: Better to use KAsyncService?
    UdpListenerCallback internalCallback(this, &DnsService::OnUdpListenerClosed);
    bool fListenerSuccessfullyStarted = _spUdpListener->StartListener(nullptr/*parent*/, /*inout*/port, internalCallback);

    WCHAR SourceId[] = L"System.FabricDnsService";
    WCHAR PropertyId[] = L"Socket";

    FABRIC_HEALTH_INFORMATION info;
    info.SourceId = SourceId;
    info.Property = PropertyId;
    info.RemoveWhenExpired = FALSE;
    info.TimeToLiveSeconds = FABRIC_HEALTH_REPORT_INFINITE_TTL;
    info.SequenceNumber = FABRIC_AUTO_SEQUENCE_NUMBER;

    std::wstring message;
    if (fListenerSuccessfullyStarted)
    {
        message = Common::wformatString("DnsService is listening on port {0}.", port);

        info.State = FABRIC_HEALTH_STATE_OK;
    }
    else
    {
        std::string output;

#if !defined(PLATFORM_UNIX)
        std::string command("netstat.exe -p UDP -nao");
#else
        std::string command("netstat -u udp -nap");
#endif

        Tracer().Trace(DnsTraceLevel_Info, "Begin execute command '{0}'", STR(command.c_str()));
        const ULONG timeoutInSec = 3;
        DNS::ExecuteCommandLine(command, output, timeoutInSec, Tracer());
        Tracer().Trace(DnsTraceLevel_Info, "End execute command '{0}', result '{1}'", STR(command.c_str()), STR(output.c_str()));

        std::vector<std::string> lines;
        Common::StringUtility::Split(output, lines, std::string("\n"));

        std::wstring processesListeningOnDnsPort;
        if (lines.size() > 0)
        {
            std::string findDns = Common::formatString(":{0} ", port);

            for (int i = 0; i < lines.size(); i++)
            {
                std::string & line = lines[i];
                if (std::string::npos != line.find(findDns))
                {
                    processesListeningOnDnsPort += Common::StringUtility::Utf8ToUtf16(line);
                    processesListeningOnDnsPort += L"\r\n";
                }
            }
        }

        message = Common::wformatString(
            "DnsService UDP listener is unable to start. "
            "Please make sure there are no processes listening on the DNS port {0}.\r\n"
            "List of processes listening on the DNS port:\r\n{1}",
            port, processesListeningOnDnsPort);

        info.State = FABRIC_HEALTH_STATE_WARNING;
    }

    info.Description = message.c_str();
    _spFabricHealth->ReportInstanceHealth(&info);

    if (!fListenerSuccessfullyStarted)
    {
        Tracer().Trace(DnsTraceLevel_Warning, "{0}", WSTR(message.c_str()));
        return false;
    }

    _port = port;

    Start(nullptr/*parent*/, nullptr/*callback*/);

    return true;
}

void DnsService::CloseAsync()
{
    _fActive = false;

    Tracer().Trace(DnsTraceLevel_Info, "DnsService starting close");

    Cancel();
}

void DnsService::OnUdpListenerClosed(
    __in NTSTATUS status
)
{
    Tracer().Trace(DnsTraceLevel_Noise, "DnsService closed UDP listener, status {0}", status);
    ReleaseActivities();
}

void DnsService::OnExchangeOpCompleted(
    __in_opt KAsyncContextBase* const parent,
    __in KAsyncContextBase& instance
)
{
    UNREFERENCED_PARAMETER(parent);

    DnsExchangeOp* pDnsExchangeOp = static_cast<DnsExchangeOp*>(&instance);
    pDnsExchangeOp->Reuse();

    K_LOCK_BLOCK(_lockExchangeOp)
    {
        if (_fActive)
        {
            KAsyncContextBase::CompletionCallback callback(this, &DnsService::OnExchangeOpCompleted);
            pDnsExchangeOp->StartExchange(this, callback);
        }
    }
}

void DnsService::StartExchangeOps()
{
    Tracer().Trace(DnsTraceLevel_Noise, "DnsService: starting {0} ExchangeOps", _params.NumberOfConcurrentQueries);

    for (ULONG i = 0; i < _params.NumberOfConcurrentQueries; i++)
    {
        DnsExchangeOp::SPtr spExchangeOp;
        DnsExchangeOp::Create(
            /*out*/spExchangeOp,
            GetThisAllocator(),
            Tracer(),
            *_spDnsParser,
            *_spNetIoManager,
            *_spUdpListener,
            *_spFabricResolve,
            *_spNetworkParams,
            _params
        );

        if (STATUS_SUCCESS != _arrOps.Append(spExchangeOp))
        {
            Tracer().Trace(DnsTraceLevel_Error, "Failed to append item to the array");
            KInvariant(false);
        }
    }

    KAsyncContextBase::CompletionCallback callback(this, &DnsService::OnExchangeOpCompleted);
    for (ULONG i = 0; i < _arrOps.Count(); i++)
    {
        DnsExchangeOp::SPtr spExchangeOp = _arrOps[i];
        spExchangeOp->StartExchange(this, callback);
    }
}
