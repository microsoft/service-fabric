// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// TestLeaseAgent.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace Common;

static TextTraceWriter const WriteLeaseInfo(TraceTaskCodes::Lease, LogLevel::Info);

struct AppContext
{
public:
    AppContext()
        : count_(0), closed_(false)
    {
    }

    wstring Name;
    wstring Socket;
    Dummy_LEASING_APPLICATION_EXPIRED_CALLBACK AppFailureCallback;
    Dummy_LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK AppEstablishCallback;
    Dummy_REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteAppFailureCallback;
    PVOID Context;

    bool StartCallback()
    {
        if (closed_)
        {
            return false;
        }

        ++count_;

        return true;
    }

    void EndCallback()
    {
        --count_;
        ASSERT_IF(count_ < 0, "app context count less than 0");

        if (closed_ && count_ == 0)
        {
            closeEvent_.Set();
        }
    }

    bool Close()
    {
        ASSERT_IF(closed_, "app context already closed");
        closed_ = true;

        return (count_ == 0);
    }

    void WaitCallbacks()
    {
        closeEvent_.WaitOne();
    }

private:
    int count_;
    bool closed_;
    ManualResetEvent closeEvent_;
};

typedef shared_ptr<AppContext> AppContextSPtr;

struct LeaseContext
{
    AppContextSPtr Subject;
    AppContextSPtr Monitor;
};

typedef shared_ptr<LeaseContext> LeaseContextSPtr;

static AppContextSPtr NullApp(nullptr);
static vector<AppContextSPtr> apps;
static vector<LeaseContextSPtr> leases;
static RwLock leaseLock;

static vector<AppContextSPtr>::iterator FindApp(wstring const & name)
{
    return find_if(apps.begin(), apps.end(), [name](AppContextSPtr const & x) -> bool
    {
        return (x->Name == name); 
    });
}

static AppContextSPtr const & GetApp(wstring const & name)
{
    auto it = FindApp(name);
    if (it == apps.end())
    {
        return NullApp;
    }

    return *it;
}

static AppContextSPtr const & GetApp(HANDLE appHandle)
{
    AppContext* app = (AppContext*) appHandle;
    AppContextSPtr const & result = GetApp(app->Name);
    if (result.get() != app)
    {
        return NullApp;
    }
    return result;
}

HANDLE WINAPI Dummy_RegisterLeasingApplication(
    __in LPCWSTR SocketAddress,
    __in LPCWSTR LeasingApplicationIdentifier,
    __in PLEASE_CONFIG_DURATIONS,
    __in LONG, // LeaseSuspendDurationMilliseconds
    __in LONG, // ArbitrationDurationMilliseconds
    __in LONG, // NumOfLeaseRetry
    __in Dummy_LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired,
    __in Dummy_REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired,
    __in Dummy_LEASING_APPLICATION_ARBITRATE_CALLBACK ,
    __in_opt Dummy_LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished,
    __in_opt PVOID context,
    __out_opt PLONGLONG Instance
    )
{
    AcquireExclusiveLock lock(leaseLock);
    wstring socket(SocketAddress);
    wstring name(LeasingApplicationIdentifier);

    if (GetApp(name))
    {
        return NULL;
    }

    AppContextSPtr app = make_shared<AppContext>();
    app->Name = move(name);
    app->Socket = move(socket);
    app->AppEstablishCallback = LeasingApplicationLeaseEstablished;
    app->AppFailureCallback = LeasingApplicationExpired;
    app->RemoteAppFailureCallback = RemoteLeasingApplicationExpired;
    app->Context = context;

    *Instance = 1;

    AppContext* result = app.get();

    apps.push_back(move(app));

    WriteLeaseInfo("Register", "App {0} registered {1}", result->Name, result);

    return result;
}

BOOL WINAPI Dummy_UnregisterLeasingApplication(__in HANDLE LeasingApplication)
{
    vector<LeaseContextSPtr> notificationList;
    AppContextSPtr app;
    bool closed;

    {
        AcquireExclusiveLock lock(leaseLock);
        app = GetApp(LeasingApplication);
        if (!app)
        {
            return false;
        }

        auto end = remove_if(apps.begin(), apps.end(), [app](AppContextSPtr const & x) { return (x == app); });
        apps.erase(end, apps.end());

        closed = app->Close();

        for (auto vit = leases.begin(); vit != leases.end(); vit++)
        {
            LeaseContextSPtr const & lease = *vit;
            if (lease->Subject == app)
            {
                notificationList.push_back(lease);
            }
        }

        for (LeaseContextSPtr const & lease : notificationList)
        {
            leases.erase(find(leases.begin(), leases.end(), lease));

            AppContextSPtr monitor = lease->Monitor;

            WriteLeaseInfo("Report", "Report {0} down to {1}", app->Name, monitor->Name);

            if (monitor->StartCallback())
            {
                wstring name = app->Name;
                Threadpool::Post([name, monitor]()
                { 
                    monitor->RemoteAppFailureCallback(name.c_str(), monitor->Context);

                    AcquireExclusiveLock lock(leaseLock);
                    monitor->EndCallback();
                });
            }
        }
    }

    if (!closed)
    {
        app->WaitCallbacks();
    }

    WriteLeaseInfo("Unregister", "App {0} unregistered {1}", app->Name, (AppContext*) LeasingApplication);

    return true;
}

HANDLE WINAPI Dummy_EstablishLease(
    __in HANDLE LeasingApplication,
    __in LPCWSTR RemoteApplicationIdentifier, 
    __in LPCWSTR,
    __in LONGLONG)
{
    AcquireExclusiveLock lock(leaseLock);

    AppContextSPtr const & subject = GetApp(LeasingApplication);
    ASSERT_IF(!subject, "subject {0} does not exist", (AppContext*) LeasingApplication);

    AppContextSPtr const & monitor = GetApp(wstring(RemoteApplicationIdentifier));
    if (!monitor)
    {
        SetLastError(ERROR_RETRY);;
        return NULL;
    }

    for (auto it = leases.begin(); it != leases.end(); it++)
    {
        LeaseContextSPtr lease = *it;
        ASSERT_IF(lease->Subject == subject && lease->Monitor == monitor,
            "Lease already established {0}-{1}",
            subject->Name, monitor->Name);
    }

    LeaseContextSPtr lease = make_shared<LeaseContext>();
    lease->Subject = subject;
    lease->Monitor = monitor;

    leases.push_back(lease);

    WriteLeaseInfo("Establish", "Lease established {0}-{1}", subject->Name, RemoteApplicationIdentifier);
    if (lease->Subject->StartCallback())
    {
        Threadpool::Post([lease, RemoteApplicationIdentifier]()
        {
            lease->Subject->AppEstablishCallback(lease.get(), RemoteApplicationIdentifier, lease->Subject->Context);

            AcquireExclusiveLock lock(leaseLock);
            lease->Subject->EndCallback();
        });
    }

    return lease.get();
}

BOOL WINAPI Dummy_TerminateLease(__in HANDLE lease)
{
    AcquireExclusiveLock lock(leaseLock);

    LeaseContext* leaseContext = (LeaseContext*) lease;

    for (auto it = leases.begin(); it != leases.end(); it++)
    {
        LeaseContextSPtr const & existing = *it;
        if (existing->Subject == leaseContext->Subject && existing->Monitor == leaseContext->Monitor)
        {
            WriteLeaseInfo("Terminate", "Lease terminated {0}-{1}", leaseContext->Subject->Name, leaseContext->Monitor->Name);
            leases.erase(it);
            return true;
        }
    }

    return FALSE;
}

BOOL WINAPI Dummy_GetLeasingApplicationExpirationTime(__in HANDLE ,__out PLONG RemainingTimeToLiveMilliseconds, __out PLONGLONG KernelCurrentTime)
{
    *RemainingTimeToLiveMilliseconds = (LONG) TimeSpan::FromMinutes(1.0).TotalMilliseconds();
    *KernelCurrentTime = (LONGLONG) Stopwatch::GetTimestamp();
    return TRUE;
}

BOOL WINAPI Dummy_SetGlobalLeaseExpirationTime(__in HANDLE ,__in LONGLONG)
{
    return TRUE;
}

BOOL WINAPI Dummy_GetRemoteLeaseExpirationTime(
__in HANDLE,
__in LPCWSTR,
__out PLONG MonitorExpireTTL,
__out PLONG SubjectExpireTTL)
{
    *MonitorExpireTTL = 1000;
    *SubjectExpireTTL = 1000;
    return TRUE;
}

BOOL WINAPI Dummy_CompleteArbitrationSuccessProcessing(__in HANDLE, LONGLONG, __in LPCWSTR, LONGLONG, LONG, LONG)
{
    //TODO: Delete the registed application. Might never be done.
    return TRUE;
}

BOOL WINAPI Dummy_FaultLeaseAgent(__in LPCWSTR SocketAddress)
{
    wstring socket(SocketAddress);
    map<LeaseContextSPtr, wstring> monitorNotificationList;
    map<LeaseContextSPtr, wstring> subjectNotificationList;
    vector<AppContextSPtr> appNotificationList;

    bool foundLeaseAgent = false;
    for (auto it1 = apps.begin(); it1 != apps.end(); it1++)
    {
        AppContextSPtr const & appContext = *it1;
        if(appContext->Socket == socket)
        {
            appContext->Name;
            for (auto it2 = leases.begin(); it2 != leases.end(); it2++)
            {
                if(*it2)
                {
                    LeaseContextSPtr const & leaseContext = *it2;
                    if(leaseContext->Monitor == appContext)
                    {
                        foundLeaseAgent = true;
                        monitorNotificationList[leaseContext] = appContext->Name;
                    }
                    else if(leaseContext->Subject == appContext)
                    {
                        foundLeaseAgent = true;
                        subjectNotificationList[leaseContext] = appContext->Name;
                    }
                }
            }

            appNotificationList.push_back(appContext);
        }
    }

    AcquireExclusiveLock lock(leaseLock);
    for (auto it2 = monitorNotificationList.begin(); it2 != monitorNotificationList.end(); it2++)
    {
        LeaseContextSPtr const & leaseContext = (*it2).first;
        wstring RemoteApplicationIdentifier = (*it2).second;
        if (leaseContext->Subject != nullptr && leaseContext->Subject->StartCallback())
        {
            Threadpool::Post([leaseContext, RemoteApplicationIdentifier]()
            {
                WriteLeaseInfo("Report", "App failure notification {0} to {1}.", RemoteApplicationIdentifier, leaseContext->Subject->Name);
                leaseContext->Subject->RemoteAppFailureCallback(RemoteApplicationIdentifier.c_str(), leaseContext->Subject->Context);

                AcquireExclusiveLock lock(leaseLock);
                leaseContext->Subject->EndCallback();
            });
        }
    }

    for (auto it2 = subjectNotificationList.begin(); it2 != subjectNotificationList.end(); it2++)
    {
        LeaseContextSPtr const & leaseContext = (*it2).first;
        wstring RemoteApplicationIdentifier = (*it2).second;
        if (leaseContext->Monitor != nullptr && leaseContext->Monitor->StartCallback())
        {
            Threadpool::Post([leaseContext, RemoteApplicationIdentifier]()
            {
                WriteLeaseInfo("Report", "App failure notification {0} to {1}.", RemoteApplicationIdentifier, leaseContext->Subject->Name);
                leaseContext->Monitor->RemoteAppFailureCallback(RemoteApplicationIdentifier.c_str(), leaseContext->Monitor->Context);

                AcquireExclusiveLock lock(leaseLock);
                leaseContext->Monitor->EndCallback();
            });
        }
    }

    for (auto it2 = appNotificationList.begin(); it2 != appNotificationList.end(); it2++)
    {
        AppContextSPtr const & appContext = *it2;
        if (appContext != nullptr && appContext->StartCallback())
        {
            Threadpool::Post([appContext]()
            {
                appContext->AppFailureCallback(appContext->Context);
                AcquireExclusiveLock lock(leaseLock);
                appContext->EndCallback();
            });
        }
    }

    return foundLeaseAgent;
}

wstring Dummy_DumpLeases()
{
    AcquireReadLock lock(leaseLock);

    wstring result;
    StringWriter writer(result);
    for (auto it = leases.begin(); it != leases.end(); it++)
    {
        LeaseContextSPtr lease = *it;
        writer.WriteLine("{0}-{1}", lease->Subject->Name, lease->Monitor->Name);
    }

    return result;
}
