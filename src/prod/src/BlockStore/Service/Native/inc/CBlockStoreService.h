// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
//
// Represents the BlockStore Service.
#pragma once

#include "common.h"
#include "CDriverClient.h"
#include "trace.h"
#ifdef PLATFORM_UNIX
#include <signal.h>
#endif

class CBlockStoreService:  public IFabricCodePackageEventHandler,
                           public IFabricAsyncOperationCallback,
                           protected Common::ComUnknownBase
{
    DENY_COPY(CBlockStoreService)

    BEGIN_COM_INTERFACE_LIST(CBlockStoreService) 
        COM_INTERFACE_ITEM(IID_IUnknown, CBlockStoreService)
        COM_INTERFACE_ITEM((IID_IFabricCodePackageEventHandler), IFabricCodePackageEventHandler)              
        COM_INTERFACE_ITEM((IID_IFabricAsyncOperationCallback), IFabricAsyncOperationCallback)                
    END_COM_INTERFACE_LIST()

public:
    CBlockStoreService(service_fabric::reliable_service &service, boost::asio::io_service& ioService, u16string serviceName, string servicePartitionId, unsigned short port, bool isSingleNodeCluster, CDriverClient *pDriverClient);

    virtual ~CBlockStoreService();
    void Cleanup();
    
    __declspec(property(get = get_ServicePort)) unsigned short ServicePort;
    unsigned short get_ServicePort()
    {
        return servicePort_;
    }

    __declspec(property(get = get_ChangeRoleCompleteMutex)) std::mutex& ChangeRoleCompleteMutex;
    std::mutex& get_ChangeRoleCompleteMutex()
    {
        return mutexChangeRoleComplete_;
    }
    
    __declspec(property(get = get_IsServiceReady, put = set_IsServiceReady)) bool IsServiceReady;
    bool get_IsServiceReady()
    {
        return (bool)fIsServiceReady_;
    }

    void set_IsServiceReady(bool fIsServiceReady)
    {
        fIsServiceReady_ = fIsServiceReady;
    }
    
    __declspec(property(get = get_RefreshLUListDuringStartup, put = set_RefreshLUListDuringStartup)) bool RefreshLUListDuringStartup;
    bool get_RefreshLUListDuringStartup()
    {
        return (bool)fRefreshLUListDuringStartup_;
    }

    void set_RefreshLUListDuringStartup(bool fRefreshLUListDuringStartup)
    {
        fRefreshLUListDuringStartup_ = fRefreshLUListDuringStartup;
    }

    __declspec(property(get = get_WasServiceInterrupted)) bool WasServiceInterrupted;
    bool get_WasServiceInterrupted()
    {
        return serviceInterrupted_;
    }

    __declspec(property(get = get_IsSingleNodeCluster)) bool IsSingleNodeCluster;
    bool get_IsSingleNodeCluster()
    {
        return isSingleNodeCluster_;
    }

    void SetChangeRoleComplete() 
    {
        fIsChangeRoleComplete_ = true;
        cvChangeRoleComplete_.notify_one();
    }

    bool RegisterWithVolumeDriver();
    bool Start();
    
    static void ChangeRoleCallback(KEY_TYPE key, int32_t newRole);
    static void RemovePartitionCallback(KEY_TYPE key);
    static void AbortPartitionCallback(KEY_TYPE key);
    static void AddPartitionCallback(KEY_TYPE key, TxnReplicatorHandle replicator, PartitionHandle partition);

    static CBlockStoreService *&get_Instance()
    {
        // use a local static to support having everything in headers
        static CBlockStoreService *s_BlockStoreService = nullptr;

        return s_BlockStoreService;
    }
        
    virtual void STDMETHODCALLTYPE OnCodePackageEvent(
        /* [in] */ IFabricCodePackageActivator *source,
        /* [in] */ const FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION *eventDesc);

    virtual void STDMETHODCALLTYPE Invoke(
        /* [in] */ IFabricAsyncOperationContext *context);
    
private:
    void SetIFabricStatefulServicePartition(IFabricStatefulServicePartition* partition);
    string GetDockerVolumeDriverPort();
    bool RegisterWithVolumeDriverWorker();
    bool RegisterWithVolumeDriverWhenPrimary();
    void AcceptNextIncomingRequest();
    bool RefreshLUList();
    bool ShutdownVolumes(bool fServiceShuttingDown);
    bool InitializeInterruptHandlers();
#if defined(PLATFORM_UNIX)
    static void CBlockStoreService::InterruptHandlerCallback(
                    int sig, siginfo_t *siginfo, void *context);
#else
    static void InterruptHandlerCallback(int param);
#endif
    bool IsRCSReady();
    void TriggerServiceShutdown();
    bool ActivateVolumesAndCodePackages(KEY_TYPE key, FABRIC_REPLICA_ROLE newRole);
    void DeactivateCodePackagesAndShutdownVolumes(KEY_TYPE key);
    HRESULT ActivateDependentCodePackages();
    HRESULT DeactivateDependentCodePackages();

    static void ActivateVolumesAndCodePackagesAsync(KEY_TYPE key, FABRIC_REPLICA_ROLE newRole);
    
    
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    service_fabric::reliable_service &reliableService_;
    u16string serviceName_;
    string servicePartitionId_;
    CDriverClient *driverClient_;
    unsigned short servicePort_;
    FABRIC_REPLICA_ROLE lastReplicaRole_;
    bool serviceInterrupted_;

    // Reference to the pool of threads used by the service for processing
    // requests.
    std::vector<std::shared_ptr<boost::thread> > threadPool_;

    // Primitives to enable notification when ChangeRole is complete
    atomic<bool> fIsChangeRoleComplete_;
    std::mutex mutexChangeRoleComplete_;
    std::condition_variable cvChangeRoleComplete_;
    
    // Is the Service ready to accept requests?
    atomic<bool> fIsServiceReady_;
    atomic<bool> fRefreshLUListDuringStartup_;
    bool isSingleNodeCluster_;

    // Mutex used for synchronizing access to the service instance
    // between shutdown and change-role callback implementations.
    static std::mutex mutexSharedAccess_;

    // Primitives used to synchronize code package activations
    std::condition_variable cvCodePackagesActivated_;
    atomic<bool> fIsCodePackageActivationProcessed_;
    atomic<HRESULT> hresultCodePackageActivation_;

    std::condition_variable cvCodePackagesDeactivated_;
    atomic<bool> fIsCodePackageDeactivationProcessed_;
    atomic<HRESULT> hresultCodePackageDeactivation_;

    Common::ComPointer<IFabricCodePackageActivator> codePackageActivator_;
    Common::ComPointer<IFabricAsyncOperationContext> activateContext_;
    Common::ComPointer<IFabricAsyncOperationContext> deactivateContext_;
    Common::ComPointer<IFabricStatefulServicePartition2> partition_;
    ULONGLONG codePackageActivatorCallbackHandle_;
    atomic<bool> isCodePackageActive_;
    int dependentCodepackageTerminateCount_;
    int MAX_TERMINATE_THRESHOLD_;
    TxnReplicatorHandle replicator_;
#ifdef PLATFORM_UNIX
    struct sigaction siga;
#endif
};
