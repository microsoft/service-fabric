// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Federation;
using namespace std;
using namespace Transport;
using namespace Common;
using namespace ServiceModel;
using namespace Fabric;
using namespace TestCommon;
using namespace FederationTestCommon;
using namespace Reliability;
using namespace Naming;
using namespace Hosting2;

StringLiteral const TraceTestComponent("FabricTest.FabricUpgradeImpl");
bool TestFabricUpgradeHostingImpl::ShouldFailDownload = false;
bool TestFabricUpgradeHostingImpl::ShouldFailValidation = false;
bool TestFabricUpgradeHostingImpl::ShouldFailUpgrade = false;

class TestFabricUpgradeHostingImpl::TestValidateUpgradeAsyncOperation
    : public AsyncOperation   
{
    DENY_COPY(TestValidateUpgradeAsyncOperation)

public:
    TestValidateUpgradeAsyncOperation(
        bool bValue,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        bValue_(bValue)                
    {       
    }

    static ErrorCode End(__out bool& outBValue, AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<TestValidateUpgradeAsyncOperation>(operation);        
        outBValue = thisPtr->bValue_;
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = ShouldFailValidation ? ErrorCode(ErrorCodeValue::OperationFailed, move(wstring(L"The Validation operation failed."))) : ErrorCodeValue::Success;
        this->TryComplete(thisSPtr, error);
    }                                

private:    
    bool bValue_;    
};

class TestFabricUpgradeHostingImpl::TestFabricUpgradeAsyncOperation
    : public AsyncOperation   
{       
    DENY_COPY(TestFabricUpgradeAsyncOperation)

public:
    TestFabricUpgradeAsyncOperation(
        weak_ptr<FabricNodeWrapper> testNodeWPtr,        
        FabricVersionInstance const & curVersionIns,
        FabricUpgradeSpecification const & fabricUpgradeSpec,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        testNodeWPtr_(testNodeWPtr),        
        curVersionIns_(curVersionIns),
        fabricUpgradeSpec_(fabricUpgradeSpec)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<TestFabricUpgradeAsyncOperation>(operation)->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {   
        if(ShouldFailUpgrade)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed, move(wstring(L"The upgrade operation failed."))));
            return;
        }

        shared_ptr<FabricNodeWrapper> fabricNodeWrapper;

        if( (fabricNodeWrapper = testNodeWPtr_.lock()) == nullptr )
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            return;
        }

        bool shouldRestartNode = false;
        if (curVersionIns_.Version.CodeVersion < fabricUpgradeSpec_.Version.CodeVersion
            || fabricUpgradeSpec_.UpgradeType == UpgradeType::Rolling_ForceRestart )
        {
            shouldRestartNode = true;
        }
        
        FabricVersionInstance newFabVersionInst = FabricVersionInstance(fabricUpgradeSpec_.Version, fabricUpgradeSpec_.InstanceId);
        FABRICSESSION.FabricDispatcher.UpgradeFabricData.SetFabricVersionInstance(fabricNodeWrapper->Id, newFabVersionInst);       

        if(!shouldRestartNode)
        {
            fabricNodeWrapper->SetFabricNodeConfigEntry( 
                fabricNodeWrapper->fabricNodeConfigSPtr_->NodeVersionEntry.Key, 
                newFabVersionInst.ToString(),
                true );  //true means updatestore         
        }
        else
        {
            Threadpool::Post([fabricNodeWrapper, newFabVersionInst]
            {                   
                Common::AcquireWriteLock lock(fabricNodeWrapper->nodeRebootLock_);

                fabricNodeWrapper->nodeOpenCompletedEvent_.WaitOne(FabricTestSessionConfig::GetConfig().OpenTimeout);                
                TestSession::FailTestIfNot( fabricNodeWrapper->nodeOpenCompletedEvent_.IsSet(), "Opening Node {0} Timeout", fabricNodeWrapper->Id );                
                fabricNodeWrapper->nodeOpenCompletedEvent_.Reset();

                WriteInfo(TraceTestComponent, "abort node {0}", fabricNodeWrapper->Id);
                fabricNodeWrapper->AbortInternal();                                                         
                                
                Sleep(static_cast<DWORD>(FabricTestSessionConfig::GetConfig().DelayOpenAfterAbortNode.TotalMilliseconds()));
                
                fabricNodeWrapper->isFirstCreate_ = true;
                fabricNodeWrapper->fabricNodeConfigSPtr_->NodeVersion = newFabVersionInst;
                FABRICSESSION.ClosePendingItem(wformatString("TestNode.{0}", fabricNodeWrapper->Id));
                FABRICSESSION.RemovePendingItem(wformatString("TestNode.{0}", fabricNodeWrapper->Id));

                bool expectFailure = false;
                bool skipRuntime = false;
                //ErrorCodeValue::Enum openErrorCode = ErrorCodeValue::AddressAlreadyInUse;                                
                ErrorCodeValue::Enum openErrorCode = ErrorCodeValue::Success;                                
                vector<ErrorCodeValue::Enum> retryErrors;
                retryErrors.push_back(ErrorCodeValue::AddressAlreadyInUse);
                WriteInfo(TraceTestComponent, "open node {0}", fabricNodeWrapper->Id);
                fabricNodeWrapper->OpenInternal(FabricTestSessionConfig::GetConfig().OpenTimeout, expectFailure, !skipRuntime, openErrorCode, retryErrors);                                
                fabricNodeWrapper->nodeOpenCompletedEvent_.WaitOne(FabricTestSessionConfig::GetConfig().OpenTimeout);                
                TestSession::FailTestIfNot( fabricNodeWrapper->nodeOpenCompletedEvent_.IsSet(), "Rebooting Node {0} Timeout", fabricNodeWrapper->Id ); 
            });
        }
        
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    virtual void OnCancel()
    {     
        AsyncOperation::OnCancel();
    }

private:    
    weak_ptr<FabricNodeWrapper> testNodeWPtr_; 
    FabricVersionInstance curVersionIns_;
    FabricUpgradeSpecification fabricUpgradeSpec_;    
}; 

Common::AsyncOperationSPtr TestFabricUpgradeHostingImpl::BeginDownload(
    Common::FabricVersion const & fabricVersion,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{   
    UNREFERENCED_PARAMETER(fabricVersion);
    auto error = ShouldFailDownload ? ErrorCode(ErrorCodeValue::OperationFailed, move(wstring(L"The download operation failed."))) : ErrorCodeValue::Success;
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
}

ErrorCode TestFabricUpgradeHostingImpl::EndDownload(Common::AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}        

AsyncOperationSPtr TestFabricUpgradeHostingImpl::BeginValidateAndAnalyze(
    Common::FabricVersionInstance const & currentFabricVersionInstance,
    ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent) 
{
    bool shouldCloseReplica = false;
    if (currentFabricVersionInstance.Version.CodeVersion < fabricUpgradeSpec.Version.CodeVersion
        || fabricUpgradeSpec.UpgradeType == UpgradeType::Rolling_ForceRestart )
    {
        shouldCloseReplica = true;
    }

    return  AsyncOperation::CreateAndStart<TestValidateUpgradeAsyncOperation>(shouldCloseReplica, callback, parent);
}

ErrorCode TestFabricUpgradeHostingImpl::EndValidateAndAnalyze(
    __out bool & shouldCloseReplica,
    Common::AsyncOperationSPtr const & operation) 
{
    return TestValidateUpgradeAsyncOperation::End(shouldCloseReplica, operation);          
}

AsyncOperationSPtr TestFabricUpgradeHostingImpl::BeginUpgrade(
    Common::FabricVersionInstance const & currentFabricVersionInstance,
    ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent) 
{
    return  AsyncOperation::CreateAndStart<TestFabricUpgradeAsyncOperation>(
        testNodeWPtr_, 
        currentFabricVersionInstance, 
        fabricUpgradeSpec, 
        callback, 
        parent);     
}

ErrorCode TestFabricUpgradeHostingImpl::EndUpgrade(Common::AsyncOperationSPtr const & operation)     
{
    return TestFabricUpgradeAsyncOperation::End(operation);
};
