// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::ClusterManager;
using namespace Management::FaultAnalysisService;
using namespace Management::UpgradeOrchestrationService;
using namespace Management::CentralSecretService;
using namespace Naming;

StringLiteral const TraceComponent("ComFabricClient");

class ComFabricClient::ClientAsyncOperation : public ComAsyncOperationContext
{
public:
    ClientAsyncOperation(ComFabricClient& owner)
        : owner_(owner)
    {
    }

    virtual ~ClientAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        ComponentRootSPtr rootSPtr;
        auto hr = this->CreateComComponentRoot(rootSPtr);
        if (FAILED(hr)) { return hr; }

        hr = ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        // Parse and Validate Timeout
        if (timeoutMilliSeconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliSeconds));
        }

        return S_OK;
    }

    HRESULT CreateComComponentRoot(__out ComponentRootSPtr & result)
    {
        ComPointer<IUnknown> rootCPtr;
        HRESULT hr = owner_.QueryInterface(IID_IUnknown, rootCPtr.VoidInitializationAddress());
        if (FAILED(hr)) { return hr; }

        result = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

        return S_OK;
    }

protected:
    __declspec(property(get=get_Owner)) ComFabricClient const &Owner;
    __declspec(property(get=get_Timeout)) Common::TimeSpan Timeout;

    ComFabricClient const & get_Owner() { return owner_; }
    Common::TimeSpan get_Timeout() { return timeout_; }

    HRESULT ComAsyncOperationContextEnd()
    {
        return ComAsyncOperationContext::End();
    }

private:
    ComFabricClient& owner_;
    TimeSpan timeout_;
};

// 9841493d-c0eb-47d1-b7c1-5c9c29dfb438
static const GUID CLSID_ComFabricClient_SetAclOperation =
{ 0x9841493d, 0xc0eb, 0x47d1, { 0xb7, 0xc1, 0x5c, 0x9c, 0x29, 0xdf, 0xb4, 0x38 } };

class ComFabricClient::SetAclOperation : public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(SetAclOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        SetAclOperation,
        CLSID_ComFabricClient_SetAclOperation,
        SetAclOperation,
        ComAsyncOperationContext)

public:
    SetAclOperation(ComFabricClient& owner) : ClientAsyncOperation(owner)
    {
    }

    HRESULT Initialize(
        _In_ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
        _In_ const FABRIC_SECURITY_ACL *acl,
        _In_ DWORD timeoutMilliseconds,
        _In_ IFabricAsyncOperationCallback *callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliseconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = AccessControl::ResourceIdentifier::FromPublic(resource, resource_);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return AccessControl::FabricAcl::FromPublic(acl, acl_).ToHResult();
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<SetAclOperation> thisOperation(context, CLSID_ComFabricClient_SetAclOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:
    void OnStart(AsyncOperationSPtr const & proxySPtr) override
    {
        Owner.accessControlClient_->BeginSetAcl(
            *resource_,
            acl_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnSetAclComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnSetAclComplete(AsyncOperationSPtr const& operation)
    {
        auto error = Owner.accessControlClient_->EndSetAcl(operation);
        TryComplete(operation->Parent, error);
    }

    AccessControl::ResourceIdentifier::SPtr resource_;
    AccessControl::FabricAcl acl_;
};

// 32d330f9-1cd4-4bbf-a385-1c78defe2ed2
static const GUID CLSID_ComFabricClient_GetAclOperation =
{ 0x32d330f9, 0x1cd4, 0x4bbf, { 0xa3, 0x85, 0x1c, 0x78, 0xde, 0xfe, 0x2e, 0xd2 } };

class ComFabricClient::GetAclOperation : public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetAclOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetAclOperation,
        CLSID_ComFabricClient_GetAclOperation,
        GetAclOperation,
        ComAsyncOperationContext)

public:
    GetAclOperation(ComFabricClient & owner) : ClientAsyncOperation(owner)
    {
    }

    HRESULT Initialize(
        _In_ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
        _In_ DWORD timeoutMilliseconds,
        _In_ IFabricAsyncOperationCallback *callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliseconds, callback);
        if (FAILED(hr)) { return hr; }

        return AccessControl::ResourceIdentifier::FromPublic(resource, resource_).ToHResult();
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricGetAclResult **result)
    {
        if (context == NULL || result == NULL ) { return E_POINTER; }

        ComPointer<GetAclOperation> thisOperation(context, CLSID_ComFabricClient_GetAclOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricGetAclResult> comResult =
                make_com<ComGetAclResult, IFabricGetAclResult>(thisOperation->result_);

            *result = comResult.DetachNoRelease();
        }

        return hr;
   }

protected:
    void OnStart(AsyncOperationSPtr const & proxySPtr) override
    {
        Owner.accessControlClient_->BeginGetAcl(
            *resource_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetAclComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetAclComplete(AsyncOperationSPtr const& operation)
    {
        auto error = Owner.accessControlClient_->EndGetAcl(operation, result_);
        TryComplete(operation->Parent, error);
    }

    AccessControl::ResourceIdentifier::SPtr resource_;
    AccessControl::FabricAcl result_;
};

// {df0f9add-a0d8-42cb-8354-6293f20e62f6}
static const GUID CLSID_ComFabricClient_ProvisionApplicationTypeOperation =
{ 0xdf0f9add, 0xa0d8, 0x42cb, { 0x83, 0x54, 0x62, 0x93, 0xf2, 0x0e, 0x62, 0xf6 } };

class ComFabricClient::ProvisionApplicationTypeOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(ProvisionApplicationTypeOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ProvisionApplicationTypeOperation,
        CLSID_ComFabricClient_ProvisionApplicationTypeOperation,
        ProvisionApplicationTypeOperation,
        ComAsyncOperationContext)
public:

    ProvisionApplicationTypeOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , description_()
    {
    }

    virtual ~ProvisionApplicationTypeOperation() {};

    HRESULT Initialize(
        __in const FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        description_ = make_unique<ProvisionApplicationTypeDescription>();
        auto error = description_->FromPublicApi(*description);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    HRESULT Initialize(
        __in const FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        description_ = make_unique<ProvisionApplicationTypeDescription>();
        auto error = description_->FromPublicApi(*description);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ProvisionApplicationTypeOperation> thisOperation(context, CLSID_ComFabricClient_ProvisionApplicationTypeOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginProvisionApplicationType(
            *description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnProvisionComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnProvisionComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndProvisionApplicationType(operation);
        TryComplete(operation->Parent, error);
    }

    unique_ptr<ProvisionApplicationTypeDescription> description_;
};

// {bf7cbf61-5493-43c9-96b9-cabd29980ffb}
static const GUID CLSID_ComFabricClient_UnprovisionApplicationTypeOperation =
{ 0xbf7cbf61, 0x5493, 0x43c9, { 0x96, 0xb9, 0xca, 0xbd, 0x29, 0x98, 0x0f, 0xfb } };

class ComFabricClient::UnprovisionApplicationTypeOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UnprovisionApplicationTypeOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UnprovisionApplicationTypeOperation,
        CLSID_ComFabricClient_UnprovisionApplicationTypeOperation,
        UnprovisionApplicationTypeOperation,
        ComAsyncOperationContext)
public:

    UnprovisionApplicationTypeOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , description_()
    {
    }

    virtual ~UnprovisionApplicationTypeOperation() {};

    HRESULT Initialize(
        __in const FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = description_.FromPublicApi(*description);
        if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UnprovisionApplicationTypeOperation> thisOperation(context, CLSID_ComFabricClient_UnprovisionApplicationTypeOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginUnprovisionApplicationType(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUnprovisionComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUnprovisionComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndUnprovisionApplicationType(operation);
        TryComplete(operation->Parent, error);
    }

    UnprovisionApplicationTypeDescription description_;
};

// {a7e9df4e-9ed2-46c2-853e-30549a2c2d79}
static const GUID CLSID_ComFabricClient_CreateApplicationOperation =
{ 0xa7e9df4e, 0x9ed2, 0x46c2, { 0x85, 0x3e, 0x30, 0x54, 0x9a, 0x2c, 0x2d, 0x79 } };

class ComFabricClient::CreateApplicationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateApplicationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateApplicationOperation,
        CLSID_ComFabricClient_CreateApplicationOperation,
        CreateApplicationOperation,
        ComAsyncOperationContext)
public:

    CreateApplicationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~CreateApplicationOperation() {};

    HRESULT Initialize(
        __in ServiceModel::ApplicationDescriptionWrapper &appDesc,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        appDesc_ = move(appDesc);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateApplicationOperation> thisOperation(context, CLSID_ComFabricClient_CreateApplicationOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginCreateApplication(
            appDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnCreateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndCreateApplication(operation);
        TryComplete(operation->Parent, error);
    }

    ApplicationDescriptionWrapper appDesc_;
};

// {06dc88c6-809c-45bb-809c-fc454d93c10d}
static const GUID CLSID_ComFabricClient_UpdateApplicationOperation =
{ 0x6dc88c6, 0x809c, 0x45bb, { 0x80, 0x9c, 0xfc, 0x45, 0x4d, 0x93, 0xc1, 0xd } };

class ComFabricClient::UpdateApplicationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpdateApplicationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    UpdateApplicationOperation,
    CLSID_ComFabricClient_UpdateApplicationOperation,
    UpdateApplicationOperation,
    ComAsyncOperationContext)
public:

    UpdateApplicationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpdateApplicationOperation() {};

    HRESULT Initialize(
        __in ServiceModel::ApplicationUpdateDescriptionWrapper &appUpdateDesc,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        appUpdateDesc_ = move(appUpdateDesc);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpdateApplicationOperation> thisOperation(context, CLSID_ComFabricClient_UpdateApplicationOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginUpdateApplication(
            appUpdateDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnUpdateComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnUpdateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndUpdateApplication(operation);
        TryComplete(operation->Parent, error);
    }

    ApplicationUpdateDescriptionWrapper appUpdateDesc_;
};


// {e3b96d7d-4d54-48c4-8167-44ee61067783}
static const GUID CLSID_ComFabricClient_UpgradeApplicationOperation =
{ 0xe3b96d7d, 0x4d54, 0x48c4, { 0x81, 0x67, 0x44, 0xee, 0x61, 0x06, 0x77, 0x83 } };

class ComFabricClient::UpgradeApplicationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpgradeApplicationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpgradeApplicationOperation,
        CLSID_ComFabricClient_UpgradeApplicationOperation,
        UpgradeApplicationOperation,
        ComAsyncOperationContext)
public:

    UpgradeApplicationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpgradeApplicationOperation() {};

    HRESULT Initialize(
        __in ApplicationUpgradeDescriptionWrapper &appUpgradeDesc,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        appUpgradeDesc_ = move(appUpgradeDesc);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpgradeApplicationOperation> thisOperation(context, CLSID_ComFabricClient_UpgradeApplicationOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginUpgradeApplication(
            appUpgradeDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUpgradeComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUpgradeComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndUpgradeApplication(operation);
        TryComplete(operation->Parent, error);
    }

    ApplicationUpgradeDescriptionWrapper appUpgradeDesc_;
};

// {1254d3ef-96c3-4b71-b646-f352c939f1f5}
static const GUID CLSID_ComFabricClient_UpdateApplicationUpgradeOperation =
{ 0x1254d3ef, 0x96c3, 0x4b71, { 0xb6, 0x46, 0xf3, 0x52, 0xc9, 0x39, 0xf1, 0xf5 } };

class ComFabricClient::UpdateApplicationUpgradeOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpdateApplicationUpgradeOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpdateApplicationUpgradeOperation,
        CLSID_ComFabricClient_UpdateApplicationUpgradeOperation,
        UpdateApplicationUpgradeOperation,
        ComAsyncOperationContext)
public:

    UpdateApplicationUpgradeOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpdateApplicationUpgradeOperation() {};

    HRESULT Initialize(
        __in const FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION * publicDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        publicDescription_ = publicDescription;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpdateApplicationUpgradeOperation> thisOperation(context, CLSID_ComFabricClient_UpdateApplicationUpgradeOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        // Convert from public here instead of during Begin() so that
        // we can surface any error detail messages that might result
        // from the attempted conversion.
        //
        auto error = internalDescription_.FromPublicApi(*publicDescription_);
        if (error.IsSuccess())
        {
            Owner.appMgmtClient_->BeginUpdateApplicationUpgrade(
                internalDescription_,
                Timeout,
                [this](AsyncOperationSPtr const & operation)
                {
                    this->OnUpdateComplete(operation);
                },
                proxySPtr);
        }
        else
        {
            this->TryComplete(proxySPtr, error);
        }
    }

private:
    void OnUpdateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndUpdateApplicationUpgrade(operation);
        TryComplete(operation->Parent, error);
    }

    FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION const * publicDescription_;
    ApplicationUpgradeUpdateDescription internalDescription_;
};

// {40d4ea78-3709-4d79-a193-319354c75e57}
static const GUID CLSID_ComFabricClient_RollbackApplicationUpgradeOperation =
{ 0x40d4ea78, 0x3709, 0x4d79, { 0xa1, 0x93, 0x31, 0x93, 0x54, 0xc7, 0x5e, 0x57 } };

class ComFabricClient::RollbackApplicationUpgradeOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RollbackApplicationUpgradeOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RollbackApplicationUpgradeOperation,
        CLSID_ComFabricClient_RollbackApplicationUpgradeOperation,
        RollbackApplicationUpgradeOperation,
        ComAsyncOperationContext)
public:

    RollbackApplicationUpgradeOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~RollbackApplicationUpgradeOperation() {};

    static HRESULT Begin(
        ComFabricClient & owner,
        FABRIC_URI applicationName,
        DWORD timeoutMilliseconds,
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        if (applicationName == NULL || callback == NULL || context == NULL)
        {
            return E_POINTER;
        }

        auto operation = make_com<RollbackApplicationUpgradeOperation>(owner);

        auto hr = operation->Initialize(
            applicationName,
            timeoutMilliseconds,
            callback);
        if (FAILED(hr)) { return hr; }

        return ComAsyncOperationContext::StartAndDetach(move(operation), context);
    }

    HRESULT Initialize(
        __in FABRIC_URI applicationName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        publicName_ = applicationName;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RollbackApplicationUpgradeOperation> thisOperation(context, CLSID_ComFabricClient_RollbackApplicationUpgradeOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        NamingUri applicationName;
        auto hr = NamingUri::TryParse(publicName_, Constants::FabricClientTrace, applicationName);

        if (FAILED(hr))
        {
            this->TryComplete(proxySPtr, ErrorCode::FromHResult(hr));
        }
        else
        {
            Owner.appMgmtClient_->BeginRollbackApplicationUpgrade(
                applicationName,
                Timeout,
                [this](AsyncOperationSPtr const & operation) { this->OnOperationComplete(operation); },
                proxySPtr);
        }
    }

private:
    void OnOperationComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndRollbackApplicationUpgrade(operation);
        this->TryComplete(operation->Parent, error);
    }

    FABRIC_URI publicName_;
};

// {71bcd2ca-c1b5-4106-b400-bf3dffbc6ea6}
static const GUID CLSID_ComFabricClient_DeleteApplicationOperation =
{ 0x71bcd2ca, 0xc1b5, 0x4106, { 0xb4, 0x00, 0xbf, 0x3d, 0xff, 0xbc, 0x6e, 0xa6 } };

class ComFabricClient::DeleteApplicationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeleteApplicationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeleteApplicationOperation,
        CLSID_ComFabricClient_DeleteApplicationOperation,
        DeleteApplicationOperation,
        ComAsyncOperationContext)
public:

    static HRESULT BeginOperation(
        ComFabricClient & owner,
        const FABRIC_DELETE_APPLICATION_DESCRIPTION * description,
        DWORD timeoutMilliseconds,
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        if (context == NULL || description == NULL || description->ApplicationName == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComPointer<DeleteApplicationOperation> operation = make_com<DeleteApplicationOperation>(owner);

        HRESULT hr = operation->Initialize(
            description,
            timeoutMilliseconds,
            callback);

        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
    }

    DeleteApplicationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , description_()
    {
    }

    virtual ~DeleteApplicationOperation() {};

    HRESULT Initialize(
        __in LPCWSTR applicationName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        NamingUri appNameUri;
        hr = NamingUri::TryParse(applicationName, Constants::FabricClientTrace, appNameUri);
        description_.ApplicationName = appNameUri;
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    HRESULT Initialize(
        __in const FABRIC_DELETE_APPLICATION_DESCRIPTION * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = description_.FromPublicApi(*description);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeleteApplicationOperation> thisOperation(context, CLSID_ComFabricClient_DeleteApplicationOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginDeleteApplication(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeleteComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnDeleteComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndDeleteApplication(operation);
        TryComplete(operation->Parent, error);
    }

    DeleteApplicationDescription description_;
};

// {a8a6cb79-4928-4b52-8df7-a83a5ef87172}
static const GUID CLSID_ComFabricClient_GetApplicationUpgradeProgressOperation =
{ 0xa8a6cb79, 0x4928, 0x4b52, { 0x8d, 0xf7, 0xa8, 0x3a, 0x5e, 0xf8, 0x71, 0x72 } };

class ComFabricClient::GetApplicationUpgradeProgressOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationUpgradeProgressOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetApplicationUpgradeProgressOperation,
        CLSID_ComFabricClient_GetApplicationUpgradeProgressOperation,
        GetApplicationUpgradeProgressOperation,
        ComAsyncOperationContext)
public:

    GetApplicationUpgradeProgressOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetApplicationUpgradeProgressOperation() {};

    HRESULT Initialize(
        __in LPCWSTR applicationName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(applicationName, Constants::FabricClientTrace, applicationName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, REFIID riid, void **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationUpgradeProgressOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationUpgradeProgressOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComApplicationUpgradeProgressResult>(thisOperation->appUpgradeResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginGetApplicationUpgradeProgress(
            applicationName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUpgradeProgressComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUpgradeProgressComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndGetApplicationUpgradeProgress(operation, appUpgradeResultPtr_);
        TryComplete(operation->Parent, error);
    }

    NamingUri applicationName_;
    IApplicationUpgradeProgressResultPtr appUpgradeResultPtr_;
};

// {b6d9252f-5aaa-48f2-b1b9-0262555e0dd5}
static const GUID CLSID_ComFabricClient_GetComposeDeploymentUpgradeProgressOperation =
{ 0xb6d9252f, 0x5aaa, 0x48f2, { 0xb1, 0xb9, 0x2, 0x62, 0x55, 0x5e, 0xd, 0xd5 } };

class ComFabricClient::GetComposeDeploymentUpgradeProgressOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetComposeDeploymentUpgradeProgressOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetComposeDeploymentUpgradeProgressOperation,
        CLSID_ComFabricClient_GetComposeDeploymentUpgradeProgressOperation,
        GetComposeDeploymentUpgradeProgressOperation,
        ComAsyncOperationContext)
public:

    GetComposeDeploymentUpgradeProgressOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , deploymentName_()
        , progress_()
    {
    }

    HRESULT Initialize(
        __in LPCWSTR deploymentName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            deploymentName,
            false,
            deploymentName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, REFIID riid, void **result)
    {
        if (context == nullptr || result == nullptr) { return E_POINTER; }

        ComPointer<GetComposeDeploymentUpgradeProgressOperation> thisOperation(context, CLSID_ComFabricClient_GetComposeDeploymentUpgradeProgressOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComComposeDeploymentUpgradeProgressResult>(thisOperation->progress_);

            hr = comPtr->QueryInterface(riid, result);
        }
        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.composeMgmtClient_->BeginGetComposeDeploymentUpgradeProgress(
            deploymentName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUpgradeProgressComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUpgradeProgressComplete(__in AsyncOperationSPtr const & operation)
    {
        auto error = Owner.composeMgmtClient_->EndGetComposeDeploymentUpgradeProgress(operation, progress_);
        this->TryComplete(operation->Parent, error);
    }

    wstring deploymentName_;
    ComposeDeploymentUpgradeProgress progress_;
};

// {d33a3d52-4256-46e3-a860-6b1d5a71a6c3}
static const GUID CLSID_ComFabricClient_MoveNextUpgradeDomainOperation =
{ 0xd33a3d52, 0x4256, 0x46e3, { 0xa8, 0x60, 0x6b, 0x1d, 0x5a, 0x71, 0xa6, 0xc3 } };

class ComFabricClient::MoveNextUpgradeDomainOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(MoveNextUpgradeDomainOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        MoveNextUpgradeDomainOperation,
        CLSID_ComFabricClient_MoveNextUpgradeDomainOperation,
        MoveNextUpgradeDomainOperation,
        ComAsyncOperationContext)
public:

    MoveNextUpgradeDomainOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~MoveNextUpgradeDomainOperation() {};

    HRESULT Initialize(
        __in IFabricApplicationUpgradeProgressResult2 *upgradeProgress,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        Common::ComPointer<ComApplicationUpgradeProgressResult> comUpdateProgress(upgradeProgress, CLSID_ComApplicationUpgradeProgressResult);
        appUpgradeResultPtr_ = comUpdateProgress->get_Impl();

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<MoveNextUpgradeDomainOperation> thisOperation(context, CLSID_ComFabricClient_MoveNextUpgradeDomainOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginMoveNextApplicationUpgradeDomain(
            appUpgradeResultPtr_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnMoveNextUpgradeDomainComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnMoveNextUpgradeDomainComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndMoveNextApplicationUpgradeDomain(operation);
        TryComplete(operation->Parent, error);
    }

    IApplicationUpgradeProgressResultPtr appUpgradeResultPtr_;
};

// {d4c6ea39-d3f2-417d-813e-4be77c6e2837}
static const GUID CLSID_ComFabricClient_MoveNextUpgradeDomainOperation2 =
{ 0xd4c6ea39, 0xd3f2, 0x417d, { 0x81, 0x3e, 0x4b, 0xe7, 0x7c, 0x6e, 0x28, 0x37 } };

class ComFabricClient::MoveNextUpgradeDomainOperation2 :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(MoveNextUpgradeDomainOperation2)

    COM_INTERFACE_AND_DELEGATE_LIST(
        MoveNextUpgradeDomainOperation2,
        CLSID_ComFabricClient_MoveNextUpgradeDomainOperation2,
        MoveNextUpgradeDomainOperation2,
        ComAsyncOperationContext)
public:

    MoveNextUpgradeDomainOperation2(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~MoveNextUpgradeDomainOperation2() {};

    HRESULT Initialize(
        __in FABRIC_URI appName,
        __in LPCWSTR nextUpgradeDomain,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(appName, Constants::FabricClientTrace, appName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            nextUpgradeDomain,
            false /*acceptNull*/,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            nextUpgradeDomain_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<MoveNextUpgradeDomainOperation2> thisOperation(context, CLSID_ComFabricClient_MoveNextUpgradeDomainOperation2);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginMoveNextApplicationUpgradeDomain2(
            appName_,
            nextUpgradeDomain_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnMoveNextUpgradeDomainComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnMoveNextUpgradeDomainComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndMoveNextApplicationUpgradeDomain2(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri appName_;
    wstring nextUpgradeDomain_;
};

// {8475fe6a-7b10-4f14-ab99-fa47ef8ec36d}
static const GUID CLSID_ComFabricClient_GetApplicationManifestOperation =
{ 0x8475fe6a, 0x7b10, 0x4f14, { 0xab, 0x99, 0xfa, 0x47, 0xef, 0x8e, 0xc3, 0x6d } };

class ComFabricClient::GetApplicationManifestOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationManifestOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetApplicationManifestOperation,
        CLSID_ComFabricClient_GetApplicationManifestOperation,
        GetApplicationManifestOperation,
        ComAsyncOperationContext)
public:

    GetApplicationManifestOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetApplicationManifestOperation() {};

    HRESULT Initialize(
        __in LPCWSTR applicationTypeName,
        __in LPCWSTR applicationTypeVersion,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            applicationTypeName,
            false /*acceptNull*/,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            applicationTypeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            applicationTypeVersion,
            false /*acceptNull*/,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            applicationTypeVersion_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricStringResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationManifestOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationManifestOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(
                thisOperation->applicationManifest_);
            *result = stringResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginGetApplicationManifest(
            applicationTypeName_,
            applicationTypeVersion_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetApplicationManifestComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnGetApplicationManifestComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndGetApplicationManifest(operation, applicationManifest_);
        TryComplete(operation->Parent, error);
    }

    wstring applicationTypeName_;
    wstring applicationTypeVersion_;
    wstring applicationManifest_;
};

// {99f68f95-b7d5-4947-be44-4cfda980fb77}
static const GUID CLSID_ComFabricClient_DeployServicePackageToNodeOperation =
{0x99f68f95, 0xb7d5, 0x4947, {0xbe, 0x44, 0x4c, 0xfd, 0xa9, 0x80, 0xfb, 0xff } };

class ComFabricClient::DeployServicePackageToNodeOperation : public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeployServicePackageToNodeOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeployServicePackageToNodecOperation,
        CLSID_ComFabricClient_DeployServicePackageToNodeOperation,
        DeployServicePackageToNodeOperation,
        ComAsyncOperationContext)

public:
    DeployServicePackageToNodeOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        applicationTypeName_(),
        applicationTypeVersion_(),
        nodeName_(),
        packageSharingPolicies_()
    {
    }

    ~DeployServicePackageToNodeOperation(){};

    HRESULT Initialize(
        __in LPCWSTR applicationTypeName,
        __in LPCWSTR applicationTypeVersion,
        __in LPCWSTR serviceManifestName,
        __in const FABRIC_PACKAGE_SHARING_POLICY_LIST * sharedPackages,
        __in LPCWSTR nodeName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            applicationTypeName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            applicationTypeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            applicationTypeVersion,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            applicationTypeVersion_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            serviceManifestName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            serviceManifestName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            nodeName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            nodeName_);
        if (FAILED(hr)) { return hr; }

        vector<PackageSharingPolicyQueryObject> sharingPolicy;

         if(sharedPackages != NULL)
        {
            for(ULONG i = 0; i < sharedPackages->Count; i++)
            {
                wstring packageName;
                if(sharedPackages->Items[i].PackageName != nullptr)
                {

                    hr = StringUtility::LpcwstrToWstring(
                            sharedPackages->Items[i].PackageName,
                            false,
                            ParameterValidator::MinStringSize,
                            ParameterValidator::MaxFilePathSize,
                            packageName);
                    if (FAILED(hr)) { return hr; }
                }

                ServicePackageSharingType::Enum packageSharingType;
                auto error = ServicePackageSharingType::FromPublic(
                    sharedPackages->Items[i].Scope,
                    packageSharingType);
                if(!error.IsSuccess())
                {
                    return error.ToHResult();
                }
                PackageSharingPolicyQueryObject packageSharingPolicy(packageName, packageSharingType);
                sharingPolicy.push_back(move(packageSharingPolicy));
            }
        }

        PackageSharingPolicyList policyList(sharingPolicy);

        auto error = JsonHelper::Serialize<PackageSharingPolicyList>(policyList, packageSharingPolicies_);
        if(!error.IsSuccess())
        {
            return error.ToHResult();
        }
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeployServicePackageToNodeOperation> thisOperation(context, CLSID_ComFabricClient_DeployServicePackageToNodeOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }
protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginDeployServicePackageToNode(
            applicationTypeName_,
            applicationTypeVersion_,
            serviceManifestName_,
            packageSharingPolicies_,
            nodeName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeployServicePackageCompleted(operation);
            },
            proxySPtr);
    }

private:

    void OnDeployServicePackageCompleted(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.appMgmtClient_->EndDeployServicePackageToNode(operation);
        TryComplete(operation->Parent, error);
    }
private:
    wstring applicationTypeName_;
    wstring applicationTypeVersion_;
    wstring serviceManifestName_;
    wstring nodeName_;
    wstring packageSharingPolicies_;
};

// {df0f9add-a0d8-42cb-8354-6293f20e62f6}
static const GUID CLSID_ComFabricClient_DeActivateNodeAsyncOperation =
{ 0xdf0f9add, 0xa0d8, 0x42cb, { 0x83, 0x54, 0x62, 0x93, 0xf2, 0x0e, 0x62, 0xf6 } };

class ComFabricClient::DeActivateNodeAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeActivateNodeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeActivateNodeAsyncOperation,
        CLSID_ComFabricClient_DeActivateNodeAsyncOperation,
        DeActivateNodeAsyncOperation,
        ComAsyncOperationContext)
public:

    DeActivateNodeAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~DeActivateNodeAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR nodeName,
        __in FABRIC_NODE_DEACTIVATION_INTENT intent,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            nodeName,
            false, // dont accept null
            nodeName_);
        if (FAILED(hr)) { return hr; }

        intent_ = intent;
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeActivateNodeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_DeActivateNodeAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginDeactivateNode(
            nodeName_,
            intent_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeactivateNodeComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnDeactivateNodeComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndDeactivateNode(operation);
        TryComplete(operation->Parent, error);
    }

    std::wstring nodeName_;
    FABRIC_NODE_DEACTIVATION_INTENT intent_;
};

// {1eb64380-a2e8-4a38-b8f1-893af2ce605a}
static const GUID CLSID_ComFabricClient_ActivateNodeAsyncOperation =
{ 0x1eb64380, 0xa2e8, 0x4a38, { 0xb8, 0xf1, 0x89, 0x3a, 0xf2, 0xce, 0x60, 0x5a } };

class ComFabricClient::ActivateNodeAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(ActivateNodeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ActivateNodeAsyncOperation,
        CLSID_ComFabricClient_ActivateNodeAsyncOperation,
        ActivateNodeAsyncOperation,
        ComAsyncOperationContext)
public:

    ActivateNodeAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ActivateNodeAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR nodeName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            nodeName,
            false, // dont accept null
            nodeName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ActivateNodeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ActivateNodeAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginActivateNode(
            nodeName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnActivateNodeComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnActivateNodeComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndActivateNode(operation);
        TryComplete(operation->Parent, error);
    }

    std::wstring nodeName_;
};

// {afdbdd58-6b00-4908-838f-cb856d91fa17}
static const GUID CLSID_ComFabricClient_ProvisionFabricAsyncOperation =
{ 0xafdbdd58, 0x6b00, 0x4908, { 0x83, 0x8f, 0xcb, 0x85, 0x6d, 0x91, 0xfa, 0x17 } };

class ComFabricClient::ProvisionFabricAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(ProvisionFabricAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ProvisionFabricAsyncOperation,
        CLSID_ComFabricClient_ProvisionFabricAsyncOperation,
        ProvisionFabricAsyncOperation,
        ComAsyncOperationContext)
public:

    ProvisionFabricAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ProvisionFabricAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR codeFilepath,
        __in LPCWSTR clusterManifestFilepath,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        HRESULT hrValidate = StringUtility::LpcwstrToWstring(
            codeFilepath,
            true /*acceptNull*/,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            codePath_);
        if (FAILED(hrValidate)) { return ComUtility::OnPublicApiReturn(hrValidate); }

        hrValidate = StringUtility::LpcwstrToWstring(
            clusterManifestFilepath,
            true /*acceptNull*/,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            clusterManifestPath_);
        if (FAILED(hrValidate)) { return ComUtility::OnPublicApiReturn(hrValidate); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ProvisionFabricAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ProvisionFabricAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginProvisionFabric(
            codePath_,
            clusterManifestPath_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnProvisionFabricComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnProvisionFabricComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndProvisionFabric(operation);
        TryComplete(operation->Parent, error);
    }

    wstring codePath_;
    wstring clusterManifestPath_;
};

// {e27b242c-8f9f-4ea2-a6a3-2be2b7562c4a}
static const GUID CLSID_ComFabricClient_UpgradeFabricAsyncOperation =
{ 0xe27b242c, 0x8f9f, 0x4ea2, { 0xa6, 0xa3, 0x2b, 0xe2, 0xb7, 0x56, 0x2c, 0x4a } };

class ComFabricClient::UpgradeFabricAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpgradeFabricAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpgradeFabricAsyncOperation,
        CLSID_ComFabricClient_UpgradeFabricAsyncOperation,
        UpgradeFabricAsyncOperation,
        ComAsyncOperationContext)
public:

    UpgradeFabricAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpgradeFabricAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_UPGRADE_DESCRIPTION *upgradeDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto err = upgradeDescription_.FromPublicApi(*upgradeDescription);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpgradeFabricAsyncOperation> thisOperation(context, CLSID_ComFabricClient_UpgradeFabricAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginUpgradeFabric(
            upgradeDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUpgradeFabricComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUpgradeFabricComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndUpgradeFabric(operation);
        TryComplete(operation->Parent, error);
    }

    ServiceModel::FabricUpgradeDescriptionWrapper upgradeDescription_;
};

// {6aaa030f-06ab-47c9-b444-1a330c151791}
static const GUID CLSID_ComFabricClient_UpdateFabricUpgradeAsyncOperation =
{ 0x6aaa030f, 0x06ab, 0x47c9, { 0xb4, 0x44, 0x1a, 0x33, 0x0c, 0x15, 0x17, 0x91 } };

class ComFabricClient::UpdateFabricUpgradeAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpdateFabricUpgradeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpdateFabricUpgradeAsyncOperation,
        CLSID_ComFabricClient_UpdateFabricUpgradeAsyncOperation,
        UpdateFabricUpgradeAsyncOperation,
        ComAsyncOperationContext)
public:

    UpdateFabricUpgradeAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpdateFabricUpgradeAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_UPGRADE_UPDATE_DESCRIPTION * publicDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        publicDescription_ = publicDescription;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpdateFabricUpgradeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_UpdateFabricUpgradeAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        // Convert from public here instead of during Begin() so that
        // we can surface any error detail messages that might result
        // from the attempted conversion.
        //
        auto error = internalDescription_.FromPublicApi(*publicDescription_);
        if (error.IsSuccess())
        {
            Owner.clusterMgmtClient_->BeginUpdateFabricUpgrade(
                internalDescription_,
                Timeout,
                [this](AsyncOperationSPtr const & operation)
                {
                    this->OnUpdateComplete(operation);
                },
                proxySPtr);
        }
        else
        {
            this->TryComplete(proxySPtr, error);
        }
    }

private:
    void OnUpdateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndUpdateFabricUpgrade(operation);
        TryComplete(operation->Parent, error);
    }

    FABRIC_UPGRADE_UPDATE_DESCRIPTION const * publicDescription_;
    FabricUpgradeUpdateDescription internalDescription_;
};

// {bdb9d761-0cb6-44c2-9ecb-21a0eae497fb}
static const GUID CLSID_ComFabricClient_RollbackFabricUpgradeAsyncOperation =
{ 0xbdb9d761, 0x0cb6, 0x44c2, { 0x9e, 0xcb, 0x21, 0xa0, 0xea, 0xe4, 0x97, 0xfb } };

class ComFabricClient::RollbackFabricUpgradeAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RollbackFabricUpgradeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RollbackFabricUpgradeAsyncOperation,
        CLSID_ComFabricClient_RollbackFabricUpgradeAsyncOperation,
        RollbackFabricUpgradeAsyncOperation,
        ComAsyncOperationContext)
public:

    RollbackFabricUpgradeAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~RollbackFabricUpgradeAsyncOperation() {};

    static HRESULT Begin(
        ComFabricClient & owner,
        DWORD timeoutMilliseconds,
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        if (callback == NULL || context == NULL)
        {
            return E_POINTER;
        }

        auto operation = make_com<RollbackFabricUpgradeAsyncOperation>(owner);

        auto hr = operation->Initialize(
            timeoutMilliseconds,
            callback);
        if (FAILED(hr)) { return hr; }

        return ComAsyncOperationContext::StartAndDetach(move(operation), context);
    }


    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RollbackFabricUpgradeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RollbackFabricUpgradeAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginRollbackFabricUpgrade(
            Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnOperationComplete(operation); },
            proxySPtr);
    }

private:
    void OnOperationComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndRollbackFabricUpgrade(operation);
        TryComplete(operation->Parent, error);
    }
};

// {2927e2f1-2511-4703-b7bb-1165aa57bdbb}
static const GUID CLSID_ComFabricClient_GetFabricUpgradeProgressAsyncOperation =
{ 0x2927e2f1, 0x2511, 0x4703, { 0xb7, 0xbb, 0x11, 0x65, 0xaa, 0x57, 0xbd, 0xbb } };

class ComFabricClient::GetFabricUpgradeProgressAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetFabricUpgradeProgressAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetFabricUpgradeProgressAsyncOperation,
        CLSID_ComFabricClient_GetFabricUpgradeProgressAsyncOperation,
        GetFabricUpgradeProgressAsyncOperation,
        ComAsyncOperationContext)
public:

    GetFabricUpgradeProgressAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetFabricUpgradeProgressAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, REFIID riid, void **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetFabricUpgradeProgressAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetFabricUpgradeProgressAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricUpgradeProgressResult>(thisOperation->upgradeProgressResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginGetFabricUpgradeProgress(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetUpgradeProgressComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnGetUpgradeProgressComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndGetFabricUpgradeProgress(operation, upgradeProgressResultPtr_);
        TryComplete(operation->Parent, error);
    }

    IUpgradeProgressResultPtr upgradeProgressResultPtr_;
};

// {c74b53ef-5b61-4d5d-89c2-7423ae115e0c}
static const GUID CLSID_ComFabricClient_MoveNextUpgradeDomainAsyncOperation =
{ 0xc74b53ef, 0x5b61, 0x4d5d, { 0x89, 0xc2, 0x74, 0x23, 0xae, 0x11, 0x5e, 0x0c } };

class ComFabricClient::MoveNextUpgradeDomainAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(MoveNextUpgradeDomainAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        MoveNextUpgradeDomainAsyncOperation,
        CLSID_ComFabricClient_MoveNextUpgradeDomainAsyncOperation,
        MoveNextUpgradeDomainAsyncOperation,
        ComAsyncOperationContext)
public:

    MoveNextUpgradeDomainAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~MoveNextUpgradeDomainAsyncOperation() {};

    HRESULT Initialize(
        __in IFabricUpgradeProgressResult2 *upgradeProgress,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        Common::ComPointer<ComFabricUpgradeProgressResult> comUpdateProgress(upgradeProgress, CLSID_ComFabricUpgradeProgressResult);
        prevProgressResultPtr_ = comUpdateProgress->get_Impl();

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<MoveNextUpgradeDomainAsyncOperation> thisOperation(context, CLSID_ComFabricClient_MoveNextUpgradeDomainAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginMoveNextFabricUpgradeDomain(
            prevProgressResultPtr_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnMoveNextDomainComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnMoveNextDomainComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndMoveNextFabricUpgradeDomain(operation);
        TryComplete(operation->Parent, error);
    }

    IUpgradeProgressResultPtr prevProgressResultPtr_;
};

// {099e254f-cbb4-415c-bbb4-2cae792eb9eb}
static const GUID CLSID_ComFabricClient_MoveNextUpgradeDomainAsyncOperation2 =
{ 0x099e254f, 0xcbb4, 0x415c, { 0xbb, 0xb4, 0x2c, 0xae, 0x79, 0x2e, 0xb9, 0xeb } };

class ComFabricClient::MoveNextUpgradeDomainAsyncOperation2 :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(MoveNextUpgradeDomainAsyncOperation2)

    COM_INTERFACE_AND_DELEGATE_LIST(
        MoveNextUpgradeDomainAsyncOperation2,
        CLSID_ComFabricClient_MoveNextUpgradeDomainAsyncOperation2,
        MoveNextUpgradeDomainAsyncOperation2,
        ComAsyncOperationContext)
public:

    MoveNextUpgradeDomainAsyncOperation2(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~MoveNextUpgradeDomainAsyncOperation2() {};

    HRESULT Initialize(
        __in LPCWSTR nextDomain,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            nextDomain,
            false /*acceptNull*/,
            nextDomain_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<MoveNextUpgradeDomainAsyncOperation2> thisOperation(context, CLSID_ComFabricClient_MoveNextUpgradeDomainAsyncOperation2);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginMoveNextFabricUpgradeDomain2(
            nextDomain_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnMoveNextUpgradeDomainComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnMoveNextUpgradeDomainComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndMoveNextFabricUpgradeDomain2(operation);
        TryComplete(operation->Parent, error);
    }

    wstring nextDomain_;
};

// {06ce9621-56f7-4b27-967f-76cd0c682c3f}
static const GUID CLSID_ComFabricClient_StartInfrastructureTaskAsyncOperation =
{ 0x06ce9621, 0x56f7, 0x4b27, { 0x96, 0x7f, 0x76, 0xcd, 0x0c, 0x68, 0x2c, 0x3f } };

class ComFabricClient::StartInfrastructureTaskAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(StartInfrastructureTaskAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        StartInfrastructureTaskAsyncOperation,
        CLSID_ComFabricClient_StartInfrastructureTaskAsyncOperation,
        StartInfrastructureTaskAsyncOperation,
        ComAsyncOperationContext)
public:

    StartInfrastructureTaskAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , isComplete_(false)
    {
    }

    virtual ~StartInfrastructureTaskAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION const* description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode error = task_.FromPublicApi(*description);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        task_.UpdateDoAsyncAck(false);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, BOOLEAN * isComplete)
    {
        if (context == NULL || isComplete == NULL) { return E_POINTER; }

        ComPointer<StartInfrastructureTaskAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StartInfrastructureTaskAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *isComplete = (thisOperation->isComplete_ ? TRUE : FALSE);
        }

        return hr;
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginStartInfrastructureTask(
            task_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnStartInfrastructureTaskComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnStartInfrastructureTaskComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndStartInfrastructureTask(operation, isComplete_);
        TryComplete(operation->Parent, error);
    }

    Management::ClusterManager::InfrastructureTaskDescription task_;
    bool isComplete_;
};

// {0dba52c4-434b-4624-9c39-f9114d1b5f29}
static const GUID CLSID_ComFabricClient_FinishInfrastructureTaskAsyncOperation =
{ 0x0dba52c4, 0x434b, 0x4624, { 0x9c, 0x39, 0xf9, 0x11, 0x4d, 0x1b, 0x5f, 0x29 } };

class ComFabricClient::FinishInfrastructureTaskAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(FinishInfrastructureTaskAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        FinishInfrastructureTaskAsyncOperation,
        CLSID_ComFabricClient_FinishInfrastructureTaskAsyncOperation,
        FinishInfrastructureTaskAsyncOperation,
        ComAsyncOperationContext)
public:

    FinishInfrastructureTaskAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , isComplete_(false)
    {
    }

    virtual ~FinishInfrastructureTaskAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR taskId,
        __in ULONGLONG instanceId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        wstring parsedTaskId;
        hr = StringUtility::LpcwstrToWstring(
            taskId,
            false /*acceptNull*/,
            parsedTaskId);
        if (FAILED(hr)) { return hr; }

        taskInstance_ = Management::ClusterManager::TaskInstance(parsedTaskId, instanceId);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, BOOLEAN *isComplete)
    {
        if (context == NULL || isComplete == NULL) { return E_POINTER; }
        ComPointer<FinishInfrastructureTaskAsyncOperation> thisOperation(context, CLSID_ComFabricClient_FinishInfrastructureTaskAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *isComplete = (thisOperation->isComplete_ ? TRUE : FALSE);
        }

        return hr;
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginFinishInfrastructureTask(
            taskInstance_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnFinishInfrastructureTaskComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnFinishInfrastructureTaskComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndFinishInfrastructureTask(operation, isComplete_);
        TryComplete(operation->Parent, error);
    }

    Management::ClusterManager::TaskInstance taskInstance_;
    bool isComplete_;
};

// {2927e2f1-2511-4703-b7bb-1165aa57bdbb}
static const GUID CLSID_ComFabricClient_UnprovisionFabricAsyncOperation =
{ 0x2927e2f1, 0x2511, 0x4703, { 0xb7, 0xbb, 0x11, 0x65, 0xaa, 0x57, 0xbd, 0xbb } };

class ComFabricClient::UnprovisionFabricAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UnprovisionFabricAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UnprovisionFabricAsyncOperation,
        CLSID_ComFabricClient_UnprovisionFabricAsyncOperation,
        UnprovisionFabricAsyncOperation,
        ComAsyncOperationContext)
public:

    UnprovisionFabricAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , codeVersion_(FabricCodeVersion::Invalid)
        , configVersion_(FabricConfigVersion::Invalid)
    {
    }

    virtual ~UnprovisionFabricAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR codeVersion,
        __in LPCWSTR configVersion,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        if (codeVersion != NULL)
        {
            wstring codeVersionString;
            hr = StringUtility::LpcwstrToWstring(
                codeVersion,
                false /*acceptNull*/,
                codeVersionString);
            if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

            if (!FabricCodeVersion::TryParse(codeVersionString, codeVersion_))
            {
                return ComUtility::OnPublicApiReturn(E_INVALIDARG);
            }
        }

        if (configVersion != NULL)
        {
            wstring configVersionString;
            hr = StringUtility::LpcwstrToWstring(
                configVersion,
                false /*acceptNull*/,
                configVersionString);

            if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

            if (!FabricConfigVersion::TryParse(configVersionString, configVersion_))
            {
                return ComUtility::OnPublicApiReturn(E_INVALIDARG);
            }
        }
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UnprovisionFabricAsyncOperation> thisOperation(context, CLSID_ComFabricClient_UnprovisionFabricAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginUnprovisionFabric(
            codeVersion_,
            configVersion_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUnprovisionFabricComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUnprovisionFabricComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndUnprovisionFabric(operation);
        TryComplete(operation->Parent, error);
    }

    FabricCodeVersion codeVersion_;
    FabricConfigVersion configVersion_;
};

// {b625c5a5-c20c-4d2b-a553-dfa87cdf4a34}
static const GUID CLSID_ComFabricClient_GetClusterManifestAsyncOperation =
{ 0xb625c5a5, 0xc20c, 0x4d2b, { 0xa5, 0x53, 0xdf, 0xa8, 0x7c, 0xdf, 0x4a, 0x34 } };

class ComFabricClient::GetClusterManifestAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetClusterManifestAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetClusterManifestAsyncOperation,
        CLSID_ComFabricClient_GetClusterManifestAsyncOperation,
        GetClusterManifestAsyncOperation,
        ComAsyncOperationContext)
public:

    GetClusterManifestAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , clusterManifestResult_()
    {
    }

    virtual ~GetClusterManifestAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_CLUSTER_MANIFEST_QUERY_DESCRIPTION const * publicDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        if (publicDescription != NULL)
        {
            auto error = queryDescription_.FromPublicApi(*publicDescription);

            if (!error.IsSuccess()) { return error.ToHResult(); }
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricStringResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetClusterManifestAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetClusterManifestAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(
                thisOperation->clusterManifestResult_);
            *result = stringResult.DetachNoRelease();
        }

        return hr;
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginGetClusterManifest(
            queryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetClusterManifestComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetClusterManifestComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndGetClusterManifest(operation, clusterManifestResult_);
        TryComplete(operation->Parent, error);
    }

    ClusterManifestQueryDescription queryDescription_;
    wstring clusterManifestResult_;
};

// {6857cc45-db6c-4f37-8875-84b506d0931c}
static const GUID CLSID_ComFabricClient_NodeStateRemovedAsyncOperation =
{ 0x6857cc45, 0xdb6c, 0x4f37, {0x88, 0x75, 0x84, 0xb5, 0x06, 0xd0, 0x93, 0x1c} };

class ComFabricClient::NodeStateRemovedAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(NodeStateRemovedAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        NodeStateRemovedAsyncOperation,
        CLSID_ComFabricClient_NodeStateRemovedAsyncOperation,
        NodeStateRemovedAsyncOperation,
        ComAsyncOperationContext)
public:

    NodeStateRemovedAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~NodeStateRemovedAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR nodeName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            nodeName,
            false, // dont accept null
            nodeName_);

        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<NodeStateRemovedAsyncOperation> thisOperation(context, CLSID_ComFabricClient_NodeStateRemovedAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginNodeStateRemoved(
            nodeName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnNodeStateRemovedComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnNodeStateRemovedComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndNodeStateRemoved(operation);
        TryComplete(operation->Parent, error);
    }

    std::wstring nodeName_;
};

// {d45d01af-55c2-4b29-8b51-7eb9104a18d2}
static const GUID CLSID_ComFabricClient_RecoverPartitionsAsyncOperation =
{ 0xd45d01af, 0x55c2, 0x4b29, {0x8b, 0x51, 0x7e, 0xb9, 0x10, 0x4a, 0x18, 0xd2} };

class ComFabricClient::RecoverPartitionsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RecoverPartitionsAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RecoverPartitionsAsyncOperation,
        CLSID_ComFabricClient_RecoverPartitionsAsyncOperation,
        RecoverPartitionsAsyncOperation,
        ComAsyncOperationContext)
public:

    RecoverPartitionsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~RecoverPartitionsAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RecoverPartitionsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RecoverPartitionsAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginRecoverPartitions(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnRecoverPartitionsComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnRecoverPartitionsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndRecoverPartitions(operation);
        TryComplete(operation->Parent, error);
    }
};

// {A5A98900-C79F-40FA-ACBD-37A33904BBA6}
static const GUID CLSID_ComFabricClient_RecoverPartitionAsyncOperation =
{ 0xa5a98900, 0xc79f, 0x40fa, { 0xac, 0xbd, 0x37, 0xa3, 0x39, 0x4, 0xbb, 0xa6 } };

class ComFabricClient::RecoverPartitionAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RecoverPartitionAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RecoverPartitionAsyncOperation,
        CLSID_ComFabricClient_RecoverPartitionAsyncOperation,
        RecoverPartitionAsyncOperation,
        ComAsyncOperationContext)
public:

    RecoverPartitionAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~RecoverPartitionAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_PARTITION_ID partitionId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        partitionId_ = partitionId;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RecoverPartitionAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RecoverPartitionAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginRecoverPartition(
            Guid(partitionId_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnRecoverPartitionComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnRecoverPartitionComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndRecoverPartition(operation);
        TryComplete(operation->Parent, error);
    }

    FABRIC_PARTITION_ID partitionId_;
};

// {8C36B215-869C-4B1E-9B41-BAF99E2C65E8}
static const GUID CLSID_ComFabricClient_RecoverServicePartitionsAsyncOperation =
{ 0x8c36b215, 0x869c, 0x4b1e, { 0x9b, 0x41, 0xba, 0xf9, 0x9e, 0x2c, 0x65, 0xe8 } };


class ComFabricClient::RecoverServicePartitionsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RecoverServicePartitionsAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RecoverServicePartitionsAsyncOperation,
        CLSID_ComFabricClient_RecoverServicePartitionsAsyncOperation,
        RecoverServicePartitionsAsyncOperation,
        ComAsyncOperationContext)
public:

    RecoverServicePartitionsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~RecoverServicePartitionsAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI serviceName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        serviceName_ = wstring(serviceName);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RecoverServicePartitionsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RecoverServicePartitionsAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginRecoverServicePartitions(
            serviceName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnRecoverServicePartitionsComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnRecoverServicePartitionsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndRecoverServicePartitions(operation);
        TryComplete(operation->Parent, error);
    }

    wstring serviceName_;
};

// {9BB52971-6559-499C-A44D-83AA4966F27A}
static const GUID CLSID_ComFabricClient_RecoverSystemPartitionsAsyncOperation =
{ 0x9bb52971, 0x6559, 0x499c, { 0xa4, 0x4d, 0x83, 0xaa, 0x49, 0x66, 0xf2, 0x7a } };

class ComFabricClient::RecoverSystemPartitionsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RecoverSystemPartitionsAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RecoverSystemPartitionsAsyncOperation,
        CLSID_ComFabricClient_RecoverSystemPartitionsAsyncOperation,
        RecoverSystemPartitionsAsyncOperation,
        ComAsyncOperationContext)
public:

    RecoverSystemPartitionsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~RecoverSystemPartitionsAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RecoverSystemPartitionsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RecoverSystemPartitionsAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginRecoverSystemPartitions(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnRecoverSystemPartitionsComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnRecoverSystemPartitionsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndRecoverSystemPartitions(operation);
        TryComplete(operation->Parent, error);
    }
};

// {40fda629-d7f5-4fbb-aca7-23263f7be5d1}
static const GUID CLSID_ComFabricClient_CreateServiceOperation =
{ 0x40fda629, 0xd7f5, 0x4fbb, { 0xac, 0xa7, 0x23, 0x26, 0x3f, 0x7b, 0xe5, 0xd1 } };

class ComFabricClient::CreateServiceOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateServiceOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateServiceOperation,
        CLSID_ComFabricClient_CreateServiceOperation ,
        CreateServiceOperation,
        ComAsyncOperationContext)
public:

    CreateServiceOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , psd_()
    {
    }

    virtual ~CreateServiceOperation() {};

    HRESULT Initialize(
        const FABRIC_SERVICE_DESCRIPTION *description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = psd_.FromPublicApi(*description);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateServiceOperation> thisOperation(context, CLSID_ComFabricClient_CreateServiceOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginCreateService(
            psd_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnCreateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceMgmtClient_->EndCreateService(operation);
        TryComplete(operation->Parent, error);
    }

    PartitionedServiceDescWrapper psd_;
};

// {A5A98900-C79F-40FA-ACBD-37A33904BBA7}
static const GUID CLSID_ComFabricClient_ResetPartitionLoadAsyncOperation =
    { 0xa5a98900, 0xc79f, 0x40fa, { 0xac, 0xbd, 0x37, 0xa3, 0x39, 0x4, 0xbb, 0xa7 } };

class ComFabricClient::ResetPartitionLoadAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
    {
        DENY_COPY(ResetPartitionLoadAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
        ResetPartitionLoadAsyncOperation,
        CLSID_ComFabricClient_ResetPartitionLoadAsyncOperation,
        ResetPartitionLoadAsyncOperation,
        ComAsyncOperationContext)
    public:

        ResetPartitionLoadAsyncOperation(ComFabricClient& owner)
            : ClientAsyncOperation(owner)
        {
        }

        virtual ~ResetPartitionLoadAsyncOperation() {};

        HRESULT Initialize(
            __in FABRIC_PARTITION_ID partitionId,
            __in DWORD timeoutMilliSeconds,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
            if (FAILED(hr)) { return hr; }

            partitionId_ = partitionId;

            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<ResetPartitionLoadAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ResetPartitionLoadAsyncOperation);
            return thisOperation->ComAsyncOperationContextEnd();
        }

        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            Owner.clusterMgmtClient_->BeginResetPartitionLoad(
                Guid(partitionId_),
                Timeout,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnResetPartitionLoadComplete(operation);
            },
                proxySPtr);
        }

    private:
        void OnResetPartitionLoadComplete(__in AsyncOperationSPtr const& operation)
        {
            auto error = Owner.clusterMgmtClient_->EndResetPartitionLoad(operation);
            TryComplete(operation->Parent, error);
        }

        FABRIC_PARTITION_ID partitionId_;
    };

// {98E82ABC-89C4-4B3D-AC70-C203A0F2596B}
static const GUID CLSID_ComFabricClient_ToggleVerboseServicePlacementHealthReportingAsyncOperation =
{ 0x98e82abc, 0x89c4, 0x4b3d, { 0xac, 0x70, 0xc2, 0x3, 0xa0, 0xf2, 0x59, 0x6b } };
class ComFabricClient::ToggleVerboseServicePlacementHealthReportingAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
    {
        DENY_COPY(ToggleVerboseServicePlacementHealthReportingAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
        ToggleVerboseServicePlacementHealthReportingAsyncOperation,
        CLSID_ComFabricClient_ToggleVerboseServicePlacementHealthReportingAsyncOperation,
        ToggleVerboseServicePlacementHealthReportingAsyncOperation,
        ComAsyncOperationContext)
    public:

        ToggleVerboseServicePlacementHealthReportingAsyncOperation(ComFabricClient& owner)
            : ClientAsyncOperation(owner)
        {
        }

        virtual ~ToggleVerboseServicePlacementHealthReportingAsyncOperation() {};

        HRESULT Initialize(
            __in BOOLEAN enabled,
            __in DWORD timeoutMilliSeconds,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
            if (FAILED(hr)) { return hr; }

            enabled_ = enabled;

            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<ToggleVerboseServicePlacementHealthReportingAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ToggleVerboseServicePlacementHealthReportingAsyncOperation);
            return thisOperation->ComAsyncOperationContextEnd();
        }

        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            Owner.clusterMgmtClient_->BeginToggleVerboseServicePlacementHealthReporting(
                (enabled_ != 0),
                Timeout,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnToggleVerboseServicePlacementHealthReportingComplete(operation);
            },
                proxySPtr);
        }

    private:
        void OnToggleVerboseServicePlacementHealthReportingComplete(__in AsyncOperationSPtr const& operation)
        {
            auto error = Owner.clusterMgmtClient_->EndToggleVerboseServicePlacementHealthReporting(operation);
            TryComplete(operation->Parent, error);
        }

        BOOLEAN enabled_;
    };

// {C5B93A86-7E45-4375-8CA0-AEA2695A8F19}
static const GUID CLSID_ComFabricClient_GetUnplacedReplicaInformationOperation =
{ 0xc5b93a86, 0x7e45, 0x4375, { 0x8c, 0xa0, 0xae, 0xa2, 0x69, 0x5a, 0x8f, 0x19 } };
class ComFabricClient::GetUnplacedReplicaInformationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetUnplacedReplicaInformationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetUnplacedReplicaInformationOperation,
        CLSID_ComFabricClient_GetUnplacedReplicaInformationOperation,
        GetUnplacedReplicaInformationOperation,
        ComAsyncOperationContext)

public:
  GetUnplacedReplicaInformationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        serviceName_(),
        partitionId_(),
        UnplacedReplicaInformation_()
    {
    }

    virtual ~GetUnplacedReplicaInformationOperation() {};

    HRESULT Initialize(
        __in const FABRIC_UNPLACED_REPLICA_INFORMATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        serviceName_ = queryDescription->ServiceName;
        partitionId_ = Guid(queryDescription->PartitionId);
        onlyQueryPrimaries_ = queryDescription->OnlyQueryPrimaries;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetUnplacedReplicaInformationResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetUnplacedReplicaInformationOperation> thisOperation(context, CLSID_ComFabricClient_GetUnplacedReplicaInformationOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetUnplacedReplicaInformationResult> unplacedReplicaInformationResult =
                make_com<ComGetUnplacedReplicaInformationResult, IFabricGetUnplacedReplicaInformationResult>(move(thisOperation->UnplacedReplicaInformation_));
            *result = unplacedReplicaInformationResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetUnplacedReplicaInformation(
            serviceName_,
            partitionId_,
            onlyQueryPrimaries_ != 0,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetUnplacedReplicaInformationComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetUnplacedReplicaInformationComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetUnplacedReplicaInformation(operation, UnplacedReplicaInformation_);
        TryComplete(operation->Parent, error);
    }

private:
    std::wstring serviceName_;
    Guid partitionId_;
    BOOLEAN onlyQueryPrimaries_;

    UnplacedReplicaInformationQueryResult UnplacedReplicaInformation_;
};




// {6abf2f7c-67f7-4a7c-be66-882fd9b7755d}
static const GUID CLSID_ComFabricClient_CreateServiceFromTemplateOperation =
{ 0x6abf8f7c, 0x67f7, 0x4a7c, { 0xbe, 0x66, 0x88, 0x2f, 0xd9, 0xb7, 0x75, 0x5d } };

class ComFabricClient::CreateServiceFromTemplateOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateServiceFromTemplateOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateServiceFromTemplateOperation,
        CLSID_ComFabricClient_CreateServiceFromTemplateOperation ,
        CreateServiceFromTemplateOperation,
        ComAsyncOperationContext)
public:

    CreateServiceFromTemplateOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , svcFromTemplateDesc_()
    {
    }

    virtual ~CreateServiceFromTemplateOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_FROM_TEMPLATE_DESCRIPTION *serviceFromTemplateDesc,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = svcFromTemplateDesc_.FromPublicApi(*serviceFromTemplateDesc);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    HRESULT Initialize(
        __in FABRIC_URI appName,
        __in FABRIC_URI serviceName,
        __in LPCWSTR serviceTypeName,
        __in ULONG initializationDataSize,
        __in BYTE *initializationData,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto err = svcFromTemplateDesc_.FromPublicApi(appName, serviceName, serviceTypeName, initializationDataSize, initializationData);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateServiceFromTemplateOperation> thisOperation(context, CLSID_ComFabricClient_CreateServiceFromTemplateOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginCreateServiceFromTemplate(
            svcFromTemplateDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnCreateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceMgmtClient_->EndCreateServiceFromTemplate(operation);
        TryComplete(operation->Parent, error);
    }

    ServiceFromTemplateDescription svcFromTemplateDesc_;
};

// {42d10e8d-075a-4305-8304-96b021f54aaa}
static const GUID CLSID_ComFabricClient_UpdateServiceOperation =
{ 0x42d10e8d, 0x075a, 0x4305, { 0x83, 0x04, 0x96, 0xb0, 0x21, 0xf5, 0x4a, 0xaa } };

class ComFabricClient::UpdateServiceOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpdateServiceOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpdateServiceOperation,
        CLSID_ComFabricClient_UpdateServiceOperation ,
        UpdateServiceOperation,
        ComAsyncOperationContext)
public:

    UpdateServiceOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpdateServiceOperation() {};

    HRESULT Initialize(
        __in LPCWSTR serviceName,
        __in ServiceUpdateDescription && updateDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        updateDescription_ = move(updateDescription);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpdateServiceOperation> thisOperation(context, CLSID_ComFabricClient_UpdateServiceOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginUpdateService(
            serviceName_,
            updateDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUpdateComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUpdateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceMgmtClient_->EndUpdateService(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri serviceName_;
    ServiceUpdateDescription updateDescription_;
};

// {89687171-1c64-45c9-9291-51a12ec5dfe6}
static const GUID CLSID_ComFabricClient_DeleteServiceOperation =
{ 0x8967171, 0x1c64, 0x45c9, { 0x92, 0x91, 0x51, 0xa1, 0x2e, 0xc5, 0xdf, 0xe6 } };

class ComFabricClient::DeleteServiceOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeleteServiceOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeleteServiceOperation,
        CLSID_ComFabricClient_DeleteServiceOperation,
        DeleteServiceOperation,
        ComAsyncOperationContext)
public:

    static HRESULT BeginOperation(
        ComFabricClient & owner,
        const FABRIC_DELETE_SERVICE_DESCRIPTION * description,
        DWORD timeoutMilliSeconds,
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        if (context == NULL || description == NULL || description->ServiceName == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComPointer<DeleteServiceOperation> operation = make_com<DeleteServiceOperation>(owner);

        HRESULT hr = operation->Initialize(
            description,
            timeoutMilliSeconds,
            callback);

        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
    }

    DeleteServiceOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , description_()
    {
    }

    virtual ~DeleteServiceOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI serviceName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        NamingUri serviceNameUri;
        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceNameUri);
        description_.ServiceName = serviceNameUri;
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    HRESULT Initialize(
        __in const FABRIC_DELETE_SERVICE_DESCRIPTION * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = description_.FromPublicApi(*description);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeleteServiceOperation> thisOperation(context, CLSID_ComFabricClient_DeleteServiceOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginDeleteService(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeleteComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnDeleteComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceMgmtClient_->EndDeleteService(operation);
        TryComplete(operation->Parent, error);
    }

    DeleteServiceDescription description_;
};

// {867be7b5-6f86-461c-a54b-748512176aca}
static const GUID CLSID_ComFabricClient_GetServiceDescriptionOperation =
{ 0x867be7b5, 0x6f86, 0x461c, { 0xa5, 0x4b, 0x74, 0x85, 0x12, 0x17, 0x6a, 0xca } };

class ComFabricClient::GetServiceDescriptionOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceDescriptionOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetServiceDescriptionOperation,
        CLSID_ComFabricClient_GetServiceDescriptionOperation,
        GetServiceDescriptionOperation,
        ComAsyncOperationContext)
public:

    GetServiceDescriptionOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetServiceDescriptionOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI serviceName,
        bool fetchCached,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        fetchCached_ = fetchCached;
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricServiceDescriptionResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceDescriptionOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceDescriptionOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            thisOperation->description_->QueryInterface(IID_IFabricServiceDescriptionResult, reinterpret_cast<void**>(result));
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        if (fetchCached_)
        {
            this->FetchFromCache(proxySPtr);
        }
        else
        {
            this->FetchFromNamingService(proxySPtr);
        }
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        Naming::PartitionedServiceDescriptor psd;
        auto error = Owner.serviceMgmtClient_->EndGetServiceDescription(operation, psd);

        if (error.IsSuccess())
        {
            description_ = make_com<ComServiceDescriptionResult, IFabricServiceDescriptionResult>(move(psd));
        }

        TryComplete(operation->Parent, move(error));
    }

    void OnCompleteFromCache(__in AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        Naming::PartitionedServiceDescriptor psd;
        auto error = Owner.serviceMgmtClient_->EndGetCachedServiceDescription(operation, psd);

        if (error.IsSuccess())
        {
            description_ = make_com<ComServiceDescriptionResult, IFabricServiceDescriptionResult>(move(psd));
        }

        TryComplete(operation->Parent, move(error));
    }

    void FetchFromCache(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginGetCachedServiceDescription(
            serviceName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCompleteFromCache(operation, false);
        },
            proxySPtr);
        this->OnCompleteFromCache(operation, true);
    }

    void FetchFromNamingService(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginGetServiceDescription(
            serviceName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation, false);
        },
            proxySPtr);
        this->OnComplete(operation, true);
    }

    NamingUri serviceName_;
    Common::ComPointer<IFabricServiceDescriptionResult> description_;
    bool fetchCached_;
};

// {abb25877-1fed-46ac-9ba7-7e9434a9f926}
static const GUID CLSID_ComFabricClient_ResolveServiceOperation =
{ 0xabb25877, 0x1fed, 0x46ac, { 0x9b, 0xa7, 0x7e, 0x94, 0x34, 0xa9, 0xf9, 0x26 } };

class ComFabricClient::ResolveServiceOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(ResolveServiceOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ResolveServiceOperation,
        CLSID_ComFabricClient_ResolveServiceOperation,
        ResolveServiceOperation,
        ComAsyncOperationContext)
public:

    ResolveServiceOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ResolveServiceOperation() {};

    HRESULT Initialize(
        __in LPCWSTR serviceName,
        __in FABRIC_PARTITION_KEY_TYPE partitionKeyType,
        __in void const * partitionKey,
        __in Common::ComPointer<ComResolvedServicePartitionResult> && previousResult,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        // Partition Key
        switch (partitionKeyType)
        {
        case FABRIC_PARTITION_KEY_TYPE_NONE:
            key_ = Naming::PartitionKey();
            break;
        case FABRIC_PARTITION_KEY_TYPE_INT64:
            if (partitionKey == NULL) { return E_POINTER; }
            key_ = Naming::PartitionKey(*(reinterpret_cast<const __int64*>(partitionKey)));
            break;
        case FABRIC_PARTITION_KEY_TYPE_STRING:
            {
                LPCWSTR partitionKeyPtr = reinterpret_cast<LPCWSTR>(partitionKey);
                wstring partitionKeyString;
                hr = StringUtility::LpcwstrToWstring(partitionKeyPtr, false, partitionKeyString);
                if (FAILED(hr)) { return hr; }
                key_ = Naming::PartitionKey(partitionKeyString);
                break;
            }
        default:
            return E_INVALIDARG;
        }

        if (previousResult)
        {
            previousResult_ = previousResult->get_Impl();
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricResolvedServicePartitionResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<ResolveServiceOperation> thisOperation(context, CLSID_ComFabricClient_ResolveServiceOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            thisOperation->result_->QueryInterface(IID_IFabricResolvedServicePartitionResult, reinterpret_cast<void**>(result));
        }
        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginResolveServicePartition(
            serviceName_,
            Naming::ServiceResolutionRequestData(key_),
            previousResult_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        IResolvedServicePartitionResultPtr resultPtr;
        auto error = Owner.serviceMgmtClient_->EndResolveServicePartition(operation, resultPtr);

        if (error.IsSuccess())
        {
            result_ = make_com<ComResolvedServicePartitionResult, IFabricResolvedServicePartitionResult>(resultPtr);
        }
        TryComplete(operation->Parent, error);
    }

    NamingUri serviceName_;
    Api::IResolvedServicePartitionResultPtr previousResult_;
    Common::ComPointer<IFabricResolvedServicePartitionResult> result_;
    Naming::PartitionKey key_;
};

// {e94ebfff-7c7f-4db5-9bd9-beb73695709c}
static const GUID CLSID_ComFabricClient_GetServiceManifestOperation =
{ 0xe94ebfff, 0x7c7f, 0x4db5, { 0x9b, 0xd9, 0xbe, 0xb7, 0x36, 0x95, 0x70, 0x9c } };

class ComFabricClient::GetServiceManifestOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceManifestOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetServiceManifestOperation,
        CLSID_ComFabricClient_GetServiceManifestOperation,
        GetServiceManifestOperation,
        ComAsyncOperationContext)
public:

    GetServiceManifestOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetServiceManifestOperation() {};

    HRESULT Initialize(
        __in LPCWSTR applicationTypeName,
        __in LPCWSTR applicationTypeVersion,
        __in LPCWSTR serviceManifestName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            applicationTypeName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            applicationTypeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            applicationTypeVersion,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            applicationTypeVersion_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            serviceManifestName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceManifestName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricStringResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceManifestOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceManifestOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
             Common::ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(
                thisOperation->serviceManifest_);
            *result = stringResult.DetachNoRelease();
        }
        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginGetServiceManifest(
            applicationTypeName_,
            applicationTypeVersion_,
            serviceManifestName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceMgmtClient_->EndGetServiceManifest(operation, serviceManifest_);
        TryComplete(operation->Parent, error);
    }

    wstring applicationTypeName_;
    wstring applicationTypeVersion_;
    wstring serviceManifestName_;
    wstring serviceManifest_;
};

// {1D80DB37-94A2-4163-9CE5-A0562B3B85D9}
static const GUID CLSID_ComFabricClient_ReportFaultAsyncOperation =
{ 0x1d80db37, 0x94a2, 0x4163, { 0x9c, 0xe5, 0xa0, 0x56, 0x2b, 0x3b, 0x85, 0xd9 } };

class ComFabricClient::ReportFaultAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(ReportFaultAsyncOperation);

    COM_INTERFACE_AND_DELEGATE_LIST(
        ReportFaultAsyncOperation,
        CLSID_ComFabricClient_ReportFaultAsyncOperation,
        ReportFaultAsyncOperation,
        ComAsyncOperationContext)

    template<typename T>
    static bool GetIsForce(T const & description)
    {
        static_assert(false, "must use specialization");
    }

    template<>
    static bool GetIsForce<FABRIC_REMOVE_REPLICA_DESCRIPTION>(FABRIC_REMOVE_REPLICA_DESCRIPTION const & desc)
    {
        if (desc.Reserved == nullptr)
        {
            return false;
        }

        FABRIC_REMOVE_REPLICA_DESCRIPTION_EX1 * casted = static_cast<FABRIC_REMOVE_REPLICA_DESCRIPTION_EX1*>(desc.Reserved);
        return casted->ForceRemove ? true : false;
    }

    template<>
    static bool GetIsForce<FABRIC_RESTART_REPLICA_DESCRIPTION>(FABRIC_RESTART_REPLICA_DESCRIPTION const &)
    {
        return false;
    }

public:

    template<typename T>
    static HRESULT BeginOperation(
        ComFabricClient & owner,
        const T * description,
        FABRIC_FAULT_TYPE faultType,
        DWORD timeoutMilliseconds,
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        if ((context == NULL) || (description == NULL) || (description->NodeName == NULL))
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComPointer<ReportFaultAsyncOperation> operation = make_com<ReportFaultAsyncOperation>(owner);

        HRESULT hr = operation->Initialize(
            description->NodeName,
            description->PartitionId,
            description->ReplicaOrInstanceId,
            faultType,
            GetIsForce(*description),
            timeoutMilliseconds,
            callback);

        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
    }

    ReportFaultAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ReportFaultAsyncOperation()
    {
    }

    HRESULT Initialize(
        __in LPCWSTR nodeName,
        __in FABRIC_PARTITION_ID partitionId,
        __in FABRIC_REPLICA_ID replicaOrInstanceId,
        __in FABRIC_FAULT_TYPE faultType,
        __in bool isForce,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliseconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            nodeName,
            false,
            nodeName_);
        if (FAILED(hr)) { return hr; }

        partitionId_ = Guid(partitionId);
        replicaId_ = replicaOrInstanceId;
        faultType_ = Reliability::FaultType::FromPublicAPI(faultType);
        isForce_ = isForce;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ReportFaultAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ReportFaultAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceMgmtClient_->BeginReportFault(
            nodeName_,
            partitionId_,
            replicaId_,
            faultType_,
            isForce_,
            Timeout,
            [this] (AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.ServiceMgmtClient->EndReportFault(operation);
        TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    Guid partitionId_;
    int64 replicaId_;
    Reliability::FaultType::Enum faultType_;
    bool isForce_;
};

// {5db132f3-70a8-4cc6-a9b8-28b3ee0c1064}
static const GUID CLSID_ComFabricClient_CreateServiceGroupAsyncOperation =
{ 0x5db132f3, 0x70a8, 0x4cc6, {0xa9, 0xb8, 0x28, 0xb3, 0xee, 0x0c, 0x10, 0x64} };

class ComFabricClient::CreateServiceGroupAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateServiceGroupAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateServiceGroupAsyncOperation,
        CLSID_ComFabricClient_CreateServiceGroupAsyncOperation ,
        CreateServiceGroupAsyncOperation,
        ComAsyncOperationContext)
public:

    CreateServiceGroupAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , psd_()
    {
    }

    virtual ~CreateServiceGroupAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_DESCRIPTION *description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        Naming::PartitionedServiceDescriptor psd;
        auto error = psd.FromPublicApi(*description);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        psd.ToWrapper(psd_);
        psd_.IsServiceGroup = true;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateServiceGroupAsyncOperation> thisOperation(context, CLSID_ComFabricClient_CreateServiceGroupAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceGroupMgmtClient_->BeginCreateServiceGroup(
            psd_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnCreateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceGroupMgmtClient_->EndCreateServiceGroup(operation);
        TryComplete(operation->Parent, error);
    }

    PartitionedServiceDescWrapper psd_;
};

// {5EAC131A-B25A-42BA-81A4-EF0D0F86F341}
static const GUID CLSID_ComFabricClient_CreateServiceGroupFromTemplateAsyncOperation =
{ 0x5eac131a, 0xb25a, 0x42ba, { 0x81, 0xa4, 0xef, 0xd, 0xf, 0x86, 0xf3, 0x41 } };

class ComFabricClient::CreateServiceGroupFromTemplateAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateServiceGroupFromTemplateAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateServiceGroupFromTemplateAsyncOperation,
        CLSID_ComFabricClient_CreateServiceGroupFromTemplateAsyncOperation,
        CreateServiceGroupFromTemplateAsyncOperation,
        ComAsyncOperationContext)
public:

    CreateServiceGroupFromTemplateAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~CreateServiceGroupFromTemplateAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_GROUP_FROM_TEMPLATE_DESCRIPTION *serviceGroupFromTemplateDesc,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = svcGroupFromTemplateDesc_.FromPublicApi(*serviceGroupFromTemplateDesc);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    HRESULT Initialize(
        __in FABRIC_URI appName,
        __in FABRIC_URI serviceName,
        __in LPCWSTR serviceTypeName,
        __in ULONG initializationDataSize,
        __in BYTE *initializationData,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto err = svcGroupFromTemplateDesc_.FromPublicApi(appName, serviceName, serviceTypeName, initializationDataSize, initializationData);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateServiceGroupFromTemplateAsyncOperation> thisOperation(context, CLSID_ComFabricClient_CreateServiceGroupFromTemplateAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceGroupMgmtClient_->BeginCreateServiceGroupFromTemplate(
            svcGroupFromTemplateDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnCreateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceGroupMgmtClient_->EndCreateServiceGroupFromTemplate(operation);
        TryComplete(operation->Parent, error);
    }

    ServiceGroupFromTemplateDescription svcGroupFromTemplateDesc_;
};

// {0cca1aae-4e33-4e33-866c-9ed4299daacf}
static const GUID CLSID_ComFabricClient_DeleteServiceGroupAsyncOperation =
{  0x0cca1aae, 0x4e33, 0x4e33, {0x86, 0x6c, 0x9e, 0xd4, 0x29, 0x9d, 0xaa, 0xcf} };

class ComFabricClient::DeleteServiceGroupAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeleteServiceGroupAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeleteServiceGroupAsyncOperation,
        CLSID_ComFabricClient_DeleteServiceGroupAsyncOperation,
        DeleteServiceGroupAsyncOperation,
        ComAsyncOperationContext)
public:

    DeleteServiceGroupAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~DeleteServiceGroupAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI serviceName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeleteServiceGroupAsyncOperation> thisOperation(context, CLSID_ComFabricClient_DeleteServiceGroupAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceGroupMgmtClient_->BeginDeleteServiceGroup(
            serviceName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeleteComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnDeleteComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceGroupMgmtClient_->EndDeleteServiceGroup(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri serviceName_;
};

// {15260f01-a844-4ece-9fe8-a7a1e1f3b0aa}
static const GUID CLSID_ComFabricClient_GetDescriptionAsyncOperation =
{ 0x15260f01, 0xa844, 0x4ece, {0x9f, 0xe8, 0xa7, 0xa1, 0xe1, 0xf3, 0xb0, 0xaa} };

class ComFabricClient::GetDescriptionAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDescriptionAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDescriptionAsyncOperation,
        CLSID_ComFabricClient_GetDescriptionAsyncOperation,
        GetDescriptionAsyncOperation,
        ComAsyncOperationContext)
public:

    GetDescriptionAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetDescriptionAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI serviceName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricServiceGroupDescriptionResult ** result)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetDescriptionAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetDescriptionAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<ComServiceGroupDescriptionResult> resultCPtr =
                make_com<ComServiceGroupDescriptionResult>(thisOperation->description_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceGroupMgmtClient_->BeginGetServiceGroupDescription(
            serviceName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetDescriptionComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnGetDescriptionComplete(AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceGroupMgmtClient_->EndGetServiceGroupDescription(operation, description_);
        TryComplete(operation->Parent, error);
    }

    NamingUri serviceName_;
    Naming::ServiceGroupDescriptor description_;
};

// {52c701db-5c7b-4dee-830e-24657a5f6bd4}
static const GUID CLSID_ComFabricClient_UpdateServiceGroupAsyncOperation =
{ 0x52c701db, 0x5c7b,  0x4dee, {0x83, 0x0e, 0x24, 0x65, 0x7a, 0x5f, 0x6b, 0xd4} };

class ComFabricClient::UpdateServiceGroupAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpdateServiceGroupAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpdateServiceGroupAsyncOperation,
        CLSID_ComFabricClient_UpdateServiceGroupAsyncOperation,
        UpdateServiceGroupAsyncOperation,
        ComAsyncOperationContext)
public:

    UpdateServiceGroupAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpdateServiceGroupAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI serviceName,
        __in const FABRIC_SERVICE_GROUP_UPDATE_DESCRIPTION *serviceGroupUpdateDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = updateDescription_.FromPublicApi(*serviceGroupUpdateDescription->Description);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpdateServiceGroupAsyncOperation> thisOperation(context, CLSID_ComFabricClient_UpdateServiceGroupAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.serviceGroupMgmtClient_->BeginUpdateServiceGroup(
            serviceName_,
            updateDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUpdateComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnUpdateComplete(AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceGroupMgmtClient_->EndUpdateServiceGroup(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri serviceName_;
    ServiceUpdateDescription updateDescription_;
};


// fd71d022-bd95-405c-bb26-52210382b499
static const GUID CLSID_ComFabricClient_CreateNameAsyncOperation =
{ 0xfd71d022, 0xbd95, 0x405c, {0xbb, 0x26, 0x52, 0x21, 0x03, 0x82, 0xb4, 0x99} };

class ComFabricClient::CreateNameAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateNameAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateNameAsyncOperation,
        CLSID_ComFabricClient_CreateNameAsyncOperation,
        CreateNameAsyncOperation,
        ComAsyncOperationContext)
public:

    CreateNameAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~CreateNameAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateNameAsyncOperation> thisOperation(context, CLSID_ComFabricClient_CreateNameAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginCreateName(
            name_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateNameComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnCreateNameComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndCreateName(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
};

// 86abe159-cf51-4d5b-b64a-9bf61ee3112d
static const GUID CLSID_ComFabricClient_DeleteNameAsyncOperation =
{ 0x86abe159, 0xcf51, 0x4d5b, {0xb6, 0x4a, 0x9b, 0xf6, 0x1e, 0xe3, 0x11, 0x2d} };

class ComFabricClient::DeleteNameAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeleteNameAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeleteNameAsyncOperation,
        CLSID_ComFabricClient_DeleteNameAsyncOperation,
        DeleteNameAsyncOperation,
        ComAsyncOperationContext)
public:

    DeleteNameAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~DeleteNameAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeleteNameAsyncOperation> thisOperation(context, CLSID_ComFabricClient_DeleteNameAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginDeleteName(
            name_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeleteNameComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnDeleteNameComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndDeleteName(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
};

// 69a2d97b-306a-483f-95f6-31b2fc291ead
static const GUID CLSID_ComFabricClient_NameExistsAsyncOperation =
{ 0x69a2d97b, 0x306a, 0x483f, {0x95, 0xf6, 0x31, 0xb2, 0xfc, 0x29, 0x1e, 0xad} };

class ComFabricClient::NameExistsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(NameExistsAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        NameExistsAsyncOperation,
        CLSID_ComFabricClient_NameExistsAsyncOperation,
        NameExistsAsyncOperation,
        ComAsyncOperationContext)
public:

    NameExistsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~NameExistsAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, BOOLEAN *exists)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<NameExistsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_NameExistsAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *exists = thisOperation->exists_;
        }
        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginNameExists(
            name_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnNameExistsComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnNameExistsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndNameExists(operation, exists_);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    bool exists_;
};

// efe22061-5ea6-46d9-bad6-21fbfb415dbe
static const GUID CLSID_ComFabricClient_EnumerateSubNamesAsyncOperation =
{ 0xefe22061, 0x5ea6, 0x46d9, {0xba, 0xd6, 0x21, 0xfb, 0xfb, 0x41, 0x5d, 0xbe} };

class ComFabricClient::EnumerateSubNamesAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(EnumerateSubNamesAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        EnumerateSubNamesAsyncOperation,
        CLSID_ComFabricClient_EnumerateSubNamesAsyncOperation,
        EnumerateSubNamesAsyncOperation,
        ComAsyncOperationContext)
public:

    EnumerateSubNamesAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~EnumerateSubNamesAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in ComPointer<ComNameEnumerationResult> prevResultCPtr,
        __in BOOLEAN recursive,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        recursive_ = recursive ? true : false;
        if (prevResultCPtr)
        {
            continuationToken_ = prevResultCPtr->get_ContinuationToken();
        }
        else
        {
            continuationToken_ = Naming::EnumerateSubNamesToken();
        }
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricNameEnumerationResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<EnumerateSubNamesAsyncOperation> thisOperation(context, CLSID_ComFabricClient_EnumerateSubNamesAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<ComNameEnumerationResult> resultCPtr = make_com<ComNameEnumerationResult>(move(thisOperation->enumResult_));
            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginEnumerateSubNames(
            name_,
            continuationToken_,
            recursive_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnEnumerateSubNamesComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnEnumerateSubNamesComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndEnumerateSubNames(operation, enumResult_);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    bool recursive_;
    Naming::EnumerateSubNamesToken continuationToken_;
    Naming::EnumerateSubNamesResult enumResult_;
};

// fb01abbe-4e8f-499e-9b0c-ee01af77331c
static const GUID CLSID_ComFabricClient_PutPropertyAsyncOperation =
{ 0xfb01abbe, 0x4e8f, 0x499e, {0x9b, 0x0c, 0xee, 0x01, 0xaf, 0x77, 0x33, 0x1c} };

class ComFabricClient::PutPropertyAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(PutPropertyAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        PutPropertyAsyncOperation,
        CLSID_ComFabricClient_PutPropertyAsyncOperation,
        PutPropertyAsyncOperation,
        ComAsyncOperationContext)
public:

    PutPropertyAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~PutPropertyAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in LPCWSTR propertyName,
        __in ULONG dataLength,
        __in BYTE const *data,
        __in FABRIC_PROPERTY_TYPE_ID propertyType,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            propertyName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            propertyName_);

        if (dataLength > ServiceModel::Constants::NamedPropertyMaxValueSize)
        {
            return FABRIC_E_VALUE_TOO_LARGE;
        }

        data_ = vector<byte>(data, data + dataLength);
        typeId_ = propertyType;
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<PutPropertyAsyncOperation> thisOperation(context, CLSID_ComFabricClient_PutPropertyAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginPutProperty(
            name_,
            propertyName_,
            typeId_,
            move(data_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnPutPropertyComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnPutPropertyComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndPutProperty(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    wstring propertyName_;
    ByteBuffer data_;
    FABRIC_PROPERTY_TYPE_ID typeId_;
};

// 15fedf5a-3447-4461-865c-0bb65a441992
static const GUID CLSID_ComFabricClient_DeletePropertyAsyncOperation =
{ 0x15fedf5a, 0x3447, 0x4461, {0x86, 0x5c, 0x0b, 0xb6, 0x5a, 0x44, 0x19, 0x92} };

class ComFabricClient::DeletePropertyAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeletePropertyAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeletePropertyAsyncOperation,
        CLSID_ComFabricClient_DeletePropertyAsyncOperation,
        DeletePropertyAsyncOperation,
        ComAsyncOperationContext)
public:

    DeletePropertyAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~DeletePropertyAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in LPCWSTR propertyName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            propertyName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            propertyName_);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeletePropertyAsyncOperation> thisOperation(context, CLSID_ComFabricClient_DeletePropertyAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginDeleteProperty(
            name_,
            propertyName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeletePropertyComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnDeletePropertyComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndDeleteProperty(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    wstring propertyName_;
};

// dd5e50bf-3ba3-4465-84f6-82e527e88205
static const GUID CLSID_ComFabricClient_GetPropertyAsyncOperation =
{ 0xdd5e50bf, 0x3ba3, 0x4465, {0x84, 0xf6, 0x82, 0xe5, 0x27, 0xe8, 0x82, 0x05} };

class ComFabricClient::GetPropertyAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetPropertyAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetPropertyAsyncOperation ,
        CLSID_ComFabricClient_GetPropertyAsyncOperation ,
        GetPropertyAsyncOperation ,
        ComAsyncOperationContext)
public:

    GetPropertyAsyncOperation (ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetPropertyAsyncOperation () {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in LPCWSTR propertyName,
        __in bool includeValues,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            propertyName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            propertyName_);
        if (FAILED(hr)) { return hr; }

        includeValues_ = includeValues;
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricPropertyValueResult **result)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetPropertyAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetPropertyAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComNamedPropertyCPtr resultCPtr = make_com<ComNamedProperty>(move(thisOperation->propertyResult_));
            *result = resultCPtr.DetachNoRelease();
        }
        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricPropertyMetadataResult **result)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetPropertyAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetPropertyAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComNamedPropertyMetadataCPtr resultCPtr = make_com<ComNamedPropertyMetadata>(thisOperation->metadataResult_);
            *result = resultCPtr.DetachNoRelease();
        }
        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        if (includeValues_)
        {
            Owner.propertyMgmtClient_->BeginGetProperty(
                name_,
                propertyName_,
                Timeout,
                [this](AsyncOperationSPtr const & operation)
                {
                    this->OnGetPropertyComplete(operation);
                },
                proxySPtr);
        }
        else
        {
            Owner.propertyMgmtClient_->BeginGetPropertyMetadata(
                name_,
                propertyName_,
                Timeout,
                [this](AsyncOperationSPtr const & operation)
                {
                    this->OnGetPropertyMetadataComplete(operation);
                },
                proxySPtr);
        }
    }

private:
    void OnGetPropertyComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndGetProperty(operation, propertyResult_);
        TryComplete(operation->Parent, error);
    }

    void OnGetPropertyMetadataComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndGetPropertyMetadata(operation, metadataResult_);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    wstring propertyName_;
    bool includeValues_;
    Naming::NamePropertyResult propertyResult_;
    Naming::NamePropertyMetadataResult metadataResult_;
};

// 6861bc54-f5b5-4678-9149-f2846cc55416
static const GUID CLSID_ComFabricClient_PropertyBatchAsyncOperation =
{ 0x6861bc54, 0xf5b5, 0x4678, {0x91, 0x49, 0xf2, 0x84, 0x6c, 0xc5, 0x54, 0x16} };

class ComFabricClient::PropertyBatchAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(PropertyBatchAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        PropertyBatchAsyncOperation,
        CLSID_ComFabricClient_PropertyBatchAsyncOperation,
        PropertyBatchAsyncOperation,
        ComAsyncOperationContext)
public:

    PropertyBatchAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~PropertyBatchAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in ULONG operationCount,
        __in const FABRIC_PROPERTY_BATCH_OPERATION * operations,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        batch_ = Naming::NamePropertyOperationBatch(name_);

        for (ULONG i = 0; i < operationCount; ++i)
        {
            hr = AddToBatch(operations[i]);
            if (FAILED(hr)) { return hr; }
        }

        return S_OK;
    }

    HRESULT AddToBatch(FABRIC_PROPERTY_BATCH_OPERATION const & operation)
    {
        HRESULT hr = S_OK;

        if (operation.Value == NULL)
        {
            return E_POINTER;
        }

        switch (operation.Kind)
        {
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT:
            {
                auto op = reinterpret_cast<FABRIC_PUT_PROPERTY_OPERATION*>(operation.Value);
                if (op == NULL) { return E_POINTER; }

                vector<byte> data;
                hr = GetOperationBytes(op->PropertyTypeId, op->PropertyValue, data);
                if (FAILED(hr)) { return hr; }

                batch_.AddPutPropertyOperation(
                    op->PropertyName,
                    move(data),
                    op->PropertyTypeId);
                break;
            }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET:
            {
                auto op = reinterpret_cast<FABRIC_GET_PROPERTY_OPERATION*>(operation.Value);
                if (op == NULL) { return E_POINTER; }

                batch_.AddGetPropertyOperation(op->PropertyName, op->IncludeValue ? true : false);
                break;
            }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS:
            {
                auto op = reinterpret_cast<FABRIC_CHECK_EXISTS_PROPERTY_OPERATION*>(operation.Value);
                if (op == NULL) { return E_POINTER; }

                batch_.AddCheckExistenceOperation(op->PropertyName, op->ExistenceCheck ? true : false);
                break;
            }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE:
            {
                auto op = reinterpret_cast<FABRIC_CHECK_SEQUENCE_PROPERTY_OPERATION*>(operation.Value);
                if (op == NULL) { return E_POINTER; }

                batch_.AddCheckSequenceOperation(op->PropertyName, op->SequenceNumber);
                break;
            }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE:
            {
                auto op = reinterpret_cast<FABRIC_DELETE_PROPERTY_OPERATION*>(operation.Value);
                if (op == NULL) { return E_POINTER; }

                batch_.AddDeletePropertyOperation(op->PropertyName);
                break;
            }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT_CUSTOM:
            {
                auto op = reinterpret_cast<FABRIC_PUT_CUSTOM_PROPERTY_OPERATION*>(operation.Value);
                if (op == NULL) { return E_POINTER; }

                switch (op->PropertyTypeId)
                {
                case FABRIC_PROPERTY_TYPE_BINARY:
                case FABRIC_PROPERTY_TYPE_INT64:
                case FABRIC_PROPERTY_TYPE_DOUBLE:
                case FABRIC_PROPERTY_TYPE_WSTRING:
                case FABRIC_PROPERTY_TYPE_GUID:
                    break;
                default:
                    return E_INVALIDARG;
                }

                vector<byte> data;
                hr = GetOperationBytes(op->PropertyTypeId, op->PropertyValue, data);
                if (FAILED(hr)) { return hr; }

                batch_.AddPutCustomPropertyOperation(
                    op->PropertyName,
                    move(data),
                    op->PropertyTypeId,
                    move(wstring(op->PropertyCustomTypeId)));

                break;
            }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE:
            {
                auto op = reinterpret_cast<FABRIC_CHECK_VALUE_PROPERTY_OPERATION*>(operation.Value);
                if ((op == NULL) || (op->PropertyValue == NULL)) { return E_POINTER; }

                switch (op->PropertyTypeId)
                {
                case FABRIC_PROPERTY_TYPE_BINARY:
                case FABRIC_PROPERTY_TYPE_INT64:
                case FABRIC_PROPERTY_TYPE_DOUBLE:
                case FABRIC_PROPERTY_TYPE_WSTRING:
                case FABRIC_PROPERTY_TYPE_GUID:
                    break;
                default:
                    return E_INVALIDARG;
                }

                vector<byte> data;
                hr = GetOperationBytes(op->PropertyTypeId, op->PropertyValue, data);
                if (FAILED(hr)) { return hr; }

                batch_.AddCheckValuePropertyOperation(
                    op->PropertyName,
                    move(data),
                    op->PropertyTypeId);
                break;
            }
            default:
            {
                return E_INVALIDARG;
            }
        }

        return hr;
    }

    static HRESULT GetOperationBytes(
        FABRIC_PROPERTY_TYPE_ID propertyTypeId,
        void * propertyValue,
        __out vector<byte> & bytes)
    {
        BYTE * data = NULL;
        ULONG dataLength = 0;

        switch (propertyTypeId)
        {
        case FABRIC_PROPERTY_TYPE_BINARY:
            {
                // PropertyValue is FABRIC_OPERATION_DATA_BUFFER *
                auto inner = reinterpret_cast<FABRIC_OPERATION_DATA_BUFFER *>(propertyValue);
                data = inner->Buffer;
                dataLength = inner->BufferSize;
            }
            break;
        case FABRIC_PROPERTY_TYPE_INT64:
            {
                // PropertyValue is LONGLONG*
                data = reinterpret_cast<BYTE *>(propertyValue);
                dataLength = static_cast<ULONG>(sizeof(LONGLONG));
            }
            break;
        case FABRIC_PROPERTY_TYPE_DOUBLE:
            {
                data = reinterpret_cast<BYTE *>(propertyValue);
                dataLength = static_cast<ULONG>(sizeof(double));
            }
            break;
        case FABRIC_PROPERTY_TYPE_WSTRING:
            {
                LPCWSTR dataAsString = reinterpret_cast<LPCWSTR>(propertyValue);
                data = reinterpret_cast<BYTE *>(propertyValue);
                dataLength = StringUtility::GetDataLength(dataAsString);
            }
            break;
        case FABRIC_PROPERTY_TYPE_GUID:
            {
                data = reinterpret_cast<BYTE *>(propertyValue);
                dataLength = static_cast<ULONG>(sizeof(GUID));
            }
            break;
        default:
            return E_INVALIDARG;
        }

        bytes = vector<byte>(data, data + dataLength);
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, ULONG* failedOpIndexInRequest, IFabricPropertyBatchResult **result)
    {
        if ((context == NULL) || (failedOpIndexInRequest == NULL) || (result == NULL))
        {
            return E_POINTER;
        }

        ComPointer<PropertyBatchAsyncOperation> thisOperation(context, CLSID_ComFabricClient_PropertyBatchAsyncOperation);
        *failedOpIndexInRequest = static_cast<ULONG>(-1);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            if (thisOperation->resultPtr->GetError().IsSuccess())
            {
                ComPointer<ComPropertyBatchResult> resultCPtr = make_com<ComPropertyBatchResult>(thisOperation->resultPtr);
                *result = resultCPtr.DetachNoRelease();
            }
            else
            {
                *failedOpIndexInRequest = thisOperation->resultPtr->GetFailedOperationIndexInRequest();
                hr = thisOperation->resultPtr->GetError().ToHResult();
            }
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginSubmitPropertyBatch(
            move(batch_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnSubmitPropertyBatchComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnSubmitPropertyBatchComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndSubmitPropertyBatch(operation, resultPtr);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    Naming::NamePropertyOperationBatch batch_;
    IPropertyBatchResultPtr resultPtr;
};

// 0d290702-32a4-4f2d-ac64-bc7fbcfbc725
static const GUID CLSID_ComFabricClient_EnumeratePropertiesAsyncOperation =
{ 0x0d290702, 0x32a4, 0x4f2d, {0xac, 0x64, 0xbc, 0x7f, 0xbc, 0xfb, 0xc7, 0x25} };

class ComFabricClient::EnumeratePropertiesAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(EnumeratePropertiesAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        EnumeratePropertiesAsyncOperation,
        CLSID_ComFabricClient_EnumeratePropertiesAsyncOperation,
        EnumeratePropertiesAsyncOperation,
        ComAsyncOperationContext)
public:

    EnumeratePropertiesAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~EnumeratePropertiesAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in bool includeValues,
        __in ComPointer<ComEnumeratePropertiesResult> prevCPtr,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        includeValues_ = includeValues;
        if (prevCPtr)
        {
            continuationToken_ = prevCPtr->get_ContinuationToken();
        }
        else
        {
            continuationToken_ = Naming::EnumeratePropertiesToken();
        }
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricPropertyEnumerationResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<EnumeratePropertiesAsyncOperation> thisOperation(context, CLSID_ComFabricClient_EnumeratePropertiesAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComEnumeratePropertiesResultCPtr resultCPtr = make_com<ComEnumeratePropertiesResult>(move(thisOperation->result_));
            *result = resultCPtr.DetachNoRelease();
        }
        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginEnumerateProperties(
            name_,
            includeValues_,
            continuationToken_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnEnumeratePropertiesComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnEnumeratePropertiesComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndEnumerateProperties(operation, result_);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    bool includeValues_;
    Naming::EnumeratePropertiesToken continuationToken_;
    Naming::EnumeratePropertiesResult result_;
};

// 46bd16be-06f3-4dc7-8647-65c144d42a1c
static const GUID CLSID_ComFabricClient_PutCustomPropertyAsyncOperation =
{  0x46bd16be, 0x06f3, 0x4dc7, {0x86, 0x47, 0x65, 0xc1, 0x44, 0xd4, 0x2a, 0x1c} };

class ComFabricClient::PutCustomPropertyAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(PutCustomPropertyAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        PutCustomPropertyAsyncOperation,
        CLSID_ComFabricClient_PutCustomPropertyAsyncOperation,
        PutCustomPropertyAsyncOperation,
        ComAsyncOperationContext)
public:

    PutCustomPropertyAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~PutCustomPropertyAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR name,
        __in FABRIC_PUT_CUSTOM_PROPERTY_OPERATION const *propertyOperation,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            propertyOperation->PropertyName,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            propertyName_);
        if (FAILED(hr)) { return hr; }

        typeId_ = propertyOperation->PropertyTypeId;

        hr = StringUtility::LpcwstrToWstring(
            propertyOperation->PropertyCustomTypeId,
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            customTypeId_);
        if (FAILED(hr)) { return hr; }

        hr = PropertyBatchAsyncOperation::GetOperationBytes(typeId_, propertyOperation->PropertyValue, data_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<PutCustomPropertyAsyncOperation> thisOperation(context, CLSID_ComFabricClient_PutCustomPropertyAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.propertyMgmtClient_->BeginPutCustomProperty(
            name_,
            propertyName_,
            typeId_,
            move(data_),
            customTypeId_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnPutCustomPropertyComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnPutCustomPropertyComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.propertyMgmtClient_->EndPutCustomProperty(operation);
        TryComplete(operation->Parent, error);
    }

    NamingUri name_;
    wstring propertyName_;
    FABRIC_PROPERTY_TYPE_ID typeId_;
    ByteBuffer data_;
    wstring customTypeId_;
};

// {3f849613-edf6-4d61-af8e-f756ec04a405}
static const GUID CLSID_ComFabricClient_QueryOperation =
{ 0x3F849613, 0xEDF6, 0x4D61, { 0xAF, 0x8E, 0xF7, 0x56, 0xEC, 0x04, 0xA4, 0x05 } };


class ComFabricClient::QueryOperation : public ClientAsyncOperation
{
    DENY_COPY(QueryOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        QueryOperation,
        CLSID_ComFabricClient_QueryOperation,
        QueryOperation,
        ComAsyncOperationContext)
public:

    QueryOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~QueryOperation() {};

    HRESULT Initialize(
        __in wstring &queryName,
        __in QueryArgumentMap & queryArgs,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        queryName_ = move(queryName);
        queryArgs_ = move(queryArgs);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IInternalFabricQueryResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<QueryOperation> thisOperation(context, CLSID_ComFabricClient_QueryOperation);

        ComPointer<IInternalFabricQueryResult> queryResult = make_com<ComInternalQueryResult, IInternalFabricQueryResult>(move(thisOperation->nativeResult_));

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            queryResult->QueryInterface(IID_IInternalFabricQueryResult, reinterpret_cast<void**>(result));
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IInternalFabricQueryResult2 ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<QueryOperation> thisOperation(context, CLSID_ComFabricClient_QueryOperation);

        auto queryResult = make_com<ComInternalQueryResult, IInternalFabricQueryResult2>(move(thisOperation->nativeResult_));

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            queryResult->QueryInterface(IID_IInternalFabricQueryResult2, reinterpret_cast<void**>(result));
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = this->Owner.queryClient_->BeginInternalQuery(
            queryName_,
            queryArgs_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnQueryComplete(operation);
            },
            proxySPtr);
    }

    HRESULT ComAsyncOperationContextEnd()
    {
        return ComAsyncOperationContext::End();
    }

private:
    void OnQueryComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = this->Owner.queryClient_->EndInternalQuery(operation, nativeResult_);
        TryComplete(operation->Parent, error);
    }

    QueryResult nativeResult_;
    wstring queryName_;
    QueryArgumentMap queryArgs_;
};

class ComFabricClient::ReportTaskStateAsyncOperation :
    public ClientAsyncOperation
{
public:
    ReportTaskStateAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ReportTaskStateAsyncOperation() {};

    HRESULT Initialize(
        __in const LPCWSTR taskId,
        __in ULONGLONG instanceId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            taskId,
            false,
            taskId_);
        if (FAILED(hr)) { return hr; }

        instanceId_ = instanceId;
        return S_OK;
    }

protected:
    __declspec(property(get=get_TaskId)) wstring const &TaskId;
    __declspec(property(get=get_InstanceId)) ULONGLONG InstanceId;

    wstring const & get_TaskId() { return taskId_; }
    ULONGLONG get_InstanceId() { return instanceId_; }

private:
    wstring taskId_;
    ULONGLONG instanceId_;
};

// {c5c2b46b-a947-4ac7-a021-e930a1221fcc}
static const GUID CLSID_ComFabricClient_ReportTaskStartAsyncOperation =
{ 0xc5c2b46b, 0xa947, 0x4ac7, {0xa0, 0x21, 0xe9, 0x30, 0xa1, 0x22, 0x1f, 0xcc}};

class ComFabricClient::ReportTaskStartAsyncOperation :
    public ReportTaskStateAsyncOperation
{
    DENY_COPY(ReportTaskStartAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ReportTaskStartAsyncOperation,
        CLSID_ComFabricClient_ReportTaskStartAsyncOperation,
        ReportTaskStartAsyncOperation ,
        ComAsyncOperationContext)

public:
    ReportTaskStartAsyncOperation(ComFabricClient& owner)
        : ReportTaskStateAsyncOperation(owner)
    {
    }

    virtual ~ReportTaskStartAsyncOperation() {};

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ReportTaskStartAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ReportTaskStartAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.internalInfrastructureClient_->BeginReportStartTaskSuccess(
            TaskId,
            InstanceId,
            Guid::Empty(),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.internalInfrastructureClient_->EndReportStartTaskSuccess(operation);
        TryComplete(operation->Parent, error);
    }
};

// {73301445-fb42-4f5c-82a7-053167b1f543}
static const GUID CLSID_ComFabricClient_ReportTaskFinishAsyncOperation =
{ 0x73301445, 0xfb42, 0x4f5c, {0x82, 0xa7, 0x05, 0x31, 0x67, 0xb1, 0xf5, 0x43} };

class ComFabricClient::ReportTaskFinishAsyncOperation :
    public ReportTaskStateAsyncOperation
{
    DENY_COPY(ReportTaskFinishAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ReportTaskFinishAsyncOperation,
        CLSID_ComFabricClient_ReportTaskFinishAsyncOperation,
        ReportTaskFinishAsyncOperation ,
        ComAsyncOperationContext)

public:
    ReportTaskFinishAsyncOperation(ComFabricClient& owner)
        : ReportTaskStateAsyncOperation(owner)
    {
    }

    virtual ~ReportTaskFinishAsyncOperation() {};

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ReportTaskFinishAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ReportTaskFinishAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.internalInfrastructureClient_->BeginReportFinishTaskSuccess(
            TaskId,
            InstanceId,
            Guid::Empty(),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.internalInfrastructureClient_->EndReportFinishTaskSuccess(operation);
        TryComplete(operation->Parent, error);
    }
};

// {0ba30670-bef8-41ce-9feb-d390342b78f2}
static const GUID CLSID_ComFabricClient_ReportTaskFailureAsyncOperation =
{ 0x0ba30670, 0xbef8, 0x41ce, {0x9f, 0xeb, 0xd3, 0x90, 0x34, 0x2b, 0x78, 0xf2} };

class ComFabricClient::ReportTaskFailureAsyncOperation :
    public ReportTaskStateAsyncOperation
{
    DENY_COPY(ReportTaskFailureAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ReportTaskFailureAsyncOperation,
        CLSID_ComFabricClient_ReportTaskFailureAsyncOperation,
        ReportTaskFailureAsyncOperation ,
        ComAsyncOperationContext)
public:
    ReportTaskFailureAsyncOperation(ComFabricClient& owner)
        : ReportTaskStateAsyncOperation(owner)
    {
    }

    virtual ~ReportTaskFailureAsyncOperation() {};

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ReportTaskFailureAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ReportTaskFailureAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.internalInfrastructureClient_->BeginReportTaskFailure(
            TaskId,
            InstanceId,
            Guid::Empty(),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.internalInfrastructureClient_->EndReportTaskFailure(operation);
        TryComplete(operation->Parent, error);
    }
};

// {d2047fe0-5a0a-4e52-81e9-62b5fc778208}
static const GUID CLSID_ComFabricClient_CreateRepairRequestAsyncOperation =
{ 0xd2047fe0, 0x5a0a, 0x4e52, {0x81, 0xe9, 0x62, 0xb5, 0xfc, 0x77, 0x82, 0x08} };

class ComFabricClient::CreateRepairRequestAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(CreateRepairRequestAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateRepairRequestAsyncOperation,
        CLSID_ComFabricClient_CreateRepairRequestAsyncOperation,
        CreateRepairRequestAsyncOperation,
        ComAsyncOperationContext)

public:
    CreateRepairRequestAsyncOperation(
        ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~CreateRepairRequestAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPAIR_TASK * repairTask,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = task_.FromPublicApi(*repairTask);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out FABRIC_SEQUENCE_NUMBER * commitVersion)
    {
        if (context == NULL) { return E_POINTER; }
        if (commitVersion == NULL) { return E_POINTER; }

        ComPointer<CreateRepairRequestAsyncOperation> thisOperation(context, CLSID_ComFabricClient_CreateRepairRequestAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *commitVersion = static_cast<FABRIC_SEQUENCE_NUMBER>(thisOperation->commitVersion_);
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.repairMgmtClient_->BeginCreateRepairTask(
            task_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.repairMgmtClient_->EndCreateRepairTask(operation, commitVersion_);
        TryComplete(operation->Parent, error);
    }

    Management::RepairManager::RepairTask task_;
    int64 commitVersion_;
};

// {548f8bec-f81a-4cb2-ba7d-cfcb4e3097e9}
static const GUID CLSID_ComFabricClient_CancelRepairRequestAsyncOperation =
{ 0x548f8bec, 0xf81a, 0x4cb2, {0xba, 0x7d, 0xcf, 0xcb, 0x4e, 0x30, 0x97, 0xe9} };

class ComFabricClient::CancelRepairRequestAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(CancelRepairRequestAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CancelRepairRequestAsyncOperation,
        CLSID_ComFabricClient_CancelRepairRequestAsyncOperation,
        CancelRepairRequestAsyncOperation,
        ComAsyncOperationContext)

public:
    CancelRepairRequestAsyncOperation(
        ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~CancelRepairRequestAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPAIR_CANCEL_DESCRIPTION * requestDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = requestBody_.FromPublicApi(*requestDescription);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out FABRIC_SEQUENCE_NUMBER * commitVersion)
    {
        if (context == NULL) { return E_POINTER; }
        if (commitVersion == NULL) { return E_POINTER; }

        ComPointer<CancelRepairRequestAsyncOperation> thisOperation(context, CLSID_ComFabricClient_CancelRepairRequestAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *commitVersion = static_cast<FABRIC_SEQUENCE_NUMBER>(thisOperation->commitVersion_);
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.repairMgmtClient_->BeginCancelRepairTask(
            requestBody_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.repairMgmtClient_->EndCancelRepairTask(operation, commitVersion_);
        TryComplete(operation->Parent, error);
    }

    Management::RepairManager::CancelRepairTaskMessageBody requestBody_;
    int64 commitVersion_;
};

// {361d44a5-46f0-4963-9375-4da8dd50b863}
static const GUID CLSID_ComFabricClient_ForceApproveRepairAsyncOperation =
{ 0x361d44a5, 0x46f0, 0x4963, {0x93, 0x75, 0x4d, 0xa8, 0xdd, 0x50, 0xb8, 0x63} };

class ComFabricClient::ForceApproveRepairAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(ForceApproveRepairAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ForceApproveRepairAsyncOperation,
        CLSID_ComFabricClient_ForceApproveRepairAsyncOperation,
        ForceApproveRepairAsyncOperation,
        ComAsyncOperationContext)

public:
    ForceApproveRepairAsyncOperation(
        ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ForceApproveRepairAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPAIR_APPROVE_DESCRIPTION * requestDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = requestBody_.FromPublicApi(*requestDescription);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out FABRIC_SEQUENCE_NUMBER * commitVersion)
    {
        if (context == NULL) { return E_POINTER; }
        if (commitVersion == NULL) { return E_POINTER; }

        ComPointer<ForceApproveRepairAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ForceApproveRepairAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *commitVersion = static_cast<FABRIC_SEQUENCE_NUMBER>(thisOperation->commitVersion_);
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.repairMgmtClient_->BeginForceApproveRepairTask(
            requestBody_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.repairMgmtClient_->EndForceApproveRepairTask(operation, commitVersion_);
        TryComplete(operation->Parent, error);
    }

    Management::RepairManager::ApproveRepairTaskMessageBody requestBody_;
    int64 commitVersion_;
};

// {b5b2050f-3123-42ae-b1dd-791f3771fa20}
static const GUID CLSID_ComFabricClient_DeleteRepairRequestAsyncOperation =
{ 0xb5b2050f, 0x3123, 0x42ae, {0xb1, 0xdd, 0x79, 0x1f, 0x37, 0x71, 0xfa, 0x20} };

class ComFabricClient::DeleteRepairRequestAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(DeleteRepairRequestAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeleteRepairRequestAsyncOperation,
        CLSID_ComFabricClient_DeleteRepairRequestAsyncOperation,
        DeleteRepairRequestAsyncOperation,
        ComAsyncOperationContext)

public:
    DeleteRepairRequestAsyncOperation(
        ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~DeleteRepairRequestAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPAIR_DELETE_DESCRIPTION * requestDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = requestBody_.FromPublicApi(*requestDescription);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeleteRepairRequestAsyncOperation> thisOperation(context, CLSID_ComFabricClient_DeleteRepairRequestAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.repairMgmtClient_->BeginDeleteRepairTask(
            requestBody_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.repairMgmtClient_->EndDeleteRepairTask(operation);
        TryComplete(operation->Parent, error);
    }

    Management::RepairManager::DeleteRepairTaskMessageBody requestBody_;
};

// {02e670df-757e-47ea-b161-f026bda9ae9c}
static const GUID CLSID_ComFabricClient_UpdateRepairExecutionStateAsyncOperation =
{ 0x02e670df, 0x757e, 0x47ea, {0xb1, 0x61, 0xf0, 0x26, 0xbd, 0xa9, 0xae, 0x9c} };

class ComFabricClient::UpdateRepairExecutionStateAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(UpdateRepairExecutionStateAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpdateRepairExecutionStateAsyncOperation,
        CLSID_ComFabricClient_UpdateRepairExecutionStateAsyncOperation,
        UpdateRepairExecutionStateAsyncOperation,
        ComAsyncOperationContext)

public:
    UpdateRepairExecutionStateAsyncOperation(
        ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpdateRepairExecutionStateAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPAIR_TASK * repairTask,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = task_.FromPublicApi(*repairTask);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out FABRIC_SEQUENCE_NUMBER * commitVersion)
    {
        if (context == NULL) { return E_POINTER; }
        if (commitVersion == NULL) { return E_POINTER; }

        ComPointer<UpdateRepairExecutionStateAsyncOperation> thisOperation(context, CLSID_ComFabricClient_UpdateRepairExecutionStateAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *commitVersion = static_cast<FABRIC_SEQUENCE_NUMBER>(thisOperation->commitVersion_);
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.repairMgmtClient_->BeginUpdateRepairExecutionState(
            task_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.repairMgmtClient_->EndUpdateRepairExecutionState(operation, commitVersion_);
        TryComplete(operation->Parent, error);
    }

    Management::RepairManager::RepairTask task_;
    int64 commitVersion_;
};

// {a1e70a8b-93bc-447f-90e8-d2c098915ffe}
static const GUID CLSID_ComFabricClient_GetRepairTaskListAsyncOperation =
{ 0xa1e70a8b, 0x93bc, 0x447f, {0x90, 0xe8, 0xd2, 0xc0, 0x98, 0x91, 0x5f, 0xfe} };

class ComFabricClient::GetRepairTaskListAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(GetRepairTaskListAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetRepairTaskListAsyncOperation,
        CLSID_ComFabricClient_GetRepairTaskListAsyncOperation,
        GetRepairTaskListAsyncOperation,
        ComAsyncOperationContext)

public:
    GetRepairTaskListAsyncOperation(
        ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetRepairTaskListAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPAIR_TASK_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->TaskIdFilter,
            true,
            taskIdFilter_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ExecutorFilter,
            true,
            executorFilter_);
        if (FAILED(hr)) { return hr; }

        if (queryDescription->StateFilter != (queryDescription->StateFilter & FABRIC_REPAIR_TASK_STATE_FILTER_ALL))
        {
            return E_INVALIDARG;
        }
        stateMask_ = static_cast<Management::RepairManager::RepairTaskState::Enum>(queryDescription->StateFilter);

        if (queryDescription->Scope == NULL) { return E_INVALIDARG; }
        auto error = Management::RepairManager::RepairScopeIdentifierBase::CreateSPtrFromPublicApi(
            *queryDescription->Scope, scope_);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricGetRepairTaskListResult ** result)
    {
        if (context == NULL) { return E_POINTER; }
        if (result == NULL) { return E_POINTER; }

        ComPointer<GetRepairTaskListAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetRepairTaskListAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricGetRepairTaskListResult> repairResult =
                make_com<ComGetRepairTaskListResult, IFabricGetRepairTaskListResult>(move(thisOperation->taskResult_));
            *result = repairResult.DetachNoRelease();
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.repairMgmtClient_->BeginGetRepairTaskList(
            *scope_,
            taskIdFilter_,
            stateMask_,
            executorFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.repairMgmtClient_->EndGetRepairTaskList(operation, taskResult_);
        TryComplete(operation->Parent, error);
    }

    Management::RepairManager::RepairScopeIdentifierBaseSPtr scope_;
    wstring taskIdFilter_;
    Management::RepairManager::RepairTaskState::Enum stateMask_;
    wstring executorFilter_;
    vector<Management::RepairManager::RepairTask> taskResult_;
};

// {C114AE67-CE41-4720-935C-F172665FCEBD}
static const GUID CLSID_ComFabricClient_UpdateRepairTaskHealthPolicyAsyncOperation =
{ 0xc114ae67, 0xce41, 0x4720, { 0x93, 0x5c, 0xf1, 0x72, 0x66, 0x5f, 0xce, 0xbd } };

class ComFabricClient::UpdateRepairTaskHealthPolicyAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(UpdateRepairTaskHealthPolicyAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    UpdateRepairTaskHealthPolicyAsyncOperation,
    CLSID_ComFabricClient_UpdateRepairTaskHealthPolicyAsyncOperation,
    UpdateRepairTaskHealthPolicyAsyncOperation,
    ComAsyncOperationContext)

public:
    UpdateRepairTaskHealthPolicyAsyncOperation(
        ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~UpdateRepairTaskHealthPolicyAsyncOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION * requestDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = requestBody_.FromPublicApi(*requestDescription);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out FABRIC_SEQUENCE_NUMBER * commitVersion)
    {
        if (context == NULL) { return E_POINTER; }
        if (commitVersion == NULL) { return E_POINTER; }

        ComPointer<UpdateRepairTaskHealthPolicyAsyncOperation> thisOperation(context, CLSID_ComFabricClient_UpdateRepairTaskHealthPolicyAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *commitVersion = static_cast<FABRIC_SEQUENCE_NUMBER>(thisOperation->commitVersion_);
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.repairMgmtClient_->BeginUpdateRepairTaskHealthPolicy(
            requestBody_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.repairMgmtClient_->EndUpdateRepairTaskHealthPolicy(operation, commitVersion_);
        TryComplete(operation->Parent, error);
    }

    Management::RepairManager::UpdateRepairTaskHealthPolicyMessageBody requestBody_;
    int64 commitVersion_;
};

// {27662f5f-9f04-44a4-adfa-f216d62d0bea}
static const GUID CLSID_ComFabricClient_RunCommandAsyncOperation =
{ 0x27662f5f, 0x9f04, 0x44a4, {0xad, 0xfa, 0xf2, 0x16, 0xd6, 0x2d, 0x0b, 0xea} };

class ComFabricClient::RunCommandAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(RunCommandAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RunCommandAsyncOperation,
        CLSID_ComFabricClient_RunCommandAsyncOperation,
        RunCommandAsyncOperation ,
        ComAsyncOperationContext)

public:
    RunCommandAsyncOperation(
        ComFabricClient& owner,
        Guid const & targetPartitionId)
        : ClientAsyncOperation(owner)
         , targetPartitionId_(targetPartitionId)
    {
    }

    virtual ~RunCommandAsyncOperation() {};

    HRESULT Initialize(
        __in const LPCWSTR command,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            command,
            false,
            command_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricStringResult **result)
    {
        if (context == NULL) { return E_POINTER; }
        if (result == NULL) { return E_POINTER; }

        ComPointer<RunCommandAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RunCommandAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(
                thisOperation->result_);
            *result = stringResult.DetachNoRelease();
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.internalInfrastructureClient_->BeginRunCommand(
            command_,
            targetPartitionId_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.internalInfrastructureClient_->EndRunCommand(operation, result_);
        TryComplete(operation->Parent, error);
    }

    wstring command_;
    wstring result_;
    Guid targetPartitionId_;
};

// {fc87b42f-8878-4b61-95c6-713bdd4d8fce}
static const GUID CLSID_ComFabricClient_InvokeInfrastructureCommandAsyncOperation =
{ 0xfc87b42f, 0x8878, 0x4b61, {0x95, 0xc6, 0x71, 0x3b, 0xdd, 0x4d, 0x8f, 0xce} };

class ComFabricClient::InvokeInfrastructureCommandAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(InvokeInfrastructureCommandAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        InvokeInfrastructureCommandAsyncOperation,
        CLSID_ComFabricClient_InvokeInfrastructureCommandAsyncOperation,
        InvokeInfrastructureCommandAsyncOperation,
        ComAsyncOperationContext)

public:
    InvokeInfrastructureCommandAsyncOperation(
        ComFabricClient& owner,
        bool isAdminOperation)
        : ClientAsyncOperation(owner)
        , isAdminOperation_(isAdminOperation)
    {
    }

    virtual ~InvokeInfrastructureCommandAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI serviceName,
        __in const LPCWSTR command,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        if (serviceName == NULL)
        {
            serviceName = SystemServiceApplicationNameHelper::PublicInfrastructureServiceName->c_str();
        }

        hr = NamingUri::TryParse(serviceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            command,
            false,
            command_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricStringResult **result)
    {
        if (context == NULL) { return E_POINTER; }
        if (result == NULL) { return E_POINTER; }

        ComPointer<InvokeInfrastructureCommandAsyncOperation> thisOperation(context, CLSID_ComFabricClient_InvokeInfrastructureCommandAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(
                thisOperation->result_);
            *result = stringResult.DetachNoRelease();
        }
        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.infrastructureClient_->BeginInvokeInfrastructureCommand(
            isAdminOperation_,
            serviceName_,
            command_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.infrastructureClient_->EndInvokeInfrastructureCommand(operation, result_);
        TryComplete(operation->Parent, error);
    }

    bool isAdminOperation_;
    NamingUri serviceName_;
    wstring command_;
    wstring result_;
};

// {5B6FE4A2-0FC5-40DD-A60A-824EFFCF6CA8}
static const GUID CLSID_ComFabricClient_StartClusterConfigurationUpgradeAsyncOperation =
{ 0x5b6fe4a2, 0xfc5, 0x40dd, { 0xa6, 0xa, 0x82, 0x4e, 0xff, 0xcf, 0x6c, 0xa8 } };

class ComFabricClient::StartClusterConfigurationUpgradeAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(StartClusterConfigurationUpgradeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    StartClusterConfigurationUpgradeAsyncOperation,
    CLSID_ComFabricClient_StartClusterConfigurationUpgradeAsyncOperation,
    StartClusterConfigurationUpgradeAsyncOperation,
    ComAsyncOperationContext)

public:
    StartClusterConfigurationUpgradeAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        description_()
    {
    }

    virtual ~StartClusterConfigurationUpgradeAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_START_UPGRADE_DESCRIPTION const * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = description_.FromPublicApi(*description);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StartClusterConfigurationUpgradeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StartClusterConfigurationUpgradeAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.clusterMgmtClient_->BeginUpgradeConfiguration(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndUpgradeConfiguration(operation);
        TryComplete(operation->Parent, error);
    }

    StartUpgradeDescription description_;
};

// {FC91D2C9-803A-4703-B076-3D3DA1A57517}
static const GUID CLSID_ComFabricClient_GetClusterConfigurationUpgradeStatusAsyncOperation =
{ 0xfc91d2c9, 0x803a, 0x4703, { 0xb0, 0x76, 0x3d, 0x3d, 0xa1, 0xa5, 0x75, 0x17 } };

class ComFabricClient::GetClusterConfigurationUpgradeStatusAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(GetClusterConfigurationUpgradeStatusAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetClusterConfigurationUpgradeStatusAsyncOperation,
    CLSID_ComFabricClient_GetClusterConfigurationUpgradeStatusAsyncOperation,
    GetClusterConfigurationUpgradeStatusAsyncOperation,
    ComAsyncOperationContext)

public:
    GetClusterConfigurationUpgradeStatusAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetClusterConfigurationUpgradeStatusAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }
        errorMessage = wstring();
        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetClusterConfigurationUpgradeStatusAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetClusterConfigurationUpgradeStatusAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricOrchestrationUpgradeStatusResult>(thisOperation->orchestrationUpgradeStatusResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.clusterMgmtClient_->BeginGetClusterConfigurationUpgradeStatus(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndGetClusterConfigurationUpgradeStatus(operation, orchestrationUpgradeStatusResultPtr_);
        TryComplete(operation->Parent, error);
    }

    IFabricOrchestrationUpgradeStatusResultPtr orchestrationUpgradeStatusResultPtr_;
};

// {DEC108A0-331D-4F56-9C2B-1168288B0784}
static const GUID CLSID_ComFabricClient_GetClusterConfigurationAsyncOperation =
{ 0xdec108a0, 0x331d, 0x4f56, { 0x9c, 0x2b, 0x11, 0x68, 0x28, 0x8b, 0x7, 0x84 } };

class ComFabricClient::GetClusterConfigurationAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetClusterConfigurationAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetClusterConfigurationAsyncOperation,
    CLSID_ComFabricClient_GetClusterConfigurationAsyncOperation,
    GetClusterConfigurationAsyncOperation,
    ComAsyncOperationContext)
public:

    GetClusterConfigurationAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetClusterConfigurationAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR apiVersion,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        if (apiVersion != NULL)
        {
            apiVersion_.assign(apiVersion);
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricStringResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetClusterConfigurationAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetClusterConfigurationAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(
                thisOperation->clusterConfiguration_);
            *result = stringResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginGetClusterConfiguration(
            move(apiVersion_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetClusterConfigurationComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetClusterConfigurationComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndGetClusterConfiguration(operation, clusterConfiguration_);
        TryComplete(operation->Parent, error);
    }

    wstring clusterConfiguration_;
    wstring apiVersion_;
};


// {835CDE7C-C091-47B4-883B-3601567B50AE}
static const GUID CLSID_ComFabricClient_GetUpgradesPendingApprovalAsyncOperation =
{ 0x835cde7c, 0xc091, 0x47b4, { 0x88, 0x3b, 0x36, 0x1, 0x56, 0x7b, 0x50, 0xae } };

class ComFabricClient::GetUpgradesPendingApprovalAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(GetUpgradesPendingApprovalAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetUpgradesPendingApprovalAsyncOperation,
    CLSID_ComFabricClient_GetUpgradesPendingApprovalAsyncOperation,
    GetUpgradesPendingApprovalAsyncOperation,
    ComAsyncOperationContext)

public:
    GetUpgradesPendingApprovalAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetUpgradesPendingApprovalAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }
        errorMessage = wstring();
        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetUpgradesPendingApprovalAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetUpgradesPendingApprovalAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.clusterMgmtClient_->BeginGetUpgradesPendingApproval(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndGetUpgradesPendingApproval(operation);
        TryComplete(operation->Parent, error);
    }
};

// {21403115-9E3C-4ABF-8BC3-9E736F31AAE6}
static const GUID CLSID_ComFabricClient_StartApprovedUpgradesAsyncOperation =
{ 0x21403115, 0x9e3c, 0x4abf, { 0x8b, 0xc3, 0x9e, 0x73, 0x6f, 0x31, 0xaa, 0xe6 } };

class ComFabricClient::StartApprovedUpgradesAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(StartApprovedUpgradesAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    StartApprovedUpgradesAsyncOperation,
    CLSID_ComFabricClient_StartApprovedUpgradesAsyncOperation,
    StartApprovedUpgradesAsyncOperation,
    ComAsyncOperationContext)

public:
    StartApprovedUpgradesAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~StartApprovedUpgradesAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }
        errorMessage = wstring();
        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StartApprovedUpgradesAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StartApprovedUpgradesAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.clusterMgmtClient_->BeginStartApprovedUpgrades(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndStartApprovedUpgrades(operation);
        TryComplete(operation->Parent, error);
    }
};

// {239EF09C-220C-4E66-B731-C6A660B628A4}
static const GUID CLSID_ComFabricClient_GetUpgradeOrchestrationServiceStateAsyncOperation =
{ 0x239ef09c, 0x220c, 0x4e66,{ 0xb7, 0x31, 0xc6, 0xa6, 0x60, 0xb6, 0x28, 0xa4 } };

class ComFabricClient::GetUpgradeOrchestrationServiceStateAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetUpgradeOrchestrationServiceStateAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetUpgradeOrchestrationServiceStateAsyncOperation,
            CLSID_ComFabricClient_GetUpgradeOrchestrationServiceStateAsyncOperation,
            GetUpgradeOrchestrationServiceStateAsyncOperation,
            ComAsyncOperationContext)
public:

    GetUpgradeOrchestrationServiceStateAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetUpgradeOrchestrationServiceStateAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricStringResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetUpgradeOrchestrationServiceStateAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetUpgradeOrchestrationServiceStateAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(
                thisOperation->serviceState_);
            *result = stringResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginGetUpgradeOrchestrationServiceState(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetUpgradeOrchestrationServiceStateComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetUpgradeOrchestrationServiceStateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndGetUpgradeOrchestrationServiceState(operation, serviceState_);
        TryComplete(operation->Parent, error);
    }

    wstring serviceState_;
};

// {9A5F0909-FB89-4009-8F29-50EEADBE910F}
static const GUID CLSID_ComFabricClient_SetUpgradeOrchestrationServiceStateAsyncOperation =
{ 0x9a5f0909, 0xfb89, 0x4009,{ 0x8f, 0x29, 0x50, 0xee, 0xad, 0xbe, 0x91, 0xf } };

class ComFabricClient::SetUpgradeOrchestrationServiceStateAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(SetUpgradeOrchestrationServiceStateAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            SetUpgradeOrchestrationServiceStateAsyncOperation,
            CLSID_ComFabricClient_SetUpgradeOrchestrationServiceStateAsyncOperation,
            SetUpgradeOrchestrationServiceStateAsyncOperation,
            ComAsyncOperationContext)
public:

    SetUpgradeOrchestrationServiceStateAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        inputState_()
    {
    }

    virtual ~SetUpgradeOrchestrationServiceStateAsyncOperation() {};

    HRESULT Initialize(
        __in LPCWSTR state,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr))
        {
            return hr;
        }

        inputState_.assign(state);
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricUpgradeOrchestrationServiceStateResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<SetUpgradeOrchestrationServiceStateAsyncOperation> thisOperation(context, CLSID_ComFabricClient_SetUpgradeOrchestrationServiceStateAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricUpgradeOrchestrationServiceStateResult> comPtr = make_com<ComFabricUpgradeOrchestrationServiceStateResult, IFabricUpgradeOrchestrationServiceStateResult>(
                thisOperation->serviceStateResultPtr_);
            hr = comPtr->QueryInterface(IID_IFabricUpgradeOrchestrationServiceStateResult, (void **)result);
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginSetUpgradeOrchestrationServiceState(
            move(inputState_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnSetUpgradeOrchestrationServiceStateComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnSetUpgradeOrchestrationServiceStateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.clusterMgmtClient_->EndSetUpgradeOrchestrationServiceState(operation, serviceStateResultPtr_);
        TryComplete(operation->Parent, error);
    }

    std::wstring inputState_;
    IFabricUpgradeOrchestrationServiceStateResultPtr serviceStateResultPtr_;
};

// {57C06174-BE24-4C23-98F9-3229966D81B1}
static const GUID CLSID_ComFabricClient_GetSecretsAsyncOperation =
{ 0x57c06174, 0xbe24, 0x4c23,{ 0x98, 0xf9, 0x32, 0x29, 0x96, 0x6d, 0x81, 0xb1 } };

class ComFabricClient::GetSecretsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetSecretsAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetSecretsAsyncOperation,
            CLSID_ComFabricClient_GetSecretsAsyncOperation,
            GetSecretsAsyncOperation,
            ComAsyncOperationContext)
public:

    GetSecretsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetSecretsAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_SECRET_REFERENCE_LIST const *secretReferences,
        __in BOOLEAN includeValue,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = this->description_.FromPublicApi(secretReferences, includeValue);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricSecretsResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetSecretsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetSecretsAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricSecretsResult> secretsResult = make_com<ComSecretsResult, IFabricSecretsResult>(
                thisOperation->result_);
            *result = secretsResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.secretStoreClient_->BeginGetSecrets(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetSecretsComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetSecretsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.secretStoreClient_->EndGetSecrets(operation, result_);
        TryComplete(operation->Parent, move(error));
    }

    GetSecretsDescription description_;
    SecretsDescription result_;
};

// {E782934F-4186-4CD8-9E6B-D4F94C530AB9}
static const GUID CLSID_ComFabricClient_SetSecretsAsyncOperation =
{ 0xe782934f, 0x4186, 0x4cd8,{ 0x9e, 0x6b, 0xd4, 0xf9, 0x4c, 0x53, 0xa, 0xb9 } };

class ComFabricClient::SetSecretsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(SetSecretsAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            SetSecretsAsyncOperation,
            CLSID_ComFabricClient_SetSecretsAsyncOperation,
            SetSecretsAsyncOperation,
            ComAsyncOperationContext)
public:

    SetSecretsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~SetSecretsAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_SECRET_LIST const * secrets,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = this->description_.FromPublicApi(secrets);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricSecretsResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<SetSecretsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_SetSecretsAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricSecretsResult> secretsResult = make_com<ComSecretsResult, IFabricSecretsResult>(
                thisOperation->result_);
            *result = secretsResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.secretStoreClient_->BeginSetSecrets(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnSetSecretsComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnSetSecretsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.secretStoreClient_->EndSetSecrets(operation, result_);
        TryComplete(operation->Parent, move(error));
    }

    SecretsDescription description_;
    SecretsDescription result_;
};

// {D1A35E10-27B3-41DB-92BA-1541756BFA9E}
static const GUID CLSID_ComFabricClient_RemoveSecretsAsyncOperation =
{ 0xd1a35e10, 0x27b3, 0x41db,{ 0x92, 0xba, 0x15, 0x41, 0x75, 0x6b, 0xfa, 0x9e } };

class ComFabricClient::RemoveSecretsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RemoveSecretsAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            RemoveSecretsAsyncOperation,
            CLSID_ComFabricClient_RemoveSecretsAsyncOperation,
            RemoveSecretsAsyncOperation,
            ComAsyncOperationContext)
public:

    RemoveSecretsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~RemoveSecretsAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_SECRET_REFERENCE_LIST const * secretReferences,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = this->description_.FromPublicApi(secretReferences);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricSecretReferencesResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<RemoveSecretsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RemoveSecretsAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricSecretReferencesResult> secretReferencesResult = make_com<ComSecretReferencesResult, IFabricSecretReferencesResult>(
                thisOperation->result_);
            *result = secretReferencesResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.secretStoreClient_->BeginRemoveSecrets(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnRemoveSecretsComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnRemoveSecretsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.secretStoreClient_->EndRemoveSecrets(operation, result_);
        TryComplete(operation->Parent, move(error));
    }

    SecretReferencesDescription description_;
    SecretReferencesDescription result_;
};

// {65835992-F186-414D-AF2F-4283E391F115}
static const GUID CLSID_ComFabricClient_GetSecretVersionsAsyncOperation =
{ 0x65835992, 0xf186, 0x414d,{ 0xaf, 0x2f, 0x42, 0x83, 0xe3, 0x91, 0xf1, 0x15 } };

class ComFabricClient::GetSecretVersionsAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetSecretVersionsAsyncOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetSecretVersionsAsyncOperation,
            CLSID_ComFabricClient_GetSecretVersionsAsyncOperation,
            GetSecretVersionsAsyncOperation,
            ComAsyncOperationContext)
public:

    GetSecretVersionsAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetSecretVersionsAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_SECRET_REFERENCE_LIST const * secretReferences,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = this->description_.FromPublicApi(secretReferences);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricSecretReferencesResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetSecretVersionsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetSecretVersionsAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricSecretReferencesResult> secretReferencesResult = make_com<ComSecretReferencesResult, IFabricSecretReferencesResult>(
                thisOperation->result_);
            *result = secretReferencesResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.secretStoreClient_->BeginGetSecretVersions(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetSecretVersionsComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetSecretVersionsComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.secretStoreClient_->EndGetSecretVersions(operation, result_);
        TryComplete(operation->Parent, move(error));
    }

    SecretReferencesDescription description_;
    SecretReferencesDescription result_;
};


// {4a7b27f4-1ca0-43a3-8657-257bd8c1eae1}
static const GUID CLSID_ComFabricClient_InvokeDataLossAsyncOperation =
{ 0x4a7b27f4, 0x1ca0, 0x43a3, { 0x86, 0x57, 0x25, 0x7b, 0xd8, 0xc1, 0xea, 0xe1 } };

class ComFabricClient::InvokeDataLossAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(InvokeDataLossAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    InvokeDataLossAsyncOperation,
    CLSID_ComFabricClient_InvokeDataLossAsyncOperation,
    InvokeDataLossAsyncOperation,
    ComAsyncOperationContext)

public:
    InvokeDataLossAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        invokeDataLossDescription_()
    {
    }

    virtual ~InvokeDataLossAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION const * publicInvokeDataLossDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = invokeDataLossDescription_.FromPublicApi(*publicInvokeDataLossDescription);

        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<InvokeDataLossAsyncOperation> thisOperation(context, CLSID_ComFabricClient_InvokeDataLossAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginInvokeDataLoss(
            invokeDataLossDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndInvokeDataLoss(operation);
        TryComplete(operation->Parent, error);
    }

    InvokeDataLossDescription invokeDataLossDescription_;
};

// {d018f13b-f479-49cb-9173-5d69cc709cc1}
static const GUID CLSID_ComFabricClient_GetInvokeDataLossProgressAsyncOperation =
{ 0xd018f13b, 0xf479, 0x49cb, { 0x91, 0x73, 0x5d, 0x69, 0xcc, 0x70, 0x9c, 0xe1} };

class ComFabricClient::GetInvokeDataLossProgressAsyncOperation:
public ClientAsyncOperation
{
    DENY_COPY(GetInvokeDataLossProgressAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetInvokeDataLossProgressAsyncOperation,
    CLSID_ComFabricClient_GetInvokeDataLossProgressAsyncOperation,
    GetInvokeDataLossProgressAsyncOperation,
    ComAsyncOperationContext)

public:
    GetInvokeDataLossProgressAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        operationId_()
    {
    }

    virtual ~GetInvokeDataLossProgressAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_TEST_COMMAND_OPERATION_ID publicOperationId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        operationId_ = publicOperationId;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetInvokeDataLossProgressAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetInvokeDataLossProgressAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricInvokeDataLossProgressResult>(thisOperation->invokeDataLossProgressResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginGetInvokeDataLossProgress(
            Common::Guid(operationId_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetInvokeDataLossProgressComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetInvokeDataLossProgressComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetInvokeDataLossProgress(operation, invokeDataLossProgressResultPtr_);

        TryComplete(operation->Parent, error);
    }

    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    IInvokeDataLossProgressResultPtr invokeDataLossProgressResultPtr_;
};

// {dc1c0aca-ea44-4e28-9190-376be0a65976}
static const GUID CLSID_ComFabricClient_InvokeQuorumLossAsyncOperation =
{ 0xdc1c0aca, 0xea44, 0x4e28, { 0x91, 0x90, 0x37, 0x6b, 0xe0, 0xa6, 0x59, 0x76 } };

class ComFabricClient::InvokeQuorumLossAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(InvokeQuorumLossAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    InvokeQuorumLossAsyncOperation,
    CLSID_ComFabricClient_InvokeQuorumLossAsyncOperation,
    InvokeQuorumLossAsyncOperation,
    ComAsyncOperationContext)

public:
    InvokeQuorumLossAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        invokeQuorumLossDescription_()
    {
    }

    virtual ~InvokeQuorumLossAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION const * publicInvokeQuorumLossDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = invokeQuorumLossDescription_.FromPublicApi(*publicInvokeQuorumLossDescription);

        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<InvokeQuorumLossAsyncOperation> thisOperation(context, CLSID_ComFabricClient_InvokeQuorumLossAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginInvokeQuorumLoss(
            invokeQuorumLossDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndInvokeQuorumLoss(operation);
        TryComplete(operation->Parent, error);
    }

    InvokeQuorumLossDescription invokeQuorumLossDescription_;
};

// {8fb38958-e68c-4041-9b43-58ad055bb88d}
static const GUID CLSID_ComFabricClient_GetInvokeQuorumLossProgressAsyncOperation =
{ 0x8fb38958, 0xe68c, 0x4041, { 0x9b, 0x43, 0x58, 0xad, 0x05, 0x5b, 0xb8, 0x8d} };

class ComFabricClient::GetInvokeQuorumLossProgressAsyncOperation:
public ClientAsyncOperation
{
    DENY_COPY(GetInvokeQuorumLossProgressAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetInvokeQuorumLossProgressAsyncOperation,
    CLSID_ComFabricClient_GetInvokeQuorumLossProgressAsyncOperation,
    GetInvokeQuorumLossProgressAsyncOperation,
    ComAsyncOperationContext)

public:
    GetInvokeQuorumLossProgressAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        operationId_()
    {
    }

    virtual ~GetInvokeQuorumLossProgressAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_TEST_COMMAND_OPERATION_ID publicOperationId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        operationId_ = publicOperationId;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetInvokeQuorumLossProgressAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetInvokeQuorumLossProgressAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricInvokeQuorumLossProgressResult>(thisOperation->invokeQuorumLossProgressResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginGetInvokeQuorumLossProgress(
            Common::Guid(operationId_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetInvokeQuorumLossProgressComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetInvokeQuorumLossProgressComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetInvokeQuorumLossProgress(operation, invokeQuorumLossProgressResultPtr_);

        TryComplete(operation->Parent, error);
    }

    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    IInvokeQuorumLossProgressResultPtr invokeQuorumLossProgressResultPtr_;
};


// restart partition
// {9900c95d-d043-4f8c-8658-a186240d1453}
static const GUID CLSID_ComFabricClient_RestartPartitionAsyncOperation =
{ 0x9900c95d, 0xd043, 0x4f8c, { 0x86, 0x58, 0xa1, 0x86, 0x24, 0x0d, 0x14, 0x53 } };

class ComFabricClient::RestartPartitionAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(RestartPartitionAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    RestartPartitionAsyncOperation,
    CLSID_ComFabricClient_RestartPartitionAsyncOperation,
    RestartPartitionAsyncOperation,
    ComAsyncOperationContext)

public:
    RestartPartitionAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        restartPartitionDescription_()
    {
    }

    virtual ~RestartPartitionAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_START_PARTITION_RESTART_DESCRIPTION const * publicRestartPartitionDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = restartPartitionDescription_.FromPublicApi(*publicRestartPartitionDescription);

        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RestartPartitionAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RestartPartitionAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginRestartPartition(
            restartPartitionDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndRestartPartition(operation);
        TryComplete(operation->Parent, error);
    }

    RestartPartitionDescription restartPartitionDescription_;
};

// {3f920b8f-f3b4-47e0-b7bb-abd2513efa26}
static const GUID CLSID_ComFabricClient_GetRestartPartitionProgressAsyncOperation =
{ 0x3f920b8f, 0xf3b4, 0x47e0, { 0xb7, 0xbb, 0xab, 0xd2, 0x51, 0x3e, 0xfa, 0x26 } };

class ComFabricClient::GetRestartPartitionProgressAsyncOperation:
public ClientAsyncOperation
{
    DENY_COPY(GetRestartPartitionProgressAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetRestartPartitionProgressAsyncOperation,
    CLSID_ComFabricClient_GetRestartPartitionProgressAsyncOperation,
    GetRestartPartitionProgressAsyncOperation,
    ComAsyncOperationContext)

public:
    GetRestartPartitionProgressAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        operationId_()
    {
    }

    virtual ~GetRestartPartitionProgressAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_TEST_COMMAND_OPERATION_ID publicOperationId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        operationId_ = publicOperationId;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetRestartPartitionProgressAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetRestartPartitionProgressAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricRestartPartitionProgressResult>(thisOperation->restartPartitionProgressResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginGetRestartPartitionProgress(
            Common::Guid(operationId_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetRestartPartitionProgressComplete(operation);
            },
            proxySPtr);
     }

private:
    void OnGetRestartPartitionProgressComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetRestartPartitionProgress(operation, restartPartitionProgressResultPtr_);

        TryComplete(operation->Parent, error);
    }

    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    IRestartPartitionProgressResultPtr restartPartitionProgressResultPtr_;
};

// {48517B5B-9D0F-46AE-8A39-1FDFF3CC1468}
static const GUID CLSID_ComFabricClient_GetTestCommandListOperation =
    {0x48517b5b,0x9d0f,0x46ae,{0x8a,0x39,0x1f,0xdf,0xf3,0xcc,0x14,0x68}};

class ComFabricClient::GetTestCommandStatusListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetTestCommandStatusListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetTestCommandStatusListOperation,
        CLSID_ComFabricClient_GetTestCommandListOperation,
        GetTestCommandStatusListOperation,
        ComAsyncOperationContext)

public:
    GetTestCommandStatusListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , commandStateFilter_()
        , commandTypeFilter_()
        , commandListResult_()
    {
    }

    virtual ~GetTestCommandStatusListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_TEST_COMMAND_LIST_DESCRIPTION * queryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        commandStateFilter_ = queryDescription->TestCommandStateFilter;
        commandTypeFilter_ = queryDescription->TestCommandTypeFilter;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricTestCommandStatusResult**result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetTestCommandStatusListOperation> thisOperation(context, CLSID_ComFabricClient_GetTestCommandListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricTestCommandStatusResult> comPtr = make_com<ComGetTestCommandStatusResult, IFabricTestCommandStatusResult>(

            move(thisOperation->commandListResult_));
            *result = comPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.testManagementClient_->BeginGetTestCommandStatusList(
            commandStateFilter_,
            commandTypeFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetTestCommandStatusListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetTestCommandStatusListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetTestCommandStatusList(operation, commandListResult_);
        TryComplete(operation->Parent, error);
    }

private:

    DWORD commandStateFilter_;
    DWORD commandTypeFilter_;

    vector<TestCommandListQueryResult> commandListResult_;
};

// {320FC097-A8B0-4A8F-8C2D-EE51001EB387}
static const GUID CLSID_ComFabricClient_CancelTestCommandAsyncOperation =
{ 0x320fc097, 0xa8b0, 0x4a8f, { 0x8c, 0x2d, 0xee, 0x51, 0x0, 0x1e, 0xb3, 0x87 } };
class ComFabricClient::CancelTestCommandAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(CancelTestCommandAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    CancelTestCommandAsyncOperation,
    CLSID_ComFabricClient_CancelTestCommandAsyncOperation,
    CancelTestCommandAsyncOperation,
    ComAsyncOperationContext)

public:
    CancelTestCommandAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        description_()
    {
    }

    virtual ~CancelTestCommandAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION const * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = description_.FromPublicApi(*description);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CancelTestCommandAsyncOperation> thisOperation(context, CLSID_ComFabricClient_CancelTestCommandAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginCancelTestCommand(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndCancelTestCommand(operation);
        TryComplete(operation->Parent, error);
    }

    CancelTestCommandDescription description_;
};


// {BA4A2CB7-3C41-4995-8786-094D7092706B}
static const GUID CLSID_ComFabricClient_StartNodeTransitionAsyncOperation =
{ 0xba4a2cb7, 0x3c41, 0x4995, { 0x87, 0x86, 0x9, 0x4d, 0x70, 0x92, 0x70, 0x6b } };
class ComFabricClient::StartNodeTransitionAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(StartNodeTransitionAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        StartNodeTransitionAsyncOperation,
        CLSID_ComFabricClient_StartNodeTransitionAsyncOperation,
        StartNodeTransitionAsyncOperation,
        ComAsyncOperationContext)

public:
    StartNodeTransitionAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        description_()
    {
    }

    virtual ~StartNodeTransitionAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_NODE_TRANSITION_DESCRIPTION const * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StartNodeTransitionAsyncOperation::Init");

        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StartNodeTransitionAsyncOperation::Init: Kind:{0}",
            (int)(description->NodeTransitionType));

        auto error = description_.FromPublicApi(*description);
        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StartNodeTransitionAsyncOperation::Init: FromPublicApi error:{0}",
            error);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StartNodeTransitionAsyncOperation::Init: FromPublicApi error:{0}, NodeTransitionType:{1}, OperationId:{2}, NodeName:{3}, NodeInstanceId:{4}",
            error,
            (int)(description->NodeTransitionType),
            description_.OperationId,
            description_.NodeName,
            description_.NodeInstanceId);
        if (description->NodeTransitionType == FABRIC_NODE_TRANSITION_TYPE_STOP)
        {
            Trace.WriteInfo(
                TraceComponent,
                "ComFabricClient::StartNodeTransitionAsyncOperation::Init: FromPublicApi Duration:{0}",
                description_.StopDurationInSeconds);
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StartNodeTransitionAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StartNodeTransitionAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StartNodeTransitionAsyncOperation::OnStart");

        auto operation = Owner.testManagementClient_->BeginStartNodeTransition(
            description_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndStartNodeTransition(operation);
        TryComplete(operation->Parent, error);
    }

    StartNodeTransitionDescription description_;
};

static const GUID CLSID_ComFabricClient_GetNodeTransitionProgressAsyncOperation =
{ 0x48adc9e9, 0x1fe4, 0x4445, { 0xb3, 0xd2, 0xd1, 0x14, 0x86, 0x12, 0xf8, 0xb } };
class ComFabricClient::GetNodeTransitionProgressAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(GetNodeTransitionProgressAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetNodeTransitionProgressAsyncOperation,
    CLSID_ComFabricClient_GetNodeTransitionProgressAsyncOperation,
    GetNodeTransitionProgressAsyncOperation,
    ComAsyncOperationContext)

public:
    GetNodeTransitionProgressAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        operationId_()
    {
    }

    virtual ~GetNodeTransitionProgressAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_TEST_COMMAND_OPERATION_ID publicOperationId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        operationId_ = publicOperationId;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetNodeTransitionProgressAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetNodeTransitionProgressAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricNodeTransitionProgressResult>(thisOperation->nodeTransitionProgressResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::GetNodeTransitionProgressAsyncOperation::OnStart"
            );
        auto operation = Owner.testManagementClient_->BeginGetNodeTransitionProgress(
            Common::Guid(operationId_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetNodeTransitionProgressComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetNodeTransitionProgressComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetNodeTransitionProgress(operation, nodeTransitionProgressResultPtr_);

        TryComplete(operation->Parent, error);
    }

    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    INodeTransitionProgressResultPtr nodeTransitionProgressResultPtr_;
};

// Chaos

// {3F0CC759-770F-4193-9E69-DB04DD039CFC}
static const GUID CLSID_ComFabricClient_StartChaosAsyncOperation =
{ 0x3f0cc759, 0x770f, 0x4193, { 0x9e, 0x69, 0xdb, 0x4, 0xdd, 0x3, 0x9c, 0xfc } };


class ComFabricClient::StartChaosAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(StartChaosAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    StartChaosAsyncOperation,
    CLSID_ComFabricClient_StartChaosAsyncOperation,
    StartChaosAsyncOperation,
    ComAsyncOperationContext)

public:
    StartChaosAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        startChaosDescription_()
    {
    }

    virtual ~StartChaosAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_START_CHAOS_DESCRIPTION const * publicStartChaosDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = startChaosDescription_.FromPublicApi(*publicStartChaosDescription);

        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StartChaosAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StartChaosAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr) override
    {
        auto operation = Owner.testManagementClient_->BeginStartChaos(
            move(startChaosDescription_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndStartChaos(operation);
        TryComplete(operation->Parent, error);
    }

    StartChaosDescription startChaosDescription_;
};

// {6E719C14-7889-407F-9504-22FB5471B862}
static const GUID CLSID_ComFabricClient_StopChaosAsyncOperation =
{ 0x6e719c14, 0x7889, 0x407f, { 0x95, 0x4, 0x22, 0xfb, 0x54, 0x71, 0xb8, 0x62 } };

class ComFabricClient::StopChaosAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(StopChaosAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    StopChaosAsyncOperation,
    CLSID_ComFabricClient_StopChaosAsyncOperation,
    StopChaosAsyncOperation,
    ComAsyncOperationContext)

public:
    StopChaosAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~StopChaosAsyncOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StopChaosAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StopChaosAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr) override
    {
        auto operation = Owner.testManagementClient_->BeginStopChaos(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndStopChaos(operation);
        TryComplete(operation->Parent, error);
    }
};

// {05CD6AC3-C230-42C5-B366-587C5126C4C0}
static const GUID CLSID_ComFabricClient_GetChaosReportAsyncOperation =
{ 0x5cd6ac3, 0xc230, 0x42c5, { 0xb3, 0x66, 0x58, 0x7c, 0x51, 0x26, 0xc4, 0xc0 } };

class ComFabricClient::GetChaosReportAsyncOperation:
public ClientAsyncOperation
{
    DENY_COPY(GetChaosReportAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetChaosReportAsyncOperation,
    CLSID_ComFabricClient_GetChaosReportAsyncOperation,
    GetChaosReportAsyncOperation,
    ComAsyncOperationContext)

public:
    GetChaosReportAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetChaosReportAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_GET_CHAOS_REPORT_DESCRIPTION const * publicGetChaosReportDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        if (publicGetChaosReportDescription->Filter != nullptr && DateTime(publicGetChaosReportDescription->Filter->StartTimeUtc) > DateTime::Zero
            && publicGetChaosReportDescription->ContinuationToken != nullptr)
        {
            errorMessage = wformatString("{0}", ComFabricClientFASResource::GetResources().GetChaosReportArgumentInvalid);
            return ErrorCode(ErrorCodeValue::InvalidArgument).ToHResult();
        }
        else if (publicGetChaosReportDescription->Filter == nullptr
            && publicGetChaosReportDescription->ContinuationToken == nullptr)
        {
            return ErrorCode(ErrorCodeValue::ArgumentNull).ToHResult();
        }

        auto error = getChaosReportDescription_.FromPublicApi(*publicGetChaosReportDescription);

        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == nullptr || result == nullptr) { return E_POINTER; }

        ComPointer<GetChaosReportAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetChaosReportAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricChaosReportResult>(thisOperation->chaosReportResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginGetChaosReport(
            getChaosReportDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetChaosReportComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetChaosReportComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetChaosReport(operation, chaosReportResultPtr_);

        TryComplete(operation->Parent, error);
    }

    GetChaosReportDescription getChaosReportDescription_;
    IChaosReportResultPtr chaosReportResultPtr_;
};

// {5F76A94E-F46F-406A-93EE-93E47D107058}
static const GUID CLSID_ComFabricClient_GetChaosEventsAsyncOperation =
{ 0x5f76a94e, 0xf46f, 0x406a, { 0x93, 0xee, 0x93, 0xe4, 0x7d, 0x10, 0x70, 0x58 } };

class ComFabricClient::GetChaosEventsAsyncOperation:
public ClientAsyncOperation
{
    DENY_COPY(GetChaosEventsAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetChaosEventsAsyncOperation,
    CLSID_ComFabricClient_GetChaosEventsAsyncOperation,
    GetChaosEventsAsyncOperation,
    ComAsyncOperationContext)

public:
    GetChaosEventsAsyncOperation(_In_ ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetChaosEventsAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION const * publicChaosEventsDescription,
        _In_ DWORD timeoutMilliSeconds,
        _In_ IFabricAsyncOperationCallback * callback,
        _Out_ wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = chaosEventsDescription_.FromPublicApi(*publicChaosEventsDescription);

        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        _In_ IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == nullptr || result == nullptr) { return E_POINTER; }

        ComPointer<GetChaosEventsAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetChaosEventsAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricChaosEventsSegmentResult>(thisOperation->chaosEventsSegmentResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginGetChaosEvents(
            chaosEventsDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetChaosEventsComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetChaosEventsComplete(_In_ AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetChaosEvents(operation, chaosEventsSegmentResultPtr_);

        TryComplete(operation->Parent, error);
    }

    GetChaosEventsDescription chaosEventsDescription_;
    IChaosEventsSegmentResultPtr chaosEventsSegmentResultPtr_;
};

// {2933f56c-dcfa-4f14-9ba6-1d99f3ed5f44}
static const GUID CLSID_ComFabricClient_GetChaosAsyncOperation =
{ 0x2933f56c, 0xdcfa, 0x4f14, { 0x9b, 0xa6, 0x1d, 0x99, 0xf3, 0xed, 0x5f, 0x44 } };

class ComFabricClient::GetChaosAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(GetChaosAsyncOperation);
    COM_INTERFACE_AND_DELEGATE_LIST(
        GetChaosAsyncOperation,
        CLSID_ComFabricClient_GetChaosAsyncOperation,
        GetChaosAsyncOperation,
        ComAsyncOperationContext)

public:

    GetChaosAsyncOperation(_In_ ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetChaosAsyncOperation() {};

    HRESULT Initialize(
        _In_ DWORD timeoutMilliSeconds,
        _In_ IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(
        _In_ IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == nullptr || result == nullptr) { return E_POINTER; }
        ComPointer<GetChaosAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetChaosAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricChaosDescriptionResult>(thisOperation->chaosDescriptionResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginGetChaos(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetChaosComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnGetChaosComplete(_In_ AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetChaos(operation, chaosDescriptionResultPtr_);

        TryComplete(operation->Parent, error);
    }

    IChaosDescriptionResultPtr chaosDescriptionResultPtr_;
};

// {463b1bf7-17cd-4756-8e7a-1ee594f3f41a}
static const GUID CLSID_ComFabricClient_GetChaosScheduleAsyncOperation =
{ 0x463b1bf7, 0x17cd, 0x4756, { 0x8e, 0x7a, 0x1e, 0xe5, 0x94, 0xf3, 0xf4, 0x1a } };

class ComFabricClient::GetChaosScheduleAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(GetChaosScheduleAsyncOperation);
    COM_INTERFACE_AND_DELEGATE_LIST(
        GetChaosScheduleAsyncOperation,
        CLSID_ComFabricClient_GetChaosScheduleAsyncOperation,
        GetChaosScheduleAsyncOperation,
        ComAsyncOperationContext)

public:

    GetChaosScheduleAsyncOperation(_In_ ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~GetChaosScheduleAsyncOperation() {};

    HRESULT Initialize(
        _In_ DWORD timeoutMilliSeconds,
        _In_ IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(
        _In_ IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == nullptr || result == nullptr) { return E_POINTER; }
        ComPointer<GetChaosScheduleAsyncOperation> thisOperation(context, CLSID_ComFabricClient_GetChaosScheduleAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricChaosScheduleDescriptionResult>(thisOperation->chaosScheduleDescriptionResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testManagementClient_->BeginGetChaosSchedule(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetChaosScheduleComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnGetChaosScheduleComplete(_In_ AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndGetChaosSchedule(operation, chaosScheduleDescriptionResultPtr_);

        TryComplete(operation->Parent, error);
    }

    IChaosScheduleDescriptionResultPtr chaosScheduleDescriptionResultPtr_;
};


// {023188B4-48A9-43F1-8CCB-A572A30A0FDA}
static const GUID CLSID_ComFabricClient_SetChaosScheduleAsyncOperation =
{ 0x023188b4, 0x48a9, 0x34f1, { 0x8c, 0xcb, 0xa5, 0x72, 0xa3, 0x0a, 0x0f, 0xda} };

class ComFabricClient::SetChaosScheduleAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(SetChaosScheduleAsyncOperation);
    COM_INTERFACE_AND_DELEGATE_LIST(
        SetChaosScheduleAsyncOperation,
        CLSID_ComFabricClient_SetChaosScheduleAsyncOperation,
        SetChaosScheduleAsyncOperation,
        ComAsyncOperationContext)

public:

    SetChaosScheduleAsyncOperation(_In_ ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        setChaosScheduleDescription_()
    {
    }

    virtual ~SetChaosScheduleAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION const * publicSetChaosScheduleDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = setChaosScheduleDescription_.FromPublicApi(*publicSetChaosScheduleDescription);

        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        _In_ IFabricAsyncOperationContext * context)
    {
        if (context == nullptr) { return E_POINTER; }

        ComPointer<SetChaosScheduleAsyncOperation> thisOperation(context, CLSID_ComFabricClient_SetChaosScheduleAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr) override
    {
        auto operation = Owner.testManagementClient_->BeginSetChaosSchedule(
            move(setChaosScheduleDescription_),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnComplete(_In_ AsyncOperationSPtr const& operation)
    {
        auto error = Owner.testManagementClient_->EndSetChaosSchedule(operation);
        TryComplete(operation->Parent, error);
    }

    SetChaosScheduleDescription setChaosScheduleDescription_;
};

/// IFaultManagementClient

// {A37D2E37-4554-4DF0-8717-EF7F09409DC9}
static const GUID CLSID_ComFabricClient_RestartNodeAsyncOperation =
{ 0xa37d2e37, 0x4554, 0x4df0, { 0x87, 0x17, 0xef, 0x7f, 0x9, 0x40, 0x9d, 0xc9 } };

class ComFabricClient::RestartNodeAsyncOperation : public ClientAsyncOperation
{
    DENY_COPY(RestartNodeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    RestartNodeAsyncOperation,
    CLSID_ComFabricClient_RestartNodeAsyncOperation,
    RestartNodeAsyncOperation,
    ComAsyncOperationContext)

public:
    RestartNodeAsyncOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
    {
    }

    virtual ~RestartNodeAsyncOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR nodeName,
        FABRIC_NODE_INSTANCE_ID instanceId,
        bool shouldCreateFabricDump,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        instanceId_ = wformatString(instanceId);
        shouldCreateFabricDump_ = shouldCreateFabricDump;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RestartNodeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RestartNodeAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricRestartNodeResult>(thisOperation->restartNodeResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.faultManagementClient_->BeginRestartNode(
            nodeName_,
            instanceId_,
            shouldCreateFabricDump_,
            Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRestartComplete(operation, nodeName_, instanceId_); },
            proxySPtr);
    }

private:
    void OnRestartComplete(
        __in AsyncOperationSPtr const & operation,
        wstring const & nodeName,
        wstring const & instanceId)
    {
        auto error = this->Owner.faultManagementClient_->EndRestartNode(operation, nodeName, instanceId, restartNodeResultPtr_);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    wstring instanceId_;
    bool shouldCreateFabricDump_;
    IRestartNodeResultPtr restartNodeResultPtr_;
};

// {9856416C-F27B-4233-B5F3-0B2DD422D9B3}
static const GUID CLSID_ComFabricClient_StartNodeAsyncOperation =
{ 0x9856416c, 0xf27b, 0x4233, { 0xb5, 0xf3, 0xb, 0x2d, 0xd4, 0x22, 0xd9, 0xb3 } };

class ComFabricClient::StartNodeAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(StartNodeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    StartNodeAsyncOperation,
    CLSID_ComFabricClient_StartNodeAsyncOperation,
    StartNodeAsyncOperation,
    ComAsyncOperationContext)

public:
    StartNodeAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        startNodeDescription_()
    {
    }

    virtual ~StartNodeAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME * publicStartNodeDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = startNodeDescription_.FromPublicApi(*publicStartNodeDescription);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<StartNodeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StartNodeAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricStartNodeResult>(thisOperation->startNodeResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.faultManagementClient_->BeginStartNode(
            startNodeDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation, startNodeDescription_);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation, StartNodeDescriptionUsingNodeName const & startNodeDescription)
    {
        auto error = Owner.faultManagementClient_->EndStartNode(operation, startNodeDescription, startNodeResultPtr_);
        TryComplete(operation->Parent, error);
    }

    StartNodeDescriptionUsingNodeName startNodeDescription_;
    IStartNodeResultPtr startNodeResultPtr_;
};

// {0E358659-7B06-4F0D-B6C7-D2202687AD4A}
static const GUID CLSID_ComFabricClient_StopNodeAsyncOperation =
{ 0xe358659, 0x7b06, 0x4f0d, { 0xb6, 0xc7, 0xd2, 0x20, 0x26, 0x87, 0xad, 0x4a } };


class ComFabricClient::StopNodeAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(StopNodeAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    StopNodeAsyncOperation,
    CLSID_ComFabricClient_StopNodeAsyncOperation,
    StopNodeAsyncOperation,
    ComAsyncOperationContext)

public:
    StopNodeAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        stopNodeDescription_()
    {
    }

    virtual ~StopNodeAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME * publicStopNodeDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = stopNodeDescription_.FromPublicApi(*publicStopNodeDescription);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<StopNodeAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StopNodeAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricStopNodeResult>(thisOperation->stopNodeResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.faultManagementClient_->BeginStopNode(
            stopNodeDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation, stopNodeDescription_);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation, StopNodeDescriptionUsingNodeName const & stopNodeDescription)
    {
        auto error = Owner.faultManagementClient_->EndStopNode(operation, stopNodeDescription, stopNodeResultPtr_);
        TryComplete(operation->Parent, error);
    }

    StopNodeDescriptionUsingNodeName stopNodeDescription_;
    IStopNodeResultPtr stopNodeResultPtr_;
};

// {1AC3C2FF-79B3-474B-A3EF-4E962E66B180}
static const GUID CLSID_ComFabricClient_StopNodeInternalAsyncOperation =
{ 0x1ac3c2ff, 0x79b3, 0x474b, { 0xa3, 0xef, 0x4e, 0x96, 0x2e, 0x66, 0xb1, 0x80 } };

class ComFabricClient::StopNodeInternalAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(StopNodeInternalAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        StopNodeInternalAsyncOperation,
        CLSID_ComFabricClient_StopNodeInternalAsyncOperation,
        StopNodeInternalAsyncOperation,
        ComAsyncOperationContext)

public:
    StopNodeInternalAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        stopNodeDescription_()
    {
    }

    virtual ~StopNodeInternalAsyncOperation() {};

    HRESULT Initialize(
        const FABRIC_STOP_NODE_DESCRIPTION_INTERNAL * publicStopNodeDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StopNodeInternalAsyncOperation::Initialize"
            );

        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = stopNodeDescription_.FromPublicApi(*publicStopNodeDescription);

        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StopNodeInternalAsyncOperation::Initialize, after FromPublicApi"
            );

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StopNodeInternalAsyncOperation> thisOperation(context, CLSID_ComFabricClient_StopNodeInternalAsyncOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Trace.WriteInfo(
            TraceComponent,
            "ComFabricClient::StopNodeInternalAsyncOperation::OnStart, from internal api"
            );
        auto operation = Owner.faultManagementClient_->BeginStopNodeInternal(
            stopNodeDescription_.NodeName,
            wformatString(stopNodeDescription_.NodeInstanceId),
            stopNodeDescription_.StopDurationInSeconds,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        Trace.WriteNoise(
            TraceComponent,
            "ComFabricClient::StopNodeInternalAsyncOperation::OnComplete calling EndStopNode");

        auto error = Owner.faultManagementClient_->EndStopNodeInternal(operation);
        TryComplete(operation->Parent, error);
    }

    StopNodeDescriptionInternal stopNodeDescription_;
    IStopNodeResultPtr stopNodeResultPtr_;
};


// {BB80E36A-A963-4FBE-B768-0A04111C98DC}
static const GUID CLSID_ComFabricClient_RestartDeployedCodePackageAsyncOperation =
{ 0xbb80e36a, 0xa963, 0x4fbe, { 0xb7, 0x68, 0xa, 0x4, 0x11, 0x1c, 0x98, 0xdc } };

class ComFabricClient::RestartDeployedCodePackageAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(RestartDeployedCodePackageAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    RestartDeployedCodePackageAsyncOperation,
    CLSID_ComFabricClient_RestartDeployedCodePackageAsyncOperation,
    RestartDeployedCodePackageAsyncOperation,
    ComAsyncOperationContext)

public:
    RestartDeployedCodePackageAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        restartDeployedCodePackageDescription_()
    {
    }

    virtual ~RestartDeployedCodePackageAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME * publicRestartDeployedCodePackageDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = restartDeployedCodePackageDescription_.FromPublicApi(*publicRestartDeployedCodePackageDescription);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<RestartDeployedCodePackageAsyncOperation> thisOperation(context, CLSID_ComFabricClient_RestartDeployedCodePackageAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricRestartDeployedCodePackageResult>(thisOperation->restartDeployedCodePackageResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.faultManagementClient_->BeginRestartDeployedCodePackage(
            restartDeployedCodePackageDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation, restartDeployedCodePackageDescription_);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation, RestartDeployedCodePackageDescriptionUsingNodeName const & restartDeployedCodePackageDescription)
    {
        auto error = Owner.faultManagementClient_->EndRestartDeployedCodePackage(operation, restartDeployedCodePackageDescription, restartDeployedCodePackageResultPtr_);
        TryComplete(operation->Parent, error);
    }

    RestartDeployedCodePackageDescriptionUsingNodeName restartDeployedCodePackageDescription_;
    IRestartDeployedCodePackageResultPtr restartDeployedCodePackageResultPtr_;
};

// MovePrimaryAsyncOperation
// {21D22C52-B0A2-47F0-A048-8EA7E876FEA2}
static const GUID CLSID_ComFabricClient_MovePrimaryAsyncOperation =
{ 0x21d22c52, 0xb0a2, 0x47f0, { 0xa0, 0x48, 0x8e, 0xa7, 0xe8, 0x76, 0xfe, 0xa2 } };

class ComFabricClient::MovePrimaryAsyncOperation : public ClientAsyncOperation
{
    DENY_COPY(MovePrimaryAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    MovePrimaryAsyncOperation,
    CLSID_ComFabricClient_MovePrimaryAsyncOperation,
    MovePrimaryAsyncOperation,
    ComAsyncOperationContext)

public:
    MovePrimaryAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        movePrimaryDescription_()
    {
    }

    virtual ~MovePrimaryAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME * publicMovePrimaryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = movePrimaryDescription_.FromPublicApi(*publicMovePrimaryDescription);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<MovePrimaryAsyncOperation> thisOperation(context, CLSID_ComFabricClient_MovePrimaryAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricMovePrimaryResult>(thisOperation->movePrimaryResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.faultManagementClient_->BeginMovePrimary(
            movePrimaryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation, movePrimaryDescription_);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation, MovePrimaryDescriptionUsingNodeName const & movePrimaryDescription)
    {
        auto error = Owner.faultManagementClient_->EndMovePrimary(operation, movePrimaryDescription, movePrimaryResultPtr_);
        TryComplete(operation->Parent, error);
    }

    MovePrimaryDescriptionUsingNodeName movePrimaryDescription_;
    IMovePrimaryResultPtr movePrimaryResultPtr_;
};

// MoveSecondaryAsyncOperation
// {CBABBAB4-3853-406C-BCD6-DCF841F11D3D}
static const GUID CLSID_ComFabricClient_MoveSecondaryAsyncOperation =
{ 0xcbabbab4, 0x3853, 0x406c, { 0xbc, 0xd6, 0xdc, 0xf8, 0x41, 0xf1, 0x1d, 0x3d } };

class ComFabricClient::MoveSecondaryAsyncOperation :
public ClientAsyncOperation
{
    DENY_COPY(MoveSecondaryAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    MoveSecondaryAsyncOperation,
    CLSID_ComFabricClient_MoveSecondaryAsyncOperation,
    MoveSecondaryAsyncOperation,
    ComAsyncOperationContext)

public:
    MoveSecondaryAsyncOperation(__in ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        moveSecondaryDescription_()
    {
    }

    virtual ~MoveSecondaryAsyncOperation() {};

    HRESULT Initialize(
        FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME * publicMoveSecondaryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = moveSecondaryDescription_.FromPublicApi(*publicMoveSecondaryDescription);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        REFIID riid,
        void ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<MoveSecondaryAsyncOperation> thisOperation(context, CLSID_ComFabricClient_MoveSecondaryAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto comPtr = make_com<ComFabricMoveSecondaryResult>(thisOperation->moveSecondaryResultPtr_);

            hr = comPtr->QueryInterface(riid, result);
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.faultManagementClient_->BeginMoveSecondary(
            moveSecondaryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnComplete(operation, moveSecondaryDescription_);
        },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation, MoveSecondaryDescriptionUsingNodeName const & moveSecondaryDescription)
    {
        auto error = Owner.faultManagementClient_->EndMoveSecondary(operation, moveSecondaryDescription, moveSecondaryResultPtr_);
        TryComplete(operation->Parent, error);
    }

    MoveSecondaryDescriptionUsingNodeName moveSecondaryDescription_;
    IMoveSecondaryResultPtr moveSecondaryResultPtr_;
};

// { 142623de-3e98-4bd8-a64b-ed83e2963858}
static const GUID CLSID_ComFabricClient_ResolvePartitionAsyncOperation =
{ 0x142623de, 0x3e98, 0x4bd8, {0xa6, 0x4b, 0xed, 0x83, 0xe2, 0x96, 0x38, 0x58} };

class ComFabricClient::ResolvePartitionAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(ResolvePartitionAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ResolvePartitionAsyncOperation,
        CLSID_ComFabricClient_ResolvePartitionAsyncOperation,
        ResolvePartitionAsyncOperation ,
        ComAsyncOperationContext)

public:
    ResolvePartitionAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ResolvePartitionAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_PARTITION_ID *partitionId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        id_ = *partitionId;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricResolvedServicePartitionResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<ResolvePartitionAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ResolvePartitionAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            thisOperation->resultCPtr_->QueryInterface(IID_IFabricResolvedServicePartitionResult, reinterpret_cast<void**>(result));
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testClient_->BeginResolvePartition(
            Reliability::ConsistencyUnitId(Common::Guid(id_)),
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        IResolvedServicePartitionResultPtr resultPtr;
        auto error = Owner.testClient_->EndResolvePartition(operation, resultPtr);
        if (error.IsSuccess())
        {
            resultCPtr_ = make_com<ComResolvedServicePartitionResult, IFabricResolvedServicePartitionResult>(resultPtr);
        }
        TryComplete(operation->Parent, error);
    }

    ComPointer<IFabricResolvedServicePartitionResult> resultCPtr_;
    FABRIC_PARTITION_ID id_;
};

// { 752bcef7-20d1-4d34-9ebd-3817caaec48f}
static const GUID CLSID_ComFabricClient_ResolveNameOwnerAsyncOperation =
{ 0x752bcef7, 0x20d1, 0x4d34, {0x9e, 0xbd, 0x38, 0x17, 0xca, 0xae, 0xc4, 0x8f} };

class ComFabricClient::ResolveNameOwnerAsyncOperation :
    public ClientAsyncOperation
{
    DENY_COPY(ResolveNameOwnerAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        ResolveNameOwnerAsyncOperation,
        CLSID_ComFabricClient_ResolveNameOwnerAsyncOperation,
        ResolveNameOwnerAsyncOperation ,
        ComAsyncOperationContext)
public:
    ResolveNameOwnerAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
    {
    }

    virtual ~ResolveNameOwnerAsyncOperation() {};

    HRESULT Initialize(
        __in FABRIC_URI name,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(name, Constants::FabricClientTrace, name_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricResolvedServicePartitionResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<ResolveNameOwnerAsyncOperation> thisOperation(context, CLSID_ComFabricClient_ResolveNameOwnerAsyncOperation);
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            thisOperation->resultCPtr_->QueryInterface(IID_IFabricResolvedServicePartitionResult, reinterpret_cast<void**>(result));
        }

        return hr;
    }

protected:

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.testClient_->BeginResolveNameOwner(
            name_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnComplete(__in AsyncOperationSPtr const& operation)
    {
        IResolvedServicePartitionResultPtr resultPtr;
        auto error = Owner.testClient_->EndResolveNameOwner(operation, resultPtr);
        if (error.IsSuccess())
        {
            resultCPtr_ = make_com<ComResolvedServicePartitionResult, IFabricResolvedServicePartitionResult>(resultPtr);
        }
        TryComplete(operation->Parent, error);
    }

    ComPointer<IFabricResolvedServicePartitionResult> resultCPtr_;
    NamingUri name_;
};

// ac980e62-563b-4232-9307-305de504bfa2
static const GUID CLSID_ComFabricClient_GetClusterHealthChunkOperation =
{ 0xac980e62, 0x563b, 0x4232,{ 0x93, 0x07, 0x30, 0x5d, 0xe5, 0x04, 0xbf, 0xa2 } };
class ComFabricClient::GetClusterHealthChunkOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetClusterHealthChunkOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetClusterHealthChunkOperation,
        CLSID_ComFabricClient_GetClusterHealthChunkOperation,
        GetClusterHealthChunkOperation,
        ComAsyncOperationContext)

public:
    GetClusterHealthChunkOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , clusterHealthChunk_()
    {
    }

    virtual ~GetClusterHealthChunkOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback,
        __out wstring & errorMessage)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            errorMessage = error.TakeMessage();
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricGetClusterHealthChunkResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetClusterHealthChunkOperation> thisOperation(context, CLSID_ComFabricClient_GetClusterHealthChunkOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto resultCPtr = make_com<ComGetClusterHealthChunkResult, IFabricGetClusterHealthChunkResult>(thisOperation->clusterHealthChunk_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetClusterHealthChunk(
            move(queryDescription_),
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetClusterHealthChunkComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetClusterHealthChunkComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetClusterHealthChunk(operation, /*out*/ clusterHealthChunk_);
        this->TryComplete(operation->Parent, error);
    }

    ClusterHealthChunkQueryDescription queryDescription_;
    ClusterHealthChunk clusterHealthChunk_;
};

// FFA6367D-86FA-4D78-9E5E-D21D789D588A
static const GUID CLSID_ComFabricClient_GetClusterHealthOperation =
{ 0xffa6367d, 0x86fa, 0x4d78, { 0x9e, 0x5e, 0xd2, 0x1d, 0x78, 0x9d, 0x58, 0x8a } };
class ComFabricClient::GetClusterHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetClusterHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetClusterHealthOperation,
        CLSID_ComFabricClient_GetClusterHealthOperation,
        GetClusterHealthOperation,
        ComAsyncOperationContext)

public:
    GetClusterHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , clusterHealth_()
    {
    }

    virtual ~GetClusterHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricClusterHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetClusterHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetClusterHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricClusterHealthResult> resultCPtr =
                make_com<ComClusterHealthResult, IFabricClusterHealthResult>(thisOperation->clusterHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetClusterHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetClusterHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetClusterHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetClusterHealth(operation, /*out*/ clusterHealth_);
        this->TryComplete(operation->Parent, error);
    }

    ClusterHealthQueryDescription queryDescription_;
    ClusterHealth clusterHealth_;
};

// 4c8447c1-33b4-4387-8e4c-2e5d62864115
static const GUID CLSID_ComFabricClient_GetNodeHealthOperation =
    { 0x4c8447c1, 0x33b4, 0x4387, {0x8e, 0x4c, 0x2e, 0x5d, 0x62, 0x86, 0x41, 0x15} };
class ComFabricClient::GetNodeHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetNodeHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetNodeHealthOperation,
        CLSID_ComFabricClient_GetNodeHealthOperation,
        GetNodeHealthOperation,
        ComAsyncOperationContext)

public:
    GetNodeHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , nodeHealth_()
    {
    }

    virtual ~GetNodeHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_NODE_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricNodeHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetNodeHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetNodeHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricNodeHealthResult> resultCPtr =
                make_com<ComNodeHealthResult, IFabricNodeHealthResult>(thisOperation->nodeHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetNodeHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetNodeHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetNodeHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetNodeHealth(operation, /*out*/ nodeHealth_);
        this->TryComplete(operation->Parent, error);
    }

    NodeHealthQueryDescription queryDescription_;
    NodeHealth nodeHealth_;
};

// 792e9c40-b90a-43ba-b3a1-c74ab942f653
static const GUID CLSID_ComFabricClient_GetApplicationHealthOperation =
    { 0x792e9c40, 0xb90a, 0x43ba, {0xb3, 0xa1, 0xc7, 0x4a, 0xb9, 0x42, 0xf6, 0x53} };
class ComFabricClient::GetApplicationHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetApplicationHealthOperation,
        CLSID_ComFabricClient_GetApplicationHealthOperation,
        GetApplicationHealthOperation,
        ComAsyncOperationContext)

public:
    GetApplicationHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , applicationHealth_()
    {
    }

    virtual ~GetApplicationHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricApplicationHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricApplicationHealthResult> resultCPtr =
                make_com<ComApplicationHealthResult, IFabricApplicationHealthResult>(thisOperation->applicationHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetApplicationHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetApplicationHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetApplicationHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetApplicationHealth(operation, /*out*/ applicationHealth_);
        this->TryComplete(operation->Parent, error);
    }

    ApplicationHealthQueryDescription queryDescription_;
    ApplicationHealth applicationHealth_;
};

// bda2ce14-dd6a-467d-9555-71ba2cf9a9f5
static const GUID CLSID_ComFabricClient_AddUnreliableTransportBehaviorOperation =
    { 0xbda2ce14, 0xdd6a, 0x467d, { 0x95, 0x55, 0x71, 0xba, 0x2c, 0xf9, 0xa9, 0xf5 } };
class ComFabricClient::AddUnreliableTransportBehaviorOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(AddUnreliableTransportBehaviorOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    AddUnreliableTransportBehaviorOperation,
    CLSID_ComFabricClient_AddUnreliableTransportBehaviorOperation,
    AddUnreliableTransportBehaviorOperation,
    ComAsyncOperationContext)

public:
    AddUnreliableTransportBehaviorOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
        , name_()
        , behaviorString_()
    {
    }

    virtual ~AddUnreliableTransportBehaviorOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR nodeName,
        LPCWSTR name,
        const FABRIC_UNRELIABLETRANSPORT_BEHAVIOR & behavior,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(name, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, name_);
        if (FAILED(hr)) { return hr; }

        wstring destinationString;
        hr = StringUtility::LpcwstrToWstring(behavior.Destination, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, destinationString);
        if (FAILED(hr)) { return hr; }

        wstring actionString;
        hr = StringUtility::LpcwstrToWstring(behavior.Action, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, actionString);
        if (FAILED(hr)) { return hr; }

        // probability out of this range will cause an assertion in UnreliableTransportSpecification
        if (behavior.ProbabilityToApply <= 0.0 || behavior.ProbabilityToApply > 1.0) { return E_INVALIDARG; }

        // the current UnreliableTransportSettings.ini format separates behavior from rest with a = sign.
        // Changing the design from INI to XML should solve this problem
        if (name_.find(L'=') != wstring::npos) { return E_INVALIDARG; }


        Federation::NodeId sourceNodeId;
        auto error = Federation::NodeIdGenerator::GenerateFromString(nodeName, sourceNodeId);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        wstring destinationNodeId = destinationString;
        if (!StringUtility::AreEqualCaseInsensitive(destinationNodeId, L"*"))
        {
            Federation::NodeId nodeId;
            error = Federation::NodeIdGenerator::GenerateFromString(destinationNodeId, nodeId);
            if (!error.IsSuccess())
            {
                return error.ToHResult();
            }

            destinationNodeId = nodeId.ToString();
        }

        wstring filters;
        if (behavior.Filters != NULL)
        {
            for (ULONG i = 0; i < behavior.Filters->Count; i++)
            {
                wstring filterName;
                hr = StringUtility::LpcwstrToWstring(behavior.Filters->Items[i].Name, false, filterName);
                if (FAILED(hr)) { return hr; }

                wstring value;
                hr = StringUtility::LpcwstrToWstring(behavior.Filters->Items[i].Value, false, value);
                if (FAILED(hr)) { return hr; }

                filters += wformatString("{0}:{1},", filterName, value);
            }
        }

        // trim "," from end of filters
        if (filters.length() > 0)
        {
            filters.pop_back();
        }

        behaviorString_ = wformatString("{0} {1} {2} {3} {4} {5} {6} {7} {8}",
            sourceNodeId,
            destinationNodeId,
            actionString,
            behavior.ProbabilityToApply,
            behavior.DelayInSeconds,
            behavior.DelaySpanInSeconds,
            behavior.Priority,
            behavior.ApplyCount,
            filters);

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<AddUnreliableTransportBehaviorOperation> thisOperation(context, CLSID_ComFabricClient_AddUnreliableTransportBehaviorOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginAddUnreliableTransportBehavior(
            nodeName_,
            name_,
            behaviorString_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnAddUnreliableTransportBehavior(operation); },
            proxySPtr);
    }

private:
    void OnAddUnreliableTransportBehavior(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.clusterMgmtClient_->EndAddUnreliableTransportBehavior(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    wstring name_;
    wstring behaviorString_;
};


// 15195176-5a89-45d1-a3c9-d2760762cd05
static const GUID CLSID_ComFabricClient_RemoveUnreliableTransportBehaviorOperation =
    { 0x15195176, 0x5a89, 0x45d1, { 0xa3, 0xc9, 0xd2, 0x76, 0x07, 0x62, 0xcd, 0x05 } };
class ComFabricClient::RemoveUnreliableTransportBehaviorOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RemoveUnreliableTransportBehaviorOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    RemoveUnreliableTransportBehaviorOperation,
    CLSID_ComFabricClient_RemoveUnreliableTransportBehaviorOperation,
    RemoveUnreliableTransportBehaviorOperation,
    ComAsyncOperationContext)

public:
    RemoveUnreliableTransportBehaviorOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
        , name_()
    {
    }

    virtual ~RemoveUnreliableTransportBehaviorOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR nodeName,
        LPCWSTR name,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(name, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, name_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RemoveUnreliableTransportBehaviorOperation> thisOperation(context, CLSID_ComFabricClient_RemoveUnreliableTransportBehaviorOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginRemoveUnreliableTransportBehavior(
            nodeName_,
            name_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRemoveUnreliableTransportBehavior(operation); },
            proxySPtr);
    }

private:
    void OnRemoveUnreliableTransportBehavior(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.clusterMgmtClient_->EndRemoveUnreliableTransportBehavior(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    wstring name_;
};

// d0ea9398-ef47-4c9b-a406-04bb1fe0e60e
static const GUID CLSID_ComFabricClient_AddUnreliableLeaseBehaviorOperation =
    { 0xd0ea9398, 0xef47, 0x4c9b, { 0xa4, 0x06, 0x04, 0xbb, 0x1f, 0xe0, 0xe6, 0x0e } };
    class ComFabricClient::AddUnreliableLeaseBehaviorOperation :
    public ComFabricClient::ClientAsyncOperation
    {
        DENY_COPY(AddUnreliableLeaseBehaviorOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
        AddUnreliableLeaseBehaviorOperation,
        CLSID_ComFabricClient_AddUnreliableLeaseBehaviorOperation,
        AddUnreliableLeaseBehaviorOperation,
        ComAsyncOperationContext)

    public:
        AddUnreliableLeaseBehaviorOperation(__in ComFabricClient & owner)
            : ClientAsyncOperation(owner)
            , nodeName_()
            , alias_()
            , behaviorString_()
        {
        }

        virtual ~AddUnreliableLeaseBehaviorOperation()
        {
        }

        HRESULT Initialize(
            LPCWSTR nodeName,
            LPCWSTR behaviorString,
            LPCWSTR alias,
            DWORD timeoutMilliSeconds,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
            if (FAILED(hr)) { return hr; }

            hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
            if (FAILED(hr)) { return hr; }

            hr = StringUtility::LpcwstrToWstring(alias, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, alias_);
            if (FAILED(hr)) { return hr; }

            hr = StringUtility::LpcwstrToWstring(behaviorString, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, behaviorString_);
            if (FAILED(hr)) { return hr; }

            return S_OK;
        }

        static HRESULT End(
            __in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<AddUnreliableLeaseBehaviorOperation> thisOperation(context, CLSID_ComFabricClient_AddUnreliableLeaseBehaviorOperation);

            auto hr = thisOperation->ComAsyncOperationContextEnd();

            return hr;
        }

        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            Owner.testManagementClientInternal_->BeginAddUnreliableLeaseBehavior(
                nodeName_,
                behaviorString_,
                alias_,
                this->Timeout,
                [this](AsyncOperationSPtr const & operation) { this->OnAddUnreliableLeaseBehavior(operation); },
                proxySPtr);
        }


private:
    void OnAddUnreliableLeaseBehavior(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.testManagementClientInternal_->EndAddUnreliableLeaseBehavior(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    wstring alias_;
    wstring behaviorString_;
};

// 4165a459-6d96-4628-a747-97dfbb547baf
static const GUID CLSID_ComFabricClient_RemoveUnreliableLeaseBehaviorOperation =
    { 0x4165a459, 0x6d96, 0x4628, { 0xa7, 0x47, 0x97, 0xdf, 0xbb, 0x54, 0x7b, 0xaf } };
class ComFabricClient::RemoveUnreliableLeaseBehaviorOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RemoveUnreliableLeaseBehaviorOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    RemoveUnreliableLeaseBehaviorOperation,
    CLSID_ComFabricClient_RemoveUnreliableLeaseBehaviorOperation,
    RemoveUnreliableLeaseBehaviorOperation,
    ComAsyncOperationContext)

public:
    RemoveUnreliableLeaseBehaviorOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
        , alias_()
    {
    }

    virtual ~RemoveUnreliableLeaseBehaviorOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR nodeName,
        LPCWSTR alias,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(alias, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, alias_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RemoveUnreliableLeaseBehaviorOperation> thisOperation(context, CLSID_ComFabricClient_RemoveUnreliableLeaseBehaviorOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.testManagementClientInternal_->BeginRemoveUnreliableLeaseBehavior(
            nodeName_,
            alias_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRemoveUnreliableLeaseBehavior(operation); },
            proxySPtr);
    }

private:
    void OnRemoveUnreliableLeaseBehavior(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.testManagementClientInternal_->EndRemoveUnreliableLeaseBehavior(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    wstring alias_;
};

// be479a04-845c-4d9b-afbc-f0c7fc1399f8
static const GUID CLSID_ComFabricClient_StopNodeOperation =
    { 0xbe479a04, 0x845c, 0x4d9b, {0xaf, 0xbc, 0xf0, 0xc7, 0xfc, 0x13, 0x99, 0xf8} };
class ComFabricClient::StopNodeOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(StopNodeOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        StopNodeOperation,
        CLSID_ComFabricClient_StopNodeOperation,
        StopNodeOperation,
        ComAsyncOperationContext)

public:
    StopNodeOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
    {
    }

    virtual ~StopNodeOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR nodeName,
        FABRIC_NODE_INSTANCE_ID instanceId,
        bool restart,
        bool createFabricDump,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        instanceId_ = wformatString(instanceId);

        restart_ = restart;
        createFabricDump_ = createFabricDump;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StopNodeOperation> thisOperation(context, CLSID_ComFabricClient_StopNodeOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginStopNode(
            nodeName_,
            instanceId_,
            restart_,
            createFabricDump_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnStopComplete(operation); },
            proxySPtr);
    }

private:
    void OnStopComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.clusterMgmtClient_->EndStopNode(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    wstring instanceId_;
    bool restart_;
    bool createFabricDump_;
};

// b8166e60-378a-4a40-998b-a1ad6f5498ef
static const GUID CLSID_ComFabricClient_StartNodeOperation =
    { 0xb8166e60, 0x378a, 0x4a40, {0x99, 0x8b, 0xa1, 0xad, 0x6f, 0x54, 0x98, 0xef} };
class ComFabricClient::StartNodeOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(StartNodeOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        StartNodeOperation,
        CLSID_ComFabricClient_StartNodeOperation,
        StartNodeOperation,
        ComAsyncOperationContext)

public:
    StartNodeOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
    {
    }

    virtual ~StartNodeOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR nodeName,
        FABRIC_NODE_INSTANCE_ID instanceId,
        LPCWSTR ipAddressOrFQDN,
        ULONG clusterConnectionPort,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(ipAddressOrFQDN, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, ipAddressOrFQDN_);
        if (FAILED(hr)) { return hr; }

        instanceId_ = instanceId;
        clusterConnectionPort_ = clusterConnectionPort;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<StartNodeOperation> thisOperation(context, CLSID_ComFabricClient_StartNodeOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginStartNode(
            nodeName_,
            instanceId_,
            ipAddressOrFQDN_,
            clusterConnectionPort_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnStartComplete(operation); },
            proxySPtr);
    }

private:
    void OnStartComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.clusterMgmtClient_->EndStartNode(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    uint64 instanceId_;
    wstring ipAddressOrFQDN_;
    ULONG clusterConnectionPort_;
};

    // {31B5ABA4-4BA0-4FE8-8AAD-C4633C23A611}
    static const GUID CLSID_ComFabricClient_MovePrimaryOperation =
    { 0x31b5aba4, 0x4ba0, 0x4fe8, { 0x8a, 0xad, 0xc4, 0x63, 0x3c, 0x23, 0xa6, 0x11 } };
    class ComFabricClient::MovePrimaryOperation :
    public ComFabricClient::ClientAsyncOperation
    {
        DENY_COPY(MovePrimaryOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
        MovePrimaryOperation,
        CLSID_ComFabricClient_MovePrimaryOperation,
        MovePrimaryOperation,
        ComAsyncOperationContext)

    public:
        MovePrimaryOperation(__in ComFabricClient & owner)
            : ClientAsyncOperation(owner)
            , nodeName_()
            , partitionId_()
            , ignoreConstraints_(false)
        {
        }

        virtual ~MovePrimaryOperation()
        {
        }

        HRESULT Initialize(
            __in const FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION * movePrimaryDesc,
            __in DWORD timeoutMilliSeconds,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
            if (FAILED(hr)) { return hr; }

            hr = StringUtility::LpcwstrToWstring(movePrimaryDesc->NodeName, false, 0 , ParameterValidator::MaxStringSize, nodeName_);
            if (FAILED(hr)) { return hr; }

            partitionId_ = Guid(movePrimaryDesc->PartitionId);

            if (movePrimaryDesc->Reserved != NULL)
            {
                FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION_EX1* descExt = static_cast<FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION_EX1*>(movePrimaryDesc->Reserved);
                ignoreConstraints_ = (descExt->IgnoreConstraints == 1) ? true : false;
            }
            else
            {
                ignoreConstraints_ = false;
            }


            return S_OK;
        }

        static HRESULT End(
            __in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<MovePrimaryOperation> thisOperation(context, CLSID_ComFabricClient_MovePrimaryOperation);

            auto hr = thisOperation->ComAsyncOperationContextEnd();

            return hr;
        }

        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            this->Owner.serviceMgmtClient_->BeginMovePrimary(
                nodeName_,
                partitionId_,
                ignoreConstraints_,
                this->Timeout,
                [this](AsyncOperationSPtr const & operation){ this->OnMovePrimaryComplete(operation); },
                proxySPtr);
        }

    private:
        void OnMovePrimaryComplete(AsyncOperationSPtr const & operation)
        {
            auto error = this->Owner.serviceMgmtClient_->EndMovePrimary(operation);
            this->TryComplete(operation->Parent, error);
        }

        wstring nodeName_;
        Common::Guid partitionId_;
        bool ignoreConstraints_;
    };

    // {B052637A-33BA-42D0-BB50-37765F363DCD}
    static const GUID CLSID_ComFabricClient_MoveSecondaryOperation =
    { 0xb052637a, 0x33ba, 0x42d0, { 0xbb, 0x50, 0x37, 0x76, 0x5f, 0x36, 0x3d, 0xcd } };
    class ComFabricClient::MoveSecondaryOperation :
    public ComFabricClient::ClientAsyncOperation
    {
        DENY_COPY(MoveSecondaryOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
        MoveSecondaryOperation,
        CLSID_ComFabricClient_MoveSecondaryOperation,
        MoveSecondaryOperation,
        ComAsyncOperationContext)

    public:
        MoveSecondaryOperation(__in ComFabricClient & owner)
            : ClientAsyncOperation(owner)
            , currentNodeName_()
            , newNodeName_()
            , partitionId_()
            , ignoreConstraints_(false)
        {
        }

        virtual ~MoveSecondaryOperation()
        {
        }

        HRESULT Initialize(
            __in const FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION * moveSecondaryDesc,
            __in DWORD timeoutMilliSeconds,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
            if (FAILED(hr)) { return hr; }

            hr = StringUtility::LpcwstrToWstring(moveSecondaryDesc->CurrentSecondaryNodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, currentNodeName_);
            if (FAILED(hr)) { return hr; }

            hr = StringUtility::LpcwstrToWstring(moveSecondaryDesc->NewSecondaryNodeName, false, 0, ParameterValidator::MaxStringSize, newNodeName_);
            if (FAILED(hr)) { return hr; }

            partitionId_ = Guid(moveSecondaryDesc->PartitionId);

            if (moveSecondaryDesc->Reserved != NULL)
            {
                FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION_EX1* descExt = static_cast<FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION_EX1*>(moveSecondaryDesc->Reserved);
                ignoreConstraints_ = (descExt->IgnoreConstraints == 1) ? true : false;
            }
            else
            {
                ignoreConstraints_ = false;
            }


            return S_OK;
        }

        static HRESULT End(
            __in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<MoveSecondaryOperation> thisOperation(context, CLSID_ComFabricClient_MoveSecondaryOperation);

            auto hr = thisOperation->ComAsyncOperationContextEnd();

            return hr;
        }

        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            this->Owner.serviceMgmtClient_->BeginMoveSecondary(
                currentNodeName_,
                newNodeName_,
                partitionId_,
                ignoreConstraints_,
                this->Timeout,
                [this](AsyncOperationSPtr const & operation){ this->OnMoveSecondaryComplete(operation); },
                proxySPtr);
        }

    private:
        void OnMoveSecondaryComplete(AsyncOperationSPtr const & operation)
        {
            auto error = this->Owner.serviceMgmtClient_->EndMoveSecondary(operation);
            this->TryComplete(operation->Parent, error);
        }

        wstring currentNodeName_;
        wstring newNodeName_;
        Common::Guid partitionId_;
        bool ignoreConstraints_;
    };

// 6aefc0f8-9f08-44a1-96b6-d2b5799aa2d9
static const GUID CLSID_ComFabricClient_RestartDeployedCodePackageOperation =
    { 0x6aefc0f8, 0x9f08, 0x44a1, {0x96, 0xb6, 0xd2, 0xb5, 0x79, 0x9a, 0xa2, 0xd2} };
class ComFabricClient::RestartDeployedCodePackageOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RestartDeployedCodePackageOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RestartDeployedCodePackageOperation,
        CLSID_ComFabricClient_RestartDeployedCodePackageOperation,
        RestartDeployedCodePackageOperation,
        ComAsyncOperationContext)

public:
    RestartDeployedCodePackageOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
        , applicationName_()
        , serviceManifestName_()
        , codePackageName_()
    {
    }

    virtual ~RestartDeployedCodePackageOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR nodeName,
        FABRIC_URI applicationName,
        LPCWSTR serviceManifestName,
        LPCWSTR codePackageName,
        FABRIC_INSTANCE_ID codePackageInstanceId,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(applicationName, Constants::FabricClientTrace, applicationName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(serviceManifestName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceManifestName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(codePackageName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, codePackageName_);
        if (FAILED(hr)) { return hr; }

        instanceId_ = wformatString(codePackageInstanceId);

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RestartDeployedCodePackageOperation> thisOperation(context, CLSID_ComFabricClient_RestartDeployedCodePackageOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.appMgmtClient_->BeginRestartDeployedCodePackage(
            nodeName_,
            applicationName_,
            serviceManifestName_,
            codePackageName_,
            instanceId_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRestartComplete(operation); },
            proxySPtr);
    }

private:
    void OnRestartComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.appMgmtClient_->EndRestartDeployedCodePackage(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    wstring serviceManifestName_;
    wstring codePackageName_;
    NamingUri applicationName_;
    wstring instanceId_;
};

// 710285e0-adcb-4163-a81f-449990645792
static const GUID CLSID_ComFabricClient_GetServiceHealthOperation =
    { 0x710285e0, 0xadcb, 0x4163, {0xa8, 0x1f, 0x44, 0x99, 0x90, 0x64, 0x57, 0x92} };
class ComFabricClient::GetServiceHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetServiceHealthOperation,
        CLSID_ComFabricClient_GetServiceHealthOperation,
        GetServiceHealthOperation,
        ComAsyncOperationContext)

public:
    GetServiceHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , serviceHealth_()
    {
    }

    virtual ~GetServiceHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricServiceHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricServiceHealthResult> resultCPtr =
                make_com<ComServiceHealthResult, IFabricServiceHealthResult>(thisOperation->serviceHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetServiceHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetServiceHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetServiceHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetServiceHealth(operation, /*out*/ serviceHealth_);
        this->TryComplete(operation->Parent, error);
    }

    ServiceHealthQueryDescription queryDescription_;
    ServiceHealth serviceHealth_;
};

// 4b0c9015-4322-44ac-8fe0-15631732ad13
static const GUID CLSID_ComFabricClient_GetPartitionHealthOperation =
    { 0x4b0c9015, 0x4322, 0x44ac, {0x8f, 0xe0, 0x15, 0x63, 0x17, 0x32, 0xad, 0x13} };
class ComFabricClient::GetPartitionHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetPartitionHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetPartitionHealthOperation,
        CLSID_ComFabricClient_GetPartitionHealthOperation,
        GetPartitionHealthOperation,
        ComAsyncOperationContext)

public:
    GetPartitionHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , partitionHealth_()
    {
    }

    virtual ~GetPartitionHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricPartitionHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetPartitionHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetPartitionHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricPartitionHealthResult> resultCPtr =
                make_com<ComPartitionHealthResult, IFabricPartitionHealthResult>(thisOperation->partitionHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetPartitionHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetPartitionHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetPartitionHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetPartitionHealth(operation, /*out*/ partitionHealth_);
        this->TryComplete(operation->Parent, error);
    }

    PartitionHealthQueryDescription queryDescription_;
    PartitionHealth partitionHealth_;
};

// 89d45b0d-c52f-4688-b2ef-4224185197e8
static const GUID CLSID_ComFabricClient_GetReplicaHealthOperation =
    { 0x89d45b0d, 0xc52f, 0x4688, {0xb2, 0xef, 0x42, 0x24, 0x18, 0x51, 0x97, 0xe8} };
class ComFabricClient::GetReplicaHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetReplicaHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetReplicaHealthOperation,
        CLSID_ComFabricClient_GetReplicaHealthOperation,
        GetReplicaHealthOperation,
        ComAsyncOperationContext)

public:
    GetReplicaHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , replicaHealth_()
    {
    }

    virtual ~GetReplicaHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricReplicaHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetReplicaHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetReplicaHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricReplicaHealthResult> resultCPtr =
                make_com<ComReplicaHealthResult, IFabricReplicaHealthResult>(thisOperation->replicaHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetReplicaHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetReplicaHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetReplicaHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetReplicaHealth(operation, /*out*/ replicaHealth_);
        this->TryComplete(operation->Parent, error);
    }

    ReplicaHealthQueryDescription queryDescription_;
    ReplicaHealth replicaHealth_;
};

// 6c3c7611-e3f7-4ef1-8302-aa0d8c010ba5
 static const GUID CLSID_ComFabricClient_GetDeployedApplicationHealthOperation =
    { 0x6c3c7611, 0xe3f7, 0x4ef1, {0x83, 0x02, 0xaa, 0x0d, 0x8c, 0x01, 0x0b, 0xa5} };
class ComFabricClient::GetDeployedApplicationHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedApplicationHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedApplicationHealthOperation,
        CLSID_ComFabricClient_GetDeployedApplicationHealthOperation,
        GetDeployedApplicationHealthOperation,
        ComAsyncOperationContext)

public:
    GetDeployedApplicationHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , deployedApplicationHealth_()
    {
    }

    virtual ~GetDeployedApplicationHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricDeployedApplicationHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedApplicationHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedApplicationHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricDeployedApplicationHealthResult> resultCPtr =
                make_com<ComDeployedApplicationHealthResult, IFabricDeployedApplicationHealthResult>(thisOperation->deployedApplicationHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetDeployedApplicationHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetDeployedApplicationHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetDeployedApplicationHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetDeployedApplicationHealth(operation, /*out*/ deployedApplicationHealth_);
        this->TryComplete(operation->Parent, error);
    }

    DeployedApplicationHealthQueryDescription queryDescription_;
    DeployedApplicationHealth deployedApplicationHealth_;
};

// 5211df33-df4c-4307-a74d-038dfd00dc34
static const GUID CLSID_ComFabricClient_GetDeployedServicePackageHealthOperation =
    { 0x5211df33, 0xdf4c, 0x4307, {0xa7, 0x4d, 0x03, 0x8d, 0xfd, 0x00, 0xdc, 0x34} };
class ComFabricClient::GetDeployedServicePackageHealthOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedServicePackageHealthOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedServicePackageHealthOperation,
        CLSID_ComFabricClient_GetDeployedServicePackageHealthOperation,
        GetDeployedServicePackageHealthOperation,
        ComAsyncOperationContext)

public:
    GetDeployedServicePackageHealthOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , deployedServicePackageHealth_()
    {
    }

    virtual ~GetDeployedServicePackageHealthOperation()
    {
    }

    HRESULT Initialize(
        FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION const * publicQueryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*publicQueryDescription);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricDeployedServicePackageHealthResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedServicePackageHealthOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedServicePackageHealthOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricDeployedServicePackageHealthResult> resultCPtr =
                make_com<ComDeployedServicePackageHealthResult, IFabricDeployedServicePackageHealthResult>(thisOperation->deployedServicePackageHealth_);

            *result = resultCPtr.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.healthClient_->BeginGetDeployedServicePackageHealth(
            queryDescription_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetDeployedServicePackageHealthComplete(operation); },
            proxySPtr);
    }

private:
    void OnGetDeployedServicePackageHealthComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.healthClient_->EndGetDeployedServicePackageHealth(operation, /*out*/ deployedServicePackageHealth_);
        this->TryComplete(operation->Parent, error);
    }

    DeployedServicePackageHealthQueryDescription queryDescription_;
    DeployedServicePackageHealth deployedServicePackageHealth_;
};

// {C3D4E311-B6C8-4F65-AB2C-D3466CEE29C9}
static const GUID CLSID_ComFabricClient_GetNodeListOperation =
    {0xc3d4e311,0xb6c8,0x4f65,{0xab,0x2c,0xd3,0x46,0x6c,0xee,0x29,0xc9}};
class ComFabricClient::GetNodeListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetNodeListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetNodeListOperation,
        CLSID_ComFabricClient_GetNodeListOperation,
        GetNodeListOperation,
        ComAsyncOperationContext)

public:
    GetNodeListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , nodeList_()
        , pagingStatus_()
        , excludeStoppedNodeInfo_(false)
    {
    }

    virtual ~GetNodeListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_NODE_QUERY_DESCRIPTION * queryDescription,
        __in BOOLEAN excludeStoppedNodeInfo,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = queryDescription_.FromPublicApi(*queryDescription);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(error));
        }

        excludeStoppedNodeInfo_ = excludeStoppedNodeInfo == 0 ? false : true;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetNodeListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetNodeListOperation> thisOperation(context, CLSID_ComFabricClient_GetNodeListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto nodeListResult = make_com<ComGetNodeListResult, IFabricGetNodeListResult>(
                move(thisOperation->nodeList_), move(thisOperation->pagingStatus_));
            *result = nodeListResult.DetachNoRelease();
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetNodeListResult2 **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetNodeListOperation> thisOperation(context, CLSID_ComFabricClient_GetNodeListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto nodeListResult = make_com<ComGetNodeListResult, IFabricGetNodeListResult2>(
                move(thisOperation->nodeList_), move(thisOperation->pagingStatus_));
            *result = nodeListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetNodeList(
            queryDescription_,
            excludeStoppedNodeInfo_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetNodeListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetNodeListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetNodeList(operation, nodeList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

private:
    NodeQueryDescription queryDescription_;
    vector<NodeQueryResult> nodeList_;
    PagingStatusUPtr pagingStatus_;
    bool excludeStoppedNodeInfo_;
};

// 72de3021-cf5c-43db-a704-874ba66352ac
static const GUID CLSID_ComFabricClient_GetApplicationTypeListOperation =
    {0x72de3021,0xcf5c,0x43db,{0xa7, 0x04, 0x87, 0x4b, 0xa6, 0x63, 0x52, 0xac}};
class ComFabricClient::GetApplicationTypeListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationTypeListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetApplicationTypeListOperation,
        CLSID_ComFabricClient_GetApplicationTypeListOperation,
        GetApplicationTypeListOperation,
        ComAsyncOperationContext)

public:
    GetApplicationTypeListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        applicationTypeNameFilter_(),
        applicationTypeList_()
    {
    }

    virtual ~GetApplicationTypeListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds,  callback);
        if (FAILED(hr)) { return hr; }

        // Get value of ApplicationTypeNameFilter
        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ApplicationTypeNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            applicationTypeNameFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetApplicationTypeListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationTypeListOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationTypeListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetApplicationTypeListResult> applicationTypeListResult =
                make_com<ComGetApplicationTypeListResult, IFabricGetApplicationTypeListResult>(move(thisOperation->applicationTypeList_));
            *result = applicationTypeListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetApplicationTypeList(
            applicationTypeNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetApplicationTypeListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetApplicationTypeListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetApplicationTypeList(operation, applicationTypeList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring applicationTypeNameFilter_;
    vector<ApplicationTypeQueryResult> applicationTypeList_;
};

// {2C0B33A7-A117-4706-86A3-D5E4082451B7}
static const GUID CLSID_ComFabricClient_TransportBehaviorListOperation =
{ 0x2c0b33a7, 0xa117, 0x4706, { 0x86, 0xa3, 0xd5, 0xe4, 0x8, 0x24, 0x51, 0xb7 } };
class ComFabricClient::TransportBehaviorListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(TransportBehaviorListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    TransportBehaviorListOperation,
    CLSID_ComFabricClient_TransportBehaviorListOperation,
    TransportBehaviorListOperation,
    ComAsyncOperationContext)

public:
    TransportBehaviorListOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , nodeName_()
        , transportBehaviorList_()
    {
    }

    virtual ~TransportBehaviorListOperation() = default;

    HRESULT Initialize(
        LPCWSTR nodeName,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(nodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricStringListResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<TransportBehaviorListOperation> thisOperation(context, CLSID_ComFabricClient_TransportBehaviorListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            ComStringCollectionResult::ReturnStringCollectionResult(move(thisOperation->transportBehaviorList_), result);
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.clusterMgmtClient_->BeginGetTransportBehaviorList(
            nodeName_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnTransportBehaviortOperationComplete(operation); },
            proxySPtr);
    }

private:
    void OnTransportBehaviortOperationComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.clusterMgmtClient_->EndGetTransportBehaviorList(operation, transportBehaviorList_);
        this->TryComplete(operation->Parent, error);
    }

    wstring nodeName_;
    vector<wstring> transportBehaviorList_;
};


// ea789c9d-aeed-4638-80d5-0d60112426a1
static const GUID CLSID_ComFabricClient_GetServiceTypeListOperation =
    {0xea789c9d,0xaeed,0x4638,{0x80, 0xd5, 0x0d, 0x60, 0x11, 0x24, 0x26, 0xa1}};
class ComFabricClient::GetServiceTypeListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceTypeListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetServiceTypeListOperation,
        CLSID_ComFabricClient_GetServiceTypeListOperation,
        GetServiceTypeListOperation,
        ComAsyncOperationContext)

public:
    GetServiceTypeListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        serviceTypeNameFilter_(),
        serviceTypeList_()
    {
    }

    virtual ~GetServiceTypeListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_TYPE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ApplicationTypeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            applicationTypeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ApplicationTypeVersion,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            applicationTypeVersion_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ServiceTypeNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceTypeNameFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetServiceTypeListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceTypeListOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceTypeListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetServiceTypeListResult> serviceTypeListResult =
                make_com<ComGetServiceTypeListResult, IFabricGetServiceTypeListResult>(move(thisOperation->serviceTypeList_));
            *result = serviceTypeListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetServiceTypeList(
            applicationTypeName_,
            applicationTypeVersion_,
            serviceTypeNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetServiceTypeListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetServiceTypeListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetServiceTypeList(operation, serviceTypeList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring applicationTypeName_;
    wstring applicationTypeVersion_;
    wstring serviceTypeNameFilter_;
    vector<ServiceTypeQueryResult> serviceTypeList_;
};

// c8e904a9-157d-4bda-8e2c-4a4532845bff
static const GUID CLSID_ComFabricClient_GetServiceGroupMemberTypeListOperation =
{ 0xc8e904a9, 0x157d, 0x4bda, { 0x8e, 0x2c, 0x4a, 0x45, 0x32, 0x84, 0x5b, 0xff } };
class ComFabricClient::GetServiceGroupMemberTypeListOperation :
public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceGroupMemberTypeListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetServiceGroupMemberTypeListOperation,
    CLSID_ComFabricClient_GetServiceGroupMemberTypeListOperation,
    GetServiceGroupMemberTypeListOperation,
    ComAsyncOperationContext)

public:
    GetServiceGroupMemberTypeListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        serviceGroupTypeNameFilter_(),
        serviceGroupMemberTypeList_()
    {
    }

    virtual ~GetServiceGroupMemberTypeListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ApplicationTypeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            applicationTypeName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ApplicationTypeVersion,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            applicationTypeVersion_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ServiceGroupTypeNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceGroupTypeNameFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetServiceGroupMemberTypeListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceGroupMemberTypeListOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceGroupMemberTypeListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetServiceGroupMemberTypeListResult> serviceGroupMemberTypeListResult =
                make_com<ComGetServiceGroupMemberTypeListResult, IFabricGetServiceGroupMemberTypeListResult>(move(thisOperation->serviceGroupMemberTypeList_));
            *result = serviceGroupMemberTypeListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.queryClient_->BeginGetServiceGroupMemberTypeList(
            applicationTypeName_,
            applicationTypeVersion_,
            serviceGroupTypeNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetServiceGroupMemberTypeListComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetServiceGroupMemberTypeListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetServiceGroupMemberTypeList(operation, serviceGroupMemberTypeList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring applicationTypeName_;
    wstring applicationTypeVersion_;
    wstring serviceGroupTypeNameFilter_;
    vector<ServiceGroupMemberTypeQueryResult> serviceGroupMemberTypeList_;
};

// d7617a8a-b9e9-4cd2-81a4-1662349d34cb
static const GUID CLSID_ComFabricClient_GetApplicationListOperation =
    {0xd7617a8a,0xb9e9,0x4cd2,{0x81, 0xa4, 0x16, 0x62, 0x34, 0x9d, 0x34, 0xcb}};
class ComFabricClient::GetApplicationListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetApplicationListOperation,
        CLSID_ComFabricClient_GetApplicationListOperation,
        GetApplicationListOperation,
        ComAsyncOperationContext)

public:
    GetApplicationListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , applicationQueryDescription_()
        , applicationList_()
        , pagingStatus_()
    {
    }

    virtual ~GetApplicationListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_APPLICATION_QUERY_DESCRIPTION * queryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = applicationQueryDescription_.FromPublicApi(*queryDescription);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.queryClient_->BeginGetApplicationList(
            applicationQueryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetApplicationListComplete(operation);
            },
            proxySPtr);
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetApplicationListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationListOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto applicationListResult = make_com<ComGetApplicationListResult, IFabricGetApplicationListResult>(
                move(thisOperation->applicationList_), move(thisOperation->pagingStatus_));
            *result = applicationListResult.DetachNoRelease();
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetApplicationListResult2 **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationListOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto applicationListResult = make_com<ComGetApplicationListResult, IFabricGetApplicationListResult2>(
                move(thisOperation->applicationList_), move(thisOperation->pagingStatus_));
            *result = applicationListResult.DetachNoRelease();
        }

        return hr;
    }

private:
    void OnGetApplicationListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetApplicationList(operation, applicationList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

private:
    ApplicationQueryDescription applicationQueryDescription_;
    vector<ApplicationQueryResult> applicationList_;
    PagingStatusUPtr pagingStatus_;
};

// a57ba302-9811-4eca-aff6-ba9697ae29b7
static const GUID CLSID_ComFabricClient_GetComposeDeploymentStatusListOperation =
    {0xa57ba302,0x9811,0x4eca,{0xaf, 0xf6, 0xba, 0x96, 0x97, 0xae, 0x29, 0xb7}};
class ComFabricClient::GetComposeDeploymentStatusListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetComposeDeploymentStatusListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetComposeDeploymentStatusListOperation,
        CLSID_ComFabricClient_GetComposeDeploymentStatusListOperation,
        GetComposeDeploymentStatusListOperation,
        ComAsyncOperationContext)

public:
    GetComposeDeploymentStatusListOperation(ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , dockerComposeDeploymentStatusQueryDescription_()
        , dockerComposeDeploymentStatusList_()
        , pagingStatus_()
    {
    }

    virtual ~GetComposeDeploymentStatusListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION * queryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = dockerComposeDeploymentStatusQueryDescription_.FromPublicApi(*queryDescription);
        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetComposeDeploymentStatusListResult ** result)
    {
        if (context == nullptr || result == nullptr) { return E_POINTER; }

        ComPointer<GetComposeDeploymentStatusListOperation> thisOperation(context, CLSID_ComFabricClient_GetComposeDeploymentStatusListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto listResult = make_com<ComGetComposeDeploymentStatusListResult, IFabricGetComposeDeploymentStatusListResult>(
                move(thisOperation->dockerComposeDeploymentStatusList_),
                move(thisOperation->pagingStatus_));
            *result = listResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.composeMgmtClient_->BeginGetComposeDeploymentStatusList(
            dockerComposeDeploymentStatusQueryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetComposeDeploymentStatusListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetComposeDeploymentStatusListComplete(__in AsyncOperationSPtr const & operation)
    {
        ErrorCode error = Owner.composeMgmtClient_->EndGetComposeDeploymentStatusList(operation, dockerComposeDeploymentStatusList_, pagingStatus_);
        this->TryComplete(operation->Parent, error);
    }

    ComposeDeploymentStatusQueryDescription dockerComposeDeploymentStatusQueryDescription_;
    vector<ComposeDeploymentStatusQueryResult> dockerComposeDeploymentStatusList_;
    PagingStatusUPtr pagingStatus_;
};


// 10c1cfe9-3411-49aa-b4c3-a9885d580d85
static const GUID CLSID_ComFabricClient_GetServiceListOperation =
    {0x10c1cfe9,0x3411,0x49aa,{0xb4, 0xc3, 0xa9, 0x88, 0x5d, 0x58, 0x0d, 0x85}};
class ComFabricClient::GetServiceListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetServiceListOperation,
        CLSID_ComFabricClient_GetServiceListOperation,
        GetServiceListOperation,
        ComAsyncOperationContext)

public:
    GetServiceListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , serviceQueryDescription_()
        , serviceList_()
        , pagingStatus_()
    {
    }

    virtual ~GetServiceListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = serviceQueryDescription_.FromPublicApi(*queryDescription);

        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetServiceListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceListOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto serviceListResult = make_com<ComGetServiceListResult, IFabricGetServiceListResult>(
                move(thisOperation->serviceList_), move(thisOperation->pagingStatus_));
            *result = serviceListResult.DetachNoRelease();
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetServiceListResult2 **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceListOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto serviceListResult = make_com<ComGetServiceListResult, IFabricGetServiceListResult2>(
                move(thisOperation->serviceList_), move(thisOperation->pagingStatus_));
            *result = serviceListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.queryClient_->BeginGetServiceList(
            serviceQueryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetServiceListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetServiceListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetServiceList(operation, serviceList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

private:
    ServiceQueryDescription serviceQueryDescription_;
    vector<ServiceQueryResult> serviceList_;
    PagingStatusUPtr pagingStatus_;
};

// EE8719A2-21B2-41D4-86CB-68C0CDA055A9
static const GUID CLSID_ComFabricClient_GetServiceGroupMemberListOperation =
{ 0xee8719a2, 0x21b2, 0x41d4, { 0x86, 0xcb, 0x68, 0xc0, 0xcd, 0xa0, 0x55, 0xa9 } };
class ComFabricClient::GetServiceGroupMemberListOperation :
public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceGroupMemberListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetServiceGroupMemberListOperation,
    CLSID_ComFabricClient_GetServiceGroupMemberListOperation,
    GetServiceGroupMemberListOperation,
    ComAsyncOperationContext)

public:
    GetServiceGroupMemberListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        applicationName_(NamingUri::RootNamingUri),
        serviceNameFilter_(NamingUri::RootNamingUri),
        serviceGroupMemberList_()
    {
    }

    virtual ~GetServiceGroupMemberListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_GROUP_MEMBER_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        if (queryDescription->ApplicationName != NULL)
        {
            hr = NamingUri::TryParse(queryDescription->ApplicationName, Constants::FabricClientTrace, applicationName_);
            if (FAILED(hr)) { return hr; }
        }

        if (queryDescription->ServiceNameFilter != NULL)
        {
            hr = NamingUri::TryParse(queryDescription->ServiceNameFilter, Constants::FabricClientTrace, serviceNameFilter_);
            if (FAILED(hr)) { return hr; }
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetServiceGroupMemberListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceGroupMemberListOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceGroupMemberListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetServiceGroupMemberListResult> serviceGroupMemberListResult =
                make_com<ComGetServiceGroupMemberListResult, IFabricGetServiceGroupMemberListResult>(move(thisOperation->serviceGroupMemberList_));
            *result = serviceGroupMemberListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.queryClient_->BeginGetServiceGroupMemberList(
            applicationName_,
            serviceNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetServiceGroupMemberListComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetServiceGroupMemberListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetServiceGroupMemberList(operation, serviceGroupMemberList_);
        TryComplete(operation->Parent, error);
    }

private:
    NamingUri applicationName_;
    NamingUri serviceNameFilter_;
    vector<ServiceGroupMemberQueryResult> serviceGroupMemberList_;
};

// 1f5fdce9-6d9c-4c04-b88d-ec27dd26958d
static const GUID CLSID_ComFabricClient_GetPartitionListOperation =
    {0x1f5fdce9,0x6d9c,0x4c04,{0xb8, 0x8d, 0xec, 0x27, 0xdd, 0x26, 0x95, 0x8d}};
class ComFabricClient::GetPartitionListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetPartitionListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetPartitionListOperation,
        CLSID_ComFabricClient_GetPartitionListOperation,
        GetPartitionListOperation,
        ComAsyncOperationContext)

public:
    GetPartitionListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , serviceName_()
        , partitionIdFilter_()
        , continuationToken_()
        , partitionList_()
        , pagingStatus_()
    {
    }

    virtual ~GetPartitionListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_PARTITION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        partitionIdFilter_ = Guid(queryDescription->PartitionIdFilter);

        //if the ServiceName is provided, it should be a valid Uri
        if (queryDescription->ServiceName != NULL)
        {
            hr = NamingUri::TryParse(queryDescription->ServiceName, Constants::FabricClientTrace, serviceName_);
            if (FAILED(hr)) { return hr; }
        }
        //if the ServiceName is not provided, the partitionId must not be empty
        else if (partitionIdFilter_ == partitionIdFilter_.Empty())
        {
            return E_INVALIDARG;
        }

        if (queryDescription->Reserved != NULL)
        {
            auto ex1 = reinterpret_cast<FABRIC_SERVICE_PARTITION_QUERY_DESCRIPTION_EX1*>(queryDescription->Reserved);
            hr = StringUtility::LpcwstrToWstring(
                ex1->ContinuationToken,
                true /* acceptNull */,
                ParameterValidator::MinStringSize,
                ParameterValidator::MaxStringSize,
                continuationToken_);
            if (FAILED(hr)) { return hr; }
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetPartitionListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetPartitionListOperation> thisOperation(context, CLSID_ComFabricClient_GetPartitionListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto partitionListResult = make_com<ComGetPartitionListResult, IFabricGetPartitionListResult>(
                move(thisOperation->partitionList_), move(thisOperation->pagingStatus_));
            *result = partitionListResult.DetachNoRelease();
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetPartitionListResult2 **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetPartitionListOperation> thisOperation(context, CLSID_ComFabricClient_GetPartitionListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto partitionListResult = make_com<ComGetPartitionListResult, IFabricGetPartitionListResult2>(
                move(thisOperation->partitionList_), move(thisOperation->pagingStatus_));
            *result = partitionListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetPartitionList(
            serviceName_,
            partitionIdFilter_,
            continuationToken_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetPartitionListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetPartitionListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetPartitionList(operation, partitionList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

private:
    NamingUri serviceName_;
    Guid partitionIdFilter_;
    wstring continuationToken_;
    vector<ServicePartitionQueryResult> partitionList_;
    PagingStatusUPtr pagingStatus_;
};

// 60fc4cd6-fc0f-44b0-a7a3-f2f4b1686c32
static const GUID CLSID_ComFabricClient_GetReplicaListOperation =
    {0x60fc4cd6,0xfc0f,0x44b0,{0xa7, 0xa3, 0xf2, 0xf4, 0xb1, 0x68, 0x6c, 0x32}};
class ComFabricClient::GetReplicaListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetReplicaListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetReplicaListOperation,
        CLSID_ComFabricClient_GetReplicaListOperation,
        GetReplicaListOperation,
        ComAsyncOperationContext)

public:
    GetReplicaListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , partitionId_()
        , replicaIdOrInstanceIdFilter_(0)
        , replicaStatusFilter_(FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT)
        , continuationToken_()
        , replicaList_()
        , pagingStatus_()

    {
    }

    virtual ~GetReplicaListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        partitionId_ = Guid(queryDescription->PartitionId);
        replicaIdOrInstanceIdFilter_ = queryDescription->ReplicaIdOrInstanceIdFilter;

        if (queryDescription->Reserved != NULL)
        {
            auto ex1 = reinterpret_cast<FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION_EX1*>(queryDescription->Reserved);
            replicaStatusFilter_ = ex1->ReplicaStatusFilter;

            if (ex1->Reserved != NULL)
            {
                auto ex2 = reinterpret_cast<FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION_EX2*>(ex1->Reserved);
                hr = StringUtility::LpcwstrToWstring(
                    ex2->ContinuationToken,
                    true /* acceptNull */,
                    ParameterValidator::MinStringSize,
                    ParameterValidator::MaxStringSize,
                    continuationToken_);
                if (FAILED(hr)) { return hr; }
            }
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetReplicaListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetReplicaListOperation> thisOperation(context, CLSID_ComFabricClient_GetReplicaListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto replicaListResult = make_com<ComGetReplicaListResult, IFabricGetReplicaListResult>(
                move(thisOperation->replicaList_), move(thisOperation->pagingStatus_));
            *result = replicaListResult.DetachNoRelease();
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetReplicaListResult2 **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetReplicaListOperation> thisOperation(context, CLSID_ComFabricClient_GetReplicaListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            auto replicaListResult = make_com<ComGetReplicaListResult, IFabricGetReplicaListResult2>(
                move(thisOperation->replicaList_), move(thisOperation->pagingStatus_));
            *result = replicaListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetReplicaList(
            partitionId_,
            replicaIdOrInstanceIdFilter_,
            replicaStatusFilter_,
            continuationToken_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetReplicaListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetReplicaListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetReplicaList(operation, replicaList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

private:
    Guid partitionId_;
    int64 replicaIdOrInstanceIdFilter_;
    DWORD replicaStatusFilter_;
    wstring continuationToken_;
    vector<ServiceReplicaQueryResult> replicaList_;
    PagingStatusUPtr pagingStatus_;
};

// f9c764c2-6ca7-4f75-8dd6-fb6679b1c08a
static const GUID CLSID_ComFabricClient_GetDeployedApplicationListOperation =
    {0xf9c764c2,0x6ca7,0x4f75,{0x8d, 0xd6, 0xfb, 0x66, 0x79, 0xb1, 0xc0, 0x8a}};
class ComFabricClient::GetDeployedApplicationListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedApplicationListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedApplicationListOperation,
        CLSID_ComFabricClient_GetDeployedApplicationListOperation,
        GetDeployedApplicationListOperation,
        ComAsyncOperationContext)

public:
    GetDeployedApplicationListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        nodeName_(),
        applicationNameFilter_(NamingUri::RootNamingUri),
        deployedApplicationList_()
    {
    }

    virtual ~GetDeployedApplicationListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_APPLICATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->NodeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            nodeName_);

        if (FAILED(hr)) { return hr; }

        if (queryDescription->ApplicationNameFilter != NULL)
        {
            hr = NamingUri::TryParse(
                    queryDescription->ApplicationNameFilter,
                    Constants::FabricClientTrace,
                    applicationNameFilter_);

            if (FAILED(hr)) { return hr; }
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedApplicationListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedApplicationListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedApplicationListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedApplicationListResult> deployedApplicationListResult =
                make_com<ComGetDeployedApplicationListResult, IFabricGetDeployedApplicationListResult>(move(thisOperation->deployedApplicationList_));
            *result = deployedApplicationListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetDeployedApplicationList(
            nodeName_,
            applicationNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetDeployedApplicationListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetDeployedApplicationListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetDeployedApplicationList(operation, deployedApplicationList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring nodeName_;
    NamingUri applicationNameFilter_;
    vector<DeployedApplicationQueryResult> deployedApplicationList_;
};

// b66702a0-b019-4a80-b47a-6c5203f38bf8
static const GUID CLSID_ComFabricClient_GetDeployedServiceManifestListOperation =
    {0xb66702a0,0xb019,0x4a80,{0xb4, 0x7a, 0x6c, 0x52, 0x03, 0xf3, 0x8b, 0xf8}};
class ComFabricClient::GetDeployedServiceManifestListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedServiceManifestListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedServiceManifestListOperation,
        CLSID_ComFabricClient_GetDeployedServiceManifestListOperation,
        GetDeployedServiceManifestListOperation,
        ComAsyncOperationContext)

public:
    GetDeployedServiceManifestListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        nodeName_(),
        applicationName_(),
        serviceManifestNameFilter_(),
        deployedServiceManifestList_()
    {
    }

    virtual ~GetDeployedServiceManifestListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->NodeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            nodeName_);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(queryDescription->ApplicationName, Constants::FabricClientTrace, applicationName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ServiceManifestNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceManifestNameFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedServicePackageListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedServiceManifestListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedServiceManifestListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedServicePackageListResult> deployedServiceManifestListResult =
                make_com<ComGetDeployedServiceManifestListResult, IFabricGetDeployedServicePackageListResult>(move(thisOperation->deployedServiceManifestList_));
            *result = deployedServiceManifestListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetDeployedServicePackageList(
            nodeName_,
            applicationName_,
            serviceManifestNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetDeployedServiceManifestListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetDeployedServiceManifestListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetDeployedServicePackageList(operation, deployedServiceManifestList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring nodeName_;
    NamingUri applicationName_;
    wstring serviceManifestNameFilter_;
    vector<DeployedServiceManifestQueryResult> deployedServiceManifestList_;
};

// 4d9c3005-4485-48bb-a98a-cdc7916e7c38
static const GUID CLSID_ComFabricClient_GetDeployedServiceTypeListOperation =
    {0x4d9c3005,0x4485,0x48bb,{0xa9, 0x8a, 0xcd, 0xc7, 0x91, 0x6e, 0x7c, 0x38}};
class ComFabricClient::GetDeployedServiceTypeListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedServiceTypeListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedServiceTypeListOperation,
        CLSID_ComFabricClient_GetDeployedServiceTypeListOperation,
        GetDeployedServiceTypeListOperation,
        ComAsyncOperationContext)

public:
    GetDeployedServiceTypeListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        nodeName_(),
        applicationName_(NamingUri::RootNamingUri),
        serviceManifestNameFilter_(),
        serviceTypeNameFilter_(),
        deployedServiceTypeList_()
    {
    }

    virtual ~GetDeployedServiceTypeListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->NodeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            nodeName_);
        if (FAILED(hr)) { return hr; }

        if (queryDescription->ApplicationName != NULL)
        {
            hr = NamingUri::TryParse(queryDescription->ApplicationName, Constants::FabricClientTrace, applicationName_);
            if (FAILED(hr)) { return hr; }
        }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ServiceManifestNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceManifestNameFilter_);
        if (FAILED(hr)) { return hr; }

         hr = StringUtility::LpcwstrToWstring(
            queryDescription->ServiceTypeNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceTypeNameFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedServiceTypeListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedServiceTypeListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedServiceTypeListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedServiceTypeListResult> deployedServiceTypeListResult =
                make_com<ComGetDeployedServiceTypeListResult, IFabricGetDeployedServiceTypeListResult>(move(thisOperation->deployedServiceTypeList_));
            *result = deployedServiceTypeListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetDeployedServiceTypeList(
            nodeName_,
            applicationName_,
            serviceManifestNameFilter_,
            serviceTypeNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetDeployedServiceTypeListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetDeployedServiceTypeListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetDeployedServiceTypeList(operation, deployedServiceTypeList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring nodeName_;
    NamingUri applicationName_;
    wstring serviceManifestNameFilter_;
    wstring serviceTypeNameFilter_;
    vector<DeployedServiceTypeQueryResult> deployedServiceTypeList_;
};


// 7c6cde61-7160-47de-8755-114e7846a968
static const GUID CLSID_ComFabricClient_GetDeployedCodePackageListOperation =
    {0x7c6cde61,0x7160,0x47de,{0x87, 0x55, 0x11, 0x4e, 0x78, 0x46, 0xa9, 0x68}};
class ComFabricClient::GetDeployedCodePackageListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedCodePackageListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedCodePackageListOperation,
        CLSID_ComFabricClient_GetDeployedCodePackageListOperation,
        GetDeployedCodePackageListOperation,
        ComAsyncOperationContext)

public:
    GetDeployedCodePackageListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        nodeName_(),
        applicationName_(),
        serviceManifestNameFilter_(),
        codePackageNameFilter_(),
        deployedCodePackageList_()
    {
    }

    virtual ~GetDeployedCodePackageListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->NodeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            nodeName_);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(queryDescription->ApplicationName, Constants::FabricClientTrace, applicationName_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ServiceManifestNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceManifestNameFilter_);
        if (FAILED(hr)) { return hr; }

         hr = StringUtility::LpcwstrToWstring(
            queryDescription->CodePackageNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            codePackageNameFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;

    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedCodePackageListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedCodePackageListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedCodePackageListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedCodePackageListResult> deployedCodePackageListResult =
                make_com<ComGetDeployedCodePackageListResult, IFabricGetDeployedCodePackageListResult>(move(thisOperation->deployedCodePackageList_));
            *result = deployedCodePackageListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetDeployedCodePackageList(
            nodeName_,
            applicationName_,
            serviceManifestNameFilter_,
            codePackageNameFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetDeployedCodePackageListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetDeployedCodePackageListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetDeployedCodePackageList(operation, deployedCodePackageList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring nodeName_;
    NamingUri applicationName_;
    wstring serviceManifestNameFilter_;
    wstring codePackageNameFilter_;
    vector<DeployedCodePackageQueryResult> deployedCodePackageList_;
};

// 2aece6b2-45b4-48f1-8110-d01c72c5966b
static const GUID CLSID_ComFabricClient_GetDeployedReplicaListOperation =
    {0x2aece6b2,0x45b4,0x48f1,{0x81, 0x10, 0xd0, 0x1c, 0x72, 0xc5, 0x96, 0x6b}};
class ComFabricClient::GetDeployedReplicaListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedReplicaListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedReplicaListOperation,
        CLSID_ComFabricClient_GetDeployedReplicaListOperation,
        GetDeployedReplicaListOperation,
        ComAsyncOperationContext)

public:
    GetDeployedReplicaListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        nodeName_(),
        applicationName_(NamingUri::RootNamingUri),
        serviceManifestName_(),
        partitionIdFilter_(),
        deployedReplicaList_()
    {
    }

    virtual ~GetDeployedReplicaListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->NodeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            nodeName_);
        if (FAILED(hr)) { return hr; }

        if (queryDescription->ApplicationName != NULL)
        {
            hr = NamingUri::TryParse(queryDescription->ApplicationName, Constants::FabricClientTrace, applicationName_);
            if (FAILED(hr)) { return hr; }
        }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ServiceManifestNameFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            serviceManifestName_);
        if (FAILED(hr)) { return hr; }

        partitionIdFilter_ = Guid(queryDescription->PartitionIdFilter);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedReplicaListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedReplicaListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedReplicaListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedReplicaListResult> deployedReplicaListResult =
                make_com<ComGetDeployedReplicaListResult, IFabricGetDeployedReplicaListResult>(move(thisOperation->deployedReplicaList_));
            *result = deployedReplicaListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetDeployedReplicaList(
            nodeName_,
            applicationName_,
            serviceManifestName_,
            partitionIdFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetDeployedReplicaListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetDeployedReplicaListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetDeployedReplicaList(operation, deployedReplicaList_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring nodeName_;
    NamingUri applicationName_;
    wstring serviceManifestName_;
    Guid partitionIdFilter_;
    vector<DeployedServiceReplicaQueryResult> deployedReplicaList_;
};

// 4623AC11-2D90-4934-B541-E34D85815C4F
static const GUID CLSID_ComFabricClient_GetDeployedServiceReplicaDetailOperation =
    {0x4623ac11,0x2d90,0x4934,{0xb5, 0x41, 0xe3, 0x4d, 0x85, 0x81, 0x5c, 0x4f}};
class ComFabricClient::GetDeployedServiceReplicaDetailOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedServiceReplicaDetailOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedServiceReplicaDetailOperation,
        CLSID_ComFabricClient_GetDeployedServiceReplicaDetailOperation,
        GetDeployedServiceReplicaDetailOperation,
        ComAsyncOperationContext)

public:
    GetDeployedServiceReplicaDetailOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , partitionId_()
        , replicaId_(0)
        , nodeName_()
    {
    }

    virtual ~GetDeployedServiceReplicaDetailOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->NodeName,
            false /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            nodeName_);

        if (FAILED(hr)) { return hr; }

        replicaId_ = queryDescription->ReplicaId;
        partitionId_ = Guid(queryDescription->PartitionId);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedServiceReplicaDetailResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedServiceReplicaDetailOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedServiceReplicaDetailOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedServiceReplicaDetailResult> replicatorStatusResult =
                make_com<ComGetDeployedServiceReplicaDetailResult, IFabricGetDeployedServiceReplicaDetailResult>(move(thisOperation->result_));
            *result = replicatorStatusResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetDeployedServiceReplicaDetail(
            nodeName_,
            partitionId_,
            replicaId_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetDeployedServiceReplicaDetailComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetDeployedServiceReplicaDetailComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetDeployedServiceReplicaDetail(operation, result_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring nodeName_;
    Guid partitionId_;
    FABRIC_REPLICA_ID replicaId_;
    DeployedServiceReplicaDetailQueryResult result_;
};


// {C5D4E311-B1C2-4FEF-CA2b-D3331CFE19F2}
static const GUID CLSID_ComFabricClient_GetClusterLoadInformationOperation =
    {0xc5d4e311,0xb1c2,0x4fef,{0xca,0x2b,0xd3,0x33,0x1c,0xfe,0x19,0xf2}};
class ComFabricClient::GetClusterLoadInformationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetClusterLoadInformationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetClusterLoadInformationOperation,
        CLSID_ComFabricClient_GetClusterLoadInformationOperation,
        GetClusterLoadInformationOperation,
        ComAsyncOperationContext)

public:
    GetClusterLoadInformationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        clusterLoadInformation_()
    {
    }

    virtual ~GetClusterLoadInformationOperation() {};

    HRESULT Initialize(
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetClusterLoadInformationResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetClusterLoadInformationOperation> thisOperation(context, CLSID_ComFabricClient_GetClusterLoadInformationOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetClusterLoadInformationResult> dataListResult =
                make_com<ComGetClusterLoadInformationResult, IFabricGetClusterLoadInformationResult>(move(thisOperation->clusterLoadInformation_));
            *result = dataListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetClusterLoadInformation(
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetClusterResourceManagerDataComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetClusterResourceManagerDataComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetClusterLoadInformation(operation, clusterLoadInformation_);
        TryComplete(operation->Parent, error);
    }

private:
    ClusterLoadInformationQueryResult clusterLoadInformation_;
};

// {7aa617ae-0070-4d69-83aa-2b5522371bce}
static const GUID CLSID_ComFabricClient_GetPartitionLoadInformationOperation =
    {0x7aa617ae,0x0070,0x4d69,{0x83, 0xaa, 0x2b, 0x55, 0x22, 0x37, 0x1b, 0xce}};
class ComFabricClient::GetPartitionLoadInformationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetPartitionLoadInformationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetPartitionLoadInformationOperation,
        CLSID_ComFabricClient_GetPartitionLoadInformationOperation,
        GetPartitionLoadInformationOperation,
        ComAsyncOperationContext)

public:
    GetPartitionLoadInformationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        partitionId_(),
        partitionLoadInformation_()
    {
    }

    virtual ~GetPartitionLoadInformationOperation() {};

    HRESULT Initialize(
        __in const FABRIC_PARTITION_LOAD_INFORMATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        partitionId_ = Guid(queryDescription->PartitionId);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetPartitionLoadInformationResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetPartitionLoadInformationOperation> thisOperation(context, CLSID_ComFabricClient_GetPartitionLoadInformationOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetPartitionLoadInformationResult> partitionInformationResult =
                make_com<ComGetPartitionLoadInformationResult, IFabricGetPartitionLoadInformationResult>(move(thisOperation->partitionLoadInformation_));
            *result = partitionInformationResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetPartitionLoadInformation(
            partitionId_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetPartitionLoadInformationComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetPartitionLoadInformationComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetPartitionLoadInformation(operation, partitionLoadInformation_);
        TryComplete(operation->Parent, error);
    }

private:
    Guid partitionId_;
    PartitionLoadInformationQueryResult partitionLoadInformation_;
};

// {E22BCE26-5020-4963-8BE7-F01197BDEF56}
static const GUID CLSID_ComFabricClient_GetFabricCodeVersionListOperation =
{ 0xe22bce26, 0x5020, 0x4963, { 0x8b, 0xe7, 0xf0, 0x11, 0x97, 0xbd, 0xef, 0x56 } };
class ComFabricClient::GetFabricCodeVersionListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetFabricCodeVersionListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetFabricCodeVersionListOperation,
        CLSID_ComFabricClient_GetFabricCodeVersionListOperation,
        GetFabricCodeVersionListOperation,
        ComAsyncOperationContext)

public:
    GetFabricCodeVersionListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , codeVersionFilter_()
    {
    }

    virtual ~GetFabricCodeVersionListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_PROVISIONED_CODE_VERSION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->CodeVersionFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            codeVersionFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetProvisionedCodeVersionListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetFabricCodeVersionListOperation> thisOperation(context, CLSID_ComFabricClient_GetFabricCodeVersionListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetProvisionedCodeVersionListResult> clusterCodeVersionListResult =
                make_com<ComGetProvisionedFabricCodeVersionListResult, IFabricGetProvisionedCodeVersionListResult>(move(thisOperation->result_));
            *result = clusterCodeVersionListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetProvisionedFabricCodeVersionList(
            codeVersionFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetFabricCodeVersionListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetFabricCodeVersionListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetProvisionedFabricCodeVersionList(operation, result_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring codeVersionFilter_;
    vector<ProvisionedFabricCodeVersionQueryResultItem> result_;
};

// {2450DF8E-5FC5-45FD-AB78-BC3F9031FB8F}
static const GUID CLSID_ComFabricClient_GetFabricConfigVersionListOperation =
{ 0x2450df8e, 0x5fc5, 0x45fd, { 0xab, 0x78, 0xbc, 0x3f, 0x90, 0x31, 0xfb, 0x8f } };
class ComFabricClient::GetFabricConfigVersionListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetFabricConfigVersionListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetFabricConfigVersionListOperation,
        CLSID_ComFabricClient_GetFabricConfigVersionListOperation,
        GetFabricConfigVersionListOperation,
        ComAsyncOperationContext)

public:
    GetFabricConfigVersionListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , configVersionFilter_()
    {
    }

    virtual ~GetFabricConfigVersionListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ConfigVersionFilter,
            true /* acceptNull */,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            configVersionFilter_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetProvisionedConfigVersionListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetFabricConfigVersionListOperation> thisOperation(context, CLSID_ComFabricClient_GetFabricConfigVersionListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetProvisionedConfigVersionListResult> clusterConfigVersionListResult =
                make_com<ComGetProvisionedFabricConfigVersionListResult, IFabricGetProvisionedConfigVersionListResult>(move(thisOperation->result_));
            *result = clusterConfigVersionListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetProvisionedFabricConfigVersionList(
            configVersionFilter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetFabricConfigVersionListComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetFabricConfigVersionListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetProvisionedFabricConfigVersionList(operation, result_);
        TryComplete(operation->Parent, error);
    }

private:
    wstring configVersionFilter_;
    vector<ProvisionedFabricConfigVersionQueryResultItem> result_;
};

// {C5D4E311-B1C2-4FEF-CA2b-D3331CFE19F3}
static const GUID CLSID_ComFabricClient_GetNodeLoadInformationOperation =
    {0xc5d4e311,0xb1c2,0x4fef,{0xca,0x2b,0xd3,0x33,0x1c,0xfe,0x19,0xf3}};
class ComFabricClient::GetNodeLoadInformationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetNodeLoadInformationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetNodeLoadInformationOperation,
        CLSID_ComFabricClient_GetNodeLoadInformationOperation,
        GetNodeLoadInformationOperation,
        ComAsyncOperationContext)

public:
    GetNodeLoadInformationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        NodeLoadInformation_()
    {
    }

    virtual ~GetNodeLoadInformationOperation() {};

    HRESULT Initialize(
        __in const FABRIC_NODE_LOAD_INFORMATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->NodeName,
            false, // dont accept null
            nodeName_);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetNodeLoadInformationResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetNodeLoadInformationOperation> thisOperation(context, CLSID_ComFabricClient_GetNodeLoadInformationOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetNodeLoadInformationResult> dataListResult =
                make_com<ComGetNodeLoadInformationResult, IFabricGetNodeLoadInformationResult>(move(thisOperation->NodeLoadInformation_));
            *result = dataListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetNodeLoadInformation(
            nodeName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetClusterResourceManagerDataComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetClusterResourceManagerDataComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetNodeLoadInformation(operation, NodeLoadInformation_);
        TryComplete(operation->Parent, error);
    }

private:

    std::wstring nodeName_;
    NodeLoadInformationQueryResult NodeLoadInformation_;
};

// {3de74185-d1a8-4973-b14c-7bee458f7f76}
static const GUID CLSID_ComFabricClient_GetReplicaLoadInformationOperation =
    {0x3de74185,0xd1a8,0x4973,{0xb1, 0x4c, 0x7b, 0xee, 0x45, 0x8f, 0x7f, 0x76}};
class ComFabricClient::GetReplicaLoadInformationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetReplicaLoadInformationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetReplicaLoadInformationOperation,
        CLSID_ComFabricClient_GetReplicaLoadInformationOperation,
        GetReplicaLoadInformationOperation,
        ComAsyncOperationContext)

public:
    GetReplicaLoadInformationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        partitionId_(),
        ReplicaLoadInformation_()
    {
    }

    virtual ~GetReplicaLoadInformationOperation() {};

    HRESULT Initialize(
        __in const FABRIC_REPLICA_LOAD_INFORMATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        partitionId_ = Guid(queryDescription->PartitionId);
        replicaOrInstanceId_ = queryDescription->ReplicaOrInstanceId;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetReplicaLoadInformationResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetReplicaLoadInformationOperation> thisOperation(context, CLSID_ComFabricClient_GetReplicaLoadInformationOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetReplicaLoadInformationResult> replicaLoadInformationResult =
                make_com<ComGetReplicaLoadInformationResult, IFabricGetReplicaLoadInformationResult>(move(thisOperation->ReplicaLoadInformation_));
            *result = replicaLoadInformationResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.queryClient_->BeginGetReplicaLoadInformation(
            partitionId_,
            replicaOrInstanceId_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetReplicaLoadInformationComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetReplicaLoadInformationComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetReplicaLoadInformation(operation, ReplicaLoadInformation_);
        TryComplete(operation->Parent, error);
    }

private:
    Guid partitionId_;
    int64 replicaOrInstanceId_;

    ReplicaLoadInformationQueryResult ReplicaLoadInformation_;
};

// {16531FBB-56A8-4378-BC5B-1D283A4519F9}
static const GUID CLSID_ComFabricClient_GetApplicationLoadInformationOperation =
{ 0x16531fbb, 0x56a8, 0x4378, { 0xbc, 0x5b, 0x1d, 0x28, 0x3a, 0x45, 0x19, 0xf9 } };
class ComFabricClient::GetApplicationLoadInformationOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationLoadInformationOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetApplicationLoadInformationOperation,
        CLSID_ComFabricClient_GetApplicationLoadInformationOperation,
        GetApplicationLoadInformationOperation,
        ComAsyncOperationContext)

public:
    GetApplicationLoadInformationOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        ApplicationLoadInformation_()
    {
    }

    virtual ~GetApplicationLoadInformationOperation() {};

    HRESULT Initialize(
        __in const FABRIC_APPLICATION_LOAD_INFORMATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(
            queryDescription->ApplicationName,
            false, // dont accept null
            applicationName_);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetApplicationLoadInformationResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationLoadInformationOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationLoadInformationOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetApplicationLoadInformationResult> dataListResult =
                make_com<ComGetApplicationLoadInformationResult, IFabricGetApplicationLoadInformationResult>(move(thisOperation->ApplicationLoadInformation_));
            *result = dataListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.queryClient_->BeginGetApplicationLoadInformation(
            applicationName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetClusterResourceManagerDataComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnGetClusterResourceManagerDataComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetApplicationLoadInformation(operation, ApplicationLoadInformation_);
        TryComplete(operation->Parent, error);
    }

private:

    std::wstring applicationName_;
    ApplicationLoadInformationQueryResult ApplicationLoadInformation_;
};

// {82419435-7F96-4F47-8523-830DA13AC9C6}
static const GUID CLSID_ComFabricClient_GetServiceNameOperation =
{ 0x82419435, 0x7f96, 0x4f47, { 0x85, 0x23, 0x83, 0xd, 0xa1, 0x3a, 0xc9, 0xc6 } };
class ComFabricClient::GetServiceNameOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetServiceNameOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetServiceNameOperation,
        CLSID_ComFabricClient_GetServiceNameOperation,
        GetServiceNameOperation,
        ComAsyncOperationContext)

public:
    GetServiceNameOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        serviceName_()
    {
    }

    virtual ~GetServiceNameOperation() {};

    HRESULT Initialize(
        __in const FABRIC_SERVICE_NAME_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        partitionId_ = Guid(queryDescription->PartitionId);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetServiceNameResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetServiceNameOperation> thisOperation(context, CLSID_ComFabricClient_GetServiceNameOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetServiceNameResult> nameResult =
                make_com<ComGetServiceNameResult, IFabricGetServiceNameResult>(move(thisOperation->serviceName_));
            *result = nameResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.queryClient_->BeginGetServiceName(
            partitionId_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetServiceNameComplete(operation, false);
            },
            proxySPtr);
        this->OnGetServiceNameComplete(operation, true);
    }

private:
    void OnGetServiceNameComplete(
        __in AsyncOperationSPtr const& operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = Owner.queryClient_->EndGetServiceName(operation, serviceName_);
        TryComplete(operation->Parent, error);
    }

private:
    Guid partitionId_;
    ServiceNameQueryResult serviceName_;
};

// {C31CF595-E481-4EFB-9D80-760ED07F5085}
static const GUID CLSID_ComFabricClient_GetApplicationNameOperation =
{ 0xc31cf595, 0xe481, 0x4efb, { 0x9d, 0x80, 0x76, 0xe, 0xd0, 0x7f, 0x50, 0x85 } };
class ComFabricClient::GetApplicationNameOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationNameOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetApplicationNameOperation,
        CLSID_ComFabricClient_GetApplicationNameOperation,
        GetApplicationNameOperation,
        ComAsyncOperationContext)

public:
    GetApplicationNameOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner),
        applicationName_()
    {
    }

    virtual ~GetApplicationNameOperation() {};

    HRESULT Initialize(
        __in const FABRIC_APPLICATION_NAME_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = NamingUri::TryParse(queryDescription->ServiceName, Constants::FabricClientTrace, serviceName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetApplicationNameResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationNameOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationNameOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetApplicationNameResult> nameResult =
                make_com<ComGetApplicationNameResult, IFabricGetApplicationNameResult>(move(thisOperation->applicationName_));
            *result = nameResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = Owner.queryClient_->BeginGetApplicationName(
            serviceName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetApplicationNameComplete(operation, false);
            },
            proxySPtr);
        this->OnGetApplicationNameComplete(operation, true);
    }

private:
    void OnGetApplicationNameComplete(
        __in AsyncOperationSPtr const& operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = Owner.queryClient_->EndGetApplicationName(operation, applicationName_);
        TryComplete(operation->Parent, error);
    }

private:
    NamingUri serviceName_;
    ApplicationNameQueryResult applicationName_;
};

// {e5671503-1dde-4e89-a8db-52d47a4ddaa6}
static const GUID CLSID_ComFabricClient_RegisterServiceNotificationFilterAsyncOperation =
{ 0xe5671503, 0x1dde, 0x4e89, { 0xa8, 0xdb, 0x52, 0xd4, 0x7a, 0x4d, 0xda, 0xa6 } };
class ComFabricClient::RegisterServiceNotificationFilterAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RegisterServiceNotificationFilterAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        RegisterServiceNotificationFilterAsyncOperation,
        CLSID_ComFabricClient_RegisterServiceNotificationFilterAsyncOperation,
        RegisterServiceNotificationFilterAsyncOperation,
        ComAsyncOperationContext)

public:
    RegisterServiceNotificationFilterAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , filter_(make_shared<Reliability::ServiceNotificationFilter>())
    {
    }

    virtual ~RegisterServiceNotificationFilterAsyncOperation() {};

    static HRESULT Begin(
        ComFabricClient & owner,
        const FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION * description,
        DWORD timeoutMilliseconds,
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        if (description == NULL || callback == NULL || context == NULL)
        {
            return E_POINTER;
        }

        auto operation = make_com<RegisterServiceNotificationFilterAsyncOperation>(owner);

        auto hr = operation->Initialize(
            description,
            timeoutMilliseconds,
            callback);
        if (FAILED(hr)) { return hr; }

        return ComAsyncOperationContext::StartAndDetach(move(operation), context);
    }

    HRESULT Initialize(
        __in const FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION * description,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = filter_->FromPublicApi(*description);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in LONGLONG *result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<RegisterServiceNotificationFilterAsyncOperation> thisOperation(
            context,
            CLSID_ComFabricClient_RegisterServiceNotificationFilterAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *result = thisOperation->filterIdResult_;
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.serviceMgmtClient_->BeginRegisterServiceNotificationFilter(
            filter_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnRegisterFilterComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnRegisterFilterComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceMgmtClient_->EndRegisterServiceNotificationFilter(operation, filterIdResult_);
        TryComplete(operation->Parent, error);
    }

private:
    Reliability::ServiceNotificationFilterSPtr filter_;
    uint64 filterIdResult_;
};

// {cc222b17-f5d5-4041-ba5f-ffc783c18e03}
static const GUID CLSID_ComFabricClient_UnregisterServiceNotificationFilterAsyncOperation =
{ 0xcc222b17, 0xf5d5, 0x4041, { 0xba, 0x5f, 0xff, 0xc7, 0x83, 0xc1, 0x8e, 0x03 } };
class ComFabricClient::UnregisterServiceNotificationFilterAsyncOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UnregisterServiceNotificationFilterAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UnregisterServiceNotificationFilterAsyncOperation,
        CLSID_ComFabricClient_UnregisterServiceNotificationFilterAsyncOperation,
        UnregisterServiceNotificationFilterAsyncOperation,
        ComAsyncOperationContext)

public:
    UnregisterServiceNotificationFilterAsyncOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , filterId_()
    {
    }

    virtual ~UnregisterServiceNotificationFilterAsyncOperation() {};

    static HRESULT Begin(
        ComFabricClient & owner,
        LONGLONG filterId,
        DWORD timeoutMilliseconds,
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        if (callback == NULL || context == NULL)
        {
            return E_POINTER;
        }

        auto operation = make_com<UnregisterServiceNotificationFilterAsyncOperation>(owner);

        auto hr = operation->Initialize(
            filterId,
            timeoutMilliseconds,
            callback);
        if (FAILED(hr)) { return hr; }

        return ComAsyncOperationContext::StartAndDetach(move(operation), context);
    }

    HRESULT Initialize(
        __in LONGLONG filterId,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        filterId_ = filterId;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UnregisterServiceNotificationFilterAsyncOperation> thisOperation(
            context,
            CLSID_ComFabricClient_UnregisterServiceNotificationFilterAsyncOperation);

        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
       Owner.serviceMgmtClient_->BeginUnregisterServiceNotificationFilter(
            filterId_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUnregisterFilterComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnUnregisterFilterComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.serviceMgmtClient_->EndUnregisterServiceNotificationFilter(operation);
        TryComplete(operation->Parent, error);
    }

private:
    uint64 filterId_;
};

// 3eda5d8f-aca4-4526-ad10-1fe8ceb2ebc6
static const GUID CLSID_ComFabricClient_GetApplicationTypePagedListOperation =
{ 0x3eda5d8f, 0xaca4, 0x4526, { 0xad, 0x10, 0x1f, 0xe8, 0xce, 0xb2, 0xeb, 0xc6 } };
class ComFabricClient::GetApplicationTypePagedListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationTypePagedListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
    GetApplicationTypePagedListOperation,
    CLSID_ComFabricClient_GetApplicationTypePagedListOperation,
    GetApplicationTypePagedListOperation,
    ComAsyncOperationContext)

public:
    GetApplicationTypePagedListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , pagedApplicationTypeQueryDescription_()
        , pagingStatus_()
        , applicationTypeList_()
    {
    }

    virtual ~GetApplicationTypePagedListOperation() {};

    // Initializes the variables within this class
    // based on input from the passed in QueryDescription
    HRESULT Initialize(
        __in const PAGED_FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION * queryDescription,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = pagedApplicationTypeQueryDescription_.FromPublicApi(*queryDescription);

        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    // Splits the param result into the list of ApplicationTypes and the PagingStatus
    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetApplicationTypePagedListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationTypePagedListOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationTypePagedListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetApplicationTypePagedListResult> applicationTypeListResult =
                make_com<ComGetApplicationTypePagedListResult, IFabricGetApplicationTypePagedListResult>(move(thisOperation->applicationTypeList_), move(thisOperation->pagingStatus_));
            *result = applicationTypeListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.queryClient_->BeginGetApplicationTypePagedList(
            pagedApplicationTypeQueryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetApplicationTypePagedListComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnGetApplicationTypePagedListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetApplicationTypePagedList(operation, applicationTypeList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

private:
    ApplicationTypeQueryDescription pagedApplicationTypeQueryDescription_;
    PagingStatusUPtr pagingStatus_;
    vector<ApplicationTypeQueryResult> applicationTypeList_;
};

// 1d4e8c86-1987-4c57-91d9-34fb3d175c09
static const GUID CLSID_ComFabricClient_CreateComposeDeploymentOperation =
    { 0x1d4e8c86, 0x1987, 0x4c57, {0x91, 0xd9, 0x34, 0xfb, 0x3d, 0x17, 0x5c, 0x09} };
class ComFabricClient::CreateComposeDeploymentOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateComposeDeploymentOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateComposeDeploymentOperation,
        CLSID_ComFabricClient_CreateComposeDeploymentOperation,
        CreateComposeDeploymentOperation,
        ComAsyncOperationContext)

public:
    CreateComposeDeploymentOperation(__in ComFabricClient & owner)
        : ClientAsyncOperation(owner)
        , deploymentName_()
        , dockerComposeFilePath_()
        , repositoryUserName_()
        , repositoryPassword_()
        , isPasswordEncrypted_(false)
    {
    }

    virtual ~CreateComposeDeploymentOperation()
    {
    }

    HRESULT Initialize(
        LPCWSTR deploymentName,
        LPCWSTR dockerComposeFilePath,
        LPCWSTR repositoryUserName,
        LPCWSTR repositoryPassword,
        BOOLEAN isPasswordEncrypted,
        DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(deploymentName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, deploymentName_);
        if (FAILED(hr)) { return hr; }

        if (repositoryUserName == nullptr && repositoryPassword != nullptr) { return E_INVALIDARG; }

        hr = StringUtility::LpcwstrToWstring(dockerComposeFilePath, false, ParameterValidator::MinStringSize, ParameterValidator::MaxFilePathSize, dockerComposeFilePath_);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(repositoryUserName, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, repositoryUserName_);
        if (FAILED(hr)) { return hr; }

        // TODO: define a size validation for encrypted passwords
        hr = StringUtility::LpcwstrToWstring(repositoryPassword, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, repositoryPassword_);
        if (FAILED(hr)) { return hr; }

        isPasswordEncrypted_ = isPasswordEncrypted == 0 ? false : true;

        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateComposeDeploymentOperation> thisOperation(context, CLSID_ComFabricClient_CreateComposeDeploymentOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.composeMgmtClient_->BeginCreateComposeDeployment(
            deploymentName_,
            dockerComposeFilePath_,
            L"", // overrides file
            repositoryUserName_,
            repositoryPassword_,
            isPasswordEncrypted_,
            this->Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnCreateComplete(operation); },
            proxySPtr);
    }

private:

    void OnCreateComplete(
        AsyncOperationSPtr const & operation)
    {
        auto error = this->Owner.composeMgmtClient_->EndCreateComposeDeployment(operation);
        this->TryComplete(operation->Parent, error);
    }

    wstring deploymentName_;
    wstring dockerComposeFilePath_;
    wstring repositoryUserName_;
    wstring repositoryPassword_;
    bool isPasswordEncrypted_;
};

// F18A4562-3F00-416D-A89C-033951CDA5C5
static const GUID CLSID_ComFabricClient_DeleteComposeDeploymentOperation =
{ 0xf18A4562, 0x3f00, 0x416d, { 0xa8, 0x9c, 0x03, 0x39, 0x51, 0xcd, 0xa5, 0xc5 } };

class ComFabricClient::DeleteComposeDeploymentOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeleteComposeDeploymentOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeleteComposeDeploymentOperation,
        CLSID_ComFabricClient_DeleteComposeDeploymentOperation,
        DeleteComposeDeploymentOperation,
        ComAsyncOperationContext)
public:

    DeleteComposeDeploymentOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , deploymentName_()
    {
    }

    virtual ~DeleteComposeDeploymentOperation() {};

    HRESULT Initialize(
        __in LPCWSTR deploymentName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        hr = StringUtility::LpcwstrToWstring(deploymentName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, deploymentName_);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == nullptr) { return E_POINTER; }

        ComPointer<DeleteComposeDeploymentOperation> thisOperation(context, CLSID_ComFabricClient_DeleteComposeDeploymentOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.composeMgmtClient_->BeginDeleteComposeDeployment(
            deploymentName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeleteComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnDeleteComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.composeMgmtClient_->EndDeleteComposeDeployment(operation);
        TryComplete(operation->Parent, error);
    }

    wstring deploymentName_;
};

// 1760D4C4-ECFE-49F1-93A2-ADA82FCF520D
static const GUID CLSID_ComFabricClient_UpgradeComposeDeploymentOperation =
{ 0x1760d4c4, 0xecfe, 0x49f1, { 0x93, 0xa2, 0xad, 0xa8, 0x2f, 0xcf, 0x52, 0x0d } };

class ComFabricClient::UpgradeComposeDeploymentOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(UpgradeComposeDeploymentOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpgradeComposeDeploymentOperation,
        CLSID_ComFabricClient_UpgradeComposeDeploymentOperation,
        UpgradeComposeDeploymentOperation,
        ComAsyncOperationContext)
public:

    UpgradeComposeDeploymentOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , internaldescription_()
    {
    }

    virtual ~UpgradeComposeDeploymentOperation() {};

    HRESULT Initialize(
        __in FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION const *upgradeDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = internaldescription_.FromPublicApi(*upgradeDescription);
        if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpgradeComposeDeploymentOperation> thisOperation(context, CLSID_ComFabricClient_UpgradeComposeDeploymentOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.composeMgmtClient_->BeginUpgradeComposeDeployment(
            internaldescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnAcceptUpgradeComplete(operation);
            },
            proxySPtr);
    }

private:
    void OnAcceptUpgradeComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.composeMgmtClient_->EndUpgradeComposeDeployment(operation);
        TryComplete(operation->Parent, error);
    }

    ComposeDeploymentUpgradeDescription internaldescription_;
};

// acfd82f6-e414-4c6e-a5de-da4477f0d30d
static const GUID CLSID_ComFabricClient_GetDeployedApplicationPagedListOperation =
{ 0xacfd82f6,0xe414,0x4c6e,{ 0xa5, 0xde, 0xda, 0x44, 0x77, 0xf0, 0xd3, 0x0d } };
class ComFabricClient::GetDeployedApplicationPagedListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedApplicationPagedListOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        GetDeployedApplicationPagedListOperation,
        CLSID_ComFabricClient_GetDeployedApplicationPagedListOperation,
        GetDeployedApplicationPagedListOperation,
        ComAsyncOperationContext)

public:
    GetDeployedApplicationPagedListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , queryDescription_()
        , pagingStatus_()
        , deployedApplicationList_()
    {}

    virtual ~GetDeployedApplicationPagedListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_PAGED_DEPLOYED_APPLICATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ErrorCode err = queryDescription_.FromPublicApi(*queryDescription);

        if (!err.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(err));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedApplicationPagedListResult **result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedApplicationPagedListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedApplicationPagedListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedApplicationPagedListResult> deployedApplicationListResult =
                make_com<ComGetDeployedApplicationPagedListResult, IFabricGetDeployedApplicationPagedListResult>(move(thisOperation->deployedApplicationList_), move(thisOperation->pagingStatus_));
            *result = deployedApplicationListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.queryClient_->BeginGetDeployedApplicationPagedList(
            queryDescription_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetDeployedApplicationPagedListComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnGetDeployedApplicationPagedListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.queryClient_->EndGetDeployedApplicationPagedList(operation, deployedApplicationList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

private:
    DeployedApplicationQueryDescription queryDescription_;
    vector<DeployedApplicationQueryResult> deployedApplicationList_;
    PagingStatusUPtr pagingStatus_;
};

// 9E5003B0-FAA8-435C-9C3D-B1ECA0552B93
static const GUID CLSID_ComFabricClient_RollbackComposeDeploymentOperation = 
{ 0x9e5003b0, 0xfaa8, 0x435c, { 0x9c, 0x3d, 0xb1, 0xec, 0xa0, 0x55, 0x2b, 0x93 } };
class ComFabricClient::RollbackComposeDeploymentOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(RollbackComposeDeploymentOperation)
        
    COM_INTERFACE_AND_DELEGATE_LIST(
        RollbackComposeDeploymentOperation,
        CLSID_ComFabricClient_RollbackComposeDeploymentOperation,
        RollbackComposeDeploymentOperation,
        ComAsyncOperationContext)
public:

    RollbackComposeDeploymentOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , deploymentName_()
    {
    }

    virtual ~RollbackComposeDeploymentOperation() {};

    HRESULT Initialize(
        __in LPCWSTR deploymentName,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        TRY_COM_PARSE_PUBLIC_STRING_OUT(deploymentName, parsedDeploymentName, false);

        this->deploymentName_ = move(parsedDeploymentName);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == nullptr) { return E_POINTER; }

        ComPointer<RollbackComposeDeploymentOperation> thisOperation(context, CLSID_ComFabricClient_RollbackComposeDeploymentOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.composeMgmtClient_->BeginRollbackComposeDeployment(
            deploymentName_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnRollbackComplete(operation);
        },
            proxySPtr);
    }

private:
    void OnRollbackComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.composeMgmtClient_->EndRollbackComposeDeployment(operation);
        TryComplete(operation->Parent, move(error));
    }

    wstring deploymentName_;
};

// {15d0270d-8da4-4430-a1ec-6d77410166ad}
static const GUID CLSID_ComFabricClient_CreateNetworkOperation =
{ 0x15d0270d, 0x8da4, 0x4430,{ 0xa1, 0xec, 0x6d, 0x77, 0x41, 0x01, 0x66, 0xad } };

class ComFabricClient::CreateNetworkOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(CreateNetworkOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        CreateNetworkOperation,
        CLSID_ComFabricClient_CreateNetworkOperation,
        CreateNetworkOperation,
        ComAsyncOperationContext)
public:

    CreateNetworkOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , networkDesc_()
    {
    }

    virtual ~CreateNetworkOperation() {};

    HRESULT Initialize(
        __in wstring &networkName,
        __in NetworkDescription &networkDesc,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        ServiceModel::ModelV2::NetworkResourceDescription networkResourceDescription(networkName, networkDesc);

        networkDesc_ = move(networkResourceDescription);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<CreateNetworkOperation> thisOperation(context, CLSID_ComFabricClient_CreateNetworkOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginCreateNetwork(
            networkDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateComplete(operation);
            },
            proxySPtr);
    }

private:

    void OnCreateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndCreateNetwork(operation);
        TryComplete(operation->Parent, error);
    }

    ServiceModel::ModelV2::NetworkResourceDescription networkDesc_;
};

// {b9a1fc69-8d65-4507-a6a3-fc8965c73297}
static const GUID CLSID_ComFabricClient_DeleteNetworkOperation =
{ 0xb9a1fc69, 0x8d65, 0x4507,{ 0xa6, 0xa3, 0xfc, 0x89, 0x65, 0xc7, 0x32, 0x97 } };

class ComFabricClient::DeleteNetworkOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(DeleteNetworkOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        DeleteNetworkOperation,
        CLSID_ComFabricClient_DeleteNetworkOperation,
        DeleteNetworkOperation,
        ComAsyncOperationContext)
public:

    DeleteNetworkOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , deleteNetworkDesc_()
    {
    }

    virtual ~DeleteNetworkOperation() {};

    HRESULT Initialize(
        __in DeleteNetworkDescription &deleteNetworkDesc,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        deleteNetworkDesc_ = move(deleteNetworkDesc);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeleteNetworkOperation> thisOperation(context, CLSID_ComFabricClient_DeleteNetworkOperation);
        return thisOperation->ComAsyncOperationContextEnd();
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginDeleteNetwork(
            deleteNetworkDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCreateComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnCreateComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndDeleteNetwork(operation);
        TryComplete(operation->Parent, error);
    }

    DeleteNetworkDescription deleteNetworkDesc_;
};

// {cdd6e0e1-a5de-4fa5-8654-63e3d55d01dc}
static const GUID CLSID_ComFabricClient_GetNetworkListOperation =
{ 0xcdd6e0e1, 0xa5de, 0x4fa5,{ 0x86, 0x54, 0x63, 0xe3, 0xd5, 0x5d, 0x01, 0xdc } };

class ComFabricClient::GetNetworkListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetNetworkListOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetNetworkListOperation,
            CLSID_ComFabricClient_GetNetworkListOperation,
            GetNetworkListOperation,
            ComAsyncOperationContext)
public:

    GetNetworkListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , networkQueryDesc_()
        , networkList_()
        , pagingStatus_()
    {
    }

    virtual ~GetNetworkListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_NETWORK_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }
        
        auto error = networkQueryDesc_.FromPublicApi(*queryDescription);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }		

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetNetworkListResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetNetworkListOperation> thisOperation(context, CLSID_ComFabricClient_GetNetworkListOperation);
        
        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            vector<NetworkInformation> networkInformationList;

            for (auto iter = thisOperation->networkList_.begin();
                iter != thisOperation->networkList_.end();
                ++iter)
            {
                NetworkInformation networkInformation(iter->NetworkName, iter->NetworkAddressPrefix, iter->NetworkType, iter->NetworkStatus);

                networkInformationList.push_back(move(networkInformation));
            }

            Common::ComPointer<IFabricGetNetworkListResult> networkListResult =
                make_com<ComGetNetworkListResult, IFabricGetNetworkListResult>(move(networkInformationList), move(thisOperation->pagingStatus_));
            *result = networkListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginGetNetworkList(
            networkQueryDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetNetworkListComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnGetNetworkListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndGetNetworkList(operation, networkList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

    NetworkQueryDescription networkQueryDesc_;
    vector<ModelV2::NetworkResourceDescriptionQueryResult> networkList_;
    PagingStatusUPtr pagingStatus_;
};

// {8017e230-a1f5-402b-9c4d-2732fdc7042c}
static const GUID CLSID_ComFabricClient_GetNetworkApplicationListOperation =
{ 0x8017e230, 0xa1f5, 0x402b,{ 0x9c, 0x4d, 0x27, 0x32, 0xfd, 0xc7, 0x04, 0x2c } };

class ComFabricClient::GetNetworkApplicationListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetNetworkApplicationListOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetNetworkApplicationListOperation,
            CLSID_ComFabricClient_GetNetworkApplicationListOperation,
            GetNetworkApplicationListOperation,
            ComAsyncOperationContext)
public:

    GetNetworkApplicationListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , networkApplicationQueryDesc_()
        , networkApplicationList_()
        , pagingStatus_()
    {
    }

    virtual ~GetNetworkApplicationListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_NETWORK_APPLICATION_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = networkApplicationQueryDesc_.FromPublicApi(*queryDescription);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetNetworkApplicationListResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetNetworkApplicationListOperation> thisOperation(context, CLSID_ComFabricClient_GetNetworkApplicationListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetNetworkApplicationListResult> networkApplicationListResult =
                make_com<ComGetNetworkApplicationListResult, IFabricGetNetworkApplicationListResult>(move(thisOperation->networkApplicationList_), move(thisOperation->pagingStatus_));
            *result = networkApplicationListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginGetNetworkApplicationList(
            networkApplicationQueryDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetNetworkApplicationListComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnGetNetworkApplicationListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndGetNetworkApplicationList(operation, networkApplicationList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

    NetworkApplicationQueryDescription networkApplicationQueryDesc_;
    vector<NetworkApplicationQueryResult> networkApplicationList_;
    PagingStatusUPtr pagingStatus_;
};

// {86c1e38d-eaca-4697-be24-75ffc9dd1208}
static const GUID CLSID_ComFabricClient_GetNetworkNodeListOperation =
{ 0x86c1e38d, 0xeaca, 0x4697,{ 0xbe, 0x24, 0x75, 0xff, 0xc9, 0xdd, 0x12, 0x08 } };

class ComFabricClient::GetNetworkNodeListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetNetworkNodeListOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetNetworkNodeListOperation,
            CLSID_ComFabricClient_GetNetworkNodeListOperation,
            GetNetworkNodeListOperation,
            ComAsyncOperationContext)
public:

    GetNetworkNodeListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , networkNodeQueryDesc_()
        , networkNodeList_()
        , pagingStatus_()
    {
    }

    virtual ~GetNetworkNodeListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_NETWORK_NODE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = networkNodeQueryDesc_.FromPublicApi(*queryDescription);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetNetworkNodeListResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetNetworkNodeListOperation> thisOperation(context, CLSID_ComFabricClient_GetNetworkNodeListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetNetworkNodeListResult> networkNodeListResult =
                make_com<ComGetNetworkNodeListResult, IFabricGetNetworkNodeListResult>(move(thisOperation->networkNodeList_), move(thisOperation->pagingStatus_));
            *result = networkNodeListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginGetNetworkNodeList(
            networkNodeQueryDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetNetworkNodeListComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnGetNetworkNodeListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndGetNetworkNodeList(operation, networkNodeList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

    NetworkNodeQueryDescription networkNodeQueryDesc_;
    vector<NetworkNodeQueryResult> networkNodeList_;
    PagingStatusUPtr pagingStatus_;
};

// {ee0b0721-8ba3-485f-8a6d-a0e7d2a6d537}
static const GUID CLSID_ComFabricClient_GetApplicationNetworkListOperation =
{ 0xee0b0721, 0x8ba3, 0x485f,{ 0x8a, 0x6d, 0xa0, 0xe7, 0xd2, 0xa6, 0xd5, 0x37 } };

class ComFabricClient::GetApplicationNetworkListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetApplicationNetworkListOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetApplicationNetworkListOperation,
            CLSID_ComFabricClient_GetApplicationNetworkListOperation,
            GetApplicationNetworkListOperation,
            ComAsyncOperationContext)
public:

    GetApplicationNetworkListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , applicationNetworkQueryDesc_()
        , applicationNetworkList_()
        , pagingStatus_()
    {
    }

    virtual ~GetApplicationNetworkListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_APPLICATION_NETWORK_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = applicationNetworkQueryDesc_.FromPublicApi(*queryDescription);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetApplicationNetworkListResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetApplicationNetworkListOperation> thisOperation(context, CLSID_ComFabricClient_GetApplicationNetworkListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetApplicationNetworkListResult> applicationNetworkListResult =
                make_com<ComGetApplicationNetworkListResult, IFabricGetApplicationNetworkListResult>(move(thisOperation->applicationNetworkList_), move(thisOperation->pagingStatus_));
            *result = applicationNetworkListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginGetApplicationNetworkList(
            applicationNetworkQueryDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetApplicationNetworkListComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnGetApplicationNetworkListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndGetApplicationNetworkList(operation, applicationNetworkList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

    ApplicationNetworkQueryDescription applicationNetworkQueryDesc_;
    vector<ApplicationNetworkQueryResult> applicationNetworkList_;
    PagingStatusUPtr pagingStatus_;
};

// {f963e6ac-5d70-40d4-9272-b4d3b16e3707}
static const GUID CLSID_ComFabricClient_GetDeployedNetworkListOperation =
{ 0xf963e6ac, 0x5d70, 0x40d4,{ 0x92, 0x72, 0xb4, 0xd3, 0xb1, 0x6e, 0x37, 0x07 } };

class ComFabricClient::GetDeployedNetworkListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedNetworkListOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetDeployedNetworkListOperation,
            CLSID_ComFabricClient_GetDeployedNetworkListOperation,
            GetDeployedNetworkListOperation,
            ComAsyncOperationContext)
public:

    GetDeployedNetworkListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , deployedNetworkQueryDesc_()
        , deployedNetworkList_()
        , pagingStatus_()
    {
    }

    virtual ~GetDeployedNetworkListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_NETWORK_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = deployedNetworkQueryDesc_.FromPublicApi(*queryDescription);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedNetworkListResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedNetworkListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedNetworkListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedNetworkListResult> deployedNetworkListResult =
                make_com<ComGetDeployedNetworkListResult, IFabricGetDeployedNetworkListResult>(move(thisOperation->deployedNetworkList_), move(thisOperation->pagingStatus_));
            *result = deployedNetworkListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginGetDeployedNetworkList(
            deployedNetworkQueryDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetDeployedNetworkListComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnGetDeployedNetworkListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndGetDeployedNetworkList(operation, deployedNetworkList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

    DeployedNetworkQueryDescription deployedNetworkQueryDesc_;
    vector<DeployedNetworkQueryResult> deployedNetworkList_;
    PagingStatusUPtr pagingStatus_;
};

// {412f6a08-15e8-4b04-a2fe-3d6215eb6987}
static const GUID CLSID_ComFabricClient_GetDeployedNetworkCodePackageListOperation =
{ 0x412f6a08, 0x15e8, 0x4b04,{ 0xa2, 0xfe, 0x3d, 0x62, 0x15, 0xeb, 0x69, 0x87 } };

class ComFabricClient::GetDeployedNetworkCodePackageListOperation :
    public ComFabricClient::ClientAsyncOperation
{
    DENY_COPY(GetDeployedNetworkCodePackageListOperation)

        COM_INTERFACE_AND_DELEGATE_LIST(
            GetDeployedNetworkCodePackageListOperation,
            CLSID_ComFabricClient_GetDeployedNetworkCodePackageListOperation,
            GetDeployedNetworkCodePackageListOperation,
            ComAsyncOperationContext)
public:

    GetDeployedNetworkCodePackageListOperation(ComFabricClient& owner)
        : ClientAsyncOperation(owner)
        , deployedNetworkCodePackageQueryDesc_()
        , deployedNetworkCodePackageList_()
        , pagingStatus_()
    {
    }

    virtual ~GetDeployedNetworkCodePackageListOperation() {};

    HRESULT Initialize(
        __in const FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
        __in DWORD timeoutMilliSeconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = ClientAsyncOperation::Initialize(timeoutMilliSeconds, callback);
        if (FAILED(hr)) { return hr; }

        auto error = deployedNetworkCodePackageQueryDesc_.FromPublicApi(*queryDescription);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __in IFabricGetDeployedNetworkCodePackageListResult ** result)
    {
        if (context == NULL || result == NULL) { return E_POINTER; }

        ComPointer<GetDeployedNetworkCodePackageListOperation> thisOperation(context, CLSID_ComFabricClient_GetDeployedNetworkCodePackageListOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            Common::ComPointer<IFabricGetDeployedNetworkCodePackageListResult> deployedNetworkCodePackageListResult =
                make_com<ComGetDeployedNetworkCodePackageListResult, IFabricGetDeployedNetworkCodePackageListResult>(move(thisOperation->deployedNetworkCodePackageList_), move(thisOperation->pagingStatus_));
            *result = deployedNetworkCodePackageListResult.DetachNoRelease();
        }

        return hr;
    }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        Owner.networkMgmtClient_->BeginGetDeployedNetworkCodePackageList(
            deployedNetworkCodePackageQueryDesc_,
            Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetDeployedNetworkCodePackageListComplete(operation);
        },
            proxySPtr);
    }

private:

    void OnGetDeployedNetworkCodePackageListComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = Owner.networkMgmtClient_->EndGetDeployedNetworkCodePackageList(operation, deployedNetworkCodePackageList_, pagingStatus_);
        TryComplete(operation->Parent, error);
    }

    DeployedNetworkCodePackageQueryDescription deployedNetworkCodePackageQueryDesc_;
    vector<DeployedNetworkCodePackageQueryResult> deployedNetworkCodePackageList_;
    PagingStatusUPtr pagingStatus_;
};

// ****************************************************************************
// ComFabricClient methods
// ****************************************************************************

ULONG STDMETHODCALLTYPE ComFabricClient::TryAddRef()
{
    return this->BaseTryAddRef();
}

ComFabricClient::ComFabricClient(IClientFactoryPtr const& factoryPtr)
    : factoryPtr_(factoryPtr)
    , serviceLocationChangeTrackerLock_()
    , serviceLocationChangeHandlers_()

{
}

ComFabricClient::~ComFabricClient()
{
    for (auto itr = serviceLocationChangeHandlers_.begin(); itr != serviceLocationChangeHandlers_.end(); ++itr)
    {
        itr->second->Cancel();
    }
    serviceLocationChangeHandlers_.clear();
}

HRESULT ComFabricClient::Initialize()
{
    if (!factoryPtr_.get())
    {
        return E_INVALIDARG;
    }

    auto error = Common::ComUtility::CheckMTA();
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateApplicationManagementClient(appMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateClusterManagementClient(clusterMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateRepairManagementClient(repairMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateAccessControlClient(accessControlClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateHealthClient(healthClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreatePropertyManagementClient(propertyMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateQueryClient(queryClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateServiceGroupManagementClient(serviceGroupMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateServiceManagementClient(serviceMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateSettingsClient(settingsClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateInfrastructureServiceClient(infrastructureClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateInternalInfrastructureServiceClient(internalInfrastructureClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateTestManagementClient(testManagementClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateTestManagementClientInternal(testManagementClientInternal_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateFaultManagementClient(faultManagementClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateImageStoreClient(imageStoreClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateTestClient(testClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateComposeManagementClient(composeMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateSecretStoreClient(secretStoreClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    error = factoryPtr_->CreateNetworkManagementClient(networkMgmtClient_);
    if (!error.IsSuccess()) { return error.ToHResult(); }

    return S_OK;
}

HRESULT ComFabricClient::CreateComFabricClient(IClientFactoryPtr const&factory, __out ComPointer<ComFabricClient> &resultCPtr)
{
    ComPointer<ComFabricClient> clientCPtr = make_com<ComFabricClient>(factory);
    auto hr = clientCPtr->Initialize();
    if (!FAILED(hr))
    {
        resultCPtr = move(clientCPtr);
    }

    return hr;
}

//
// IFabricClientSettings Implementation
//

HRESULT ComFabricClient::SetSecurityCredentials(
    /* [in] */ FABRIC_SECURITY_CREDENTIALS const* securityCredentials)
{
    if (securityCredentials == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    Transport::SecuritySettings securitySettings;
    auto error = Transport::SecuritySettings::FromPublicApi(*securityCredentials, securitySettings);
    if(!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

    Common::ErrorCode errorCode = settingsClient_->SetSecurity(move(securitySettings));
    if (!errorCode.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(errorCode));
    }

    return S_OK;
}

HRESULT ComFabricClient::SetKeepAlive(
    /* [in] */ ULONG keepAliveIntervalInSeconds)
{
    Common::ErrorCode errorCode = settingsClient_->SetKeepAlive(keepAliveIntervalInSeconds);
    if (!errorCode.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(ErrorCodeValue::InvalidOperation);
    }

    return S_OK;
}

//
// IFabricClientSettings2 Implementation
//

HRESULT ComFabricClient::GetSettings(
    /* [out, retval] */ IFabricClientSettingsResult **result)
{
    if (result == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    FabricClientSettings settings = settingsClient_->GetSettings();

    ComPointer<IFabricClientSettingsResult> resultCPtr =
        make_com<ComClientSettingsResult, IFabricClientSettingsResult>(move(settings));

    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}

HRESULT ComFabricClient::SetSettings(
    /* [in] */ FABRIC_CLIENT_SETTINGS const* fabricClientSettings)
{
    if (fabricClientSettings == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ServiceModel::FabricClientSettings settings;
    Common::ErrorCode errorCode = settings.FromPublicApi(*fabricClientSettings);
    if (!errorCode.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(errorCode));
    }

    errorCode = settingsClient_->SetSettings(move(settings));
    if (!errorCode.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(errorCode));
    }

    return S_OK;
}

//
// IFabricClusterManagementClient methods.
//
HRESULT STDMETHODCALLTYPE ComFabricClient::BeginNodeStateRemoved(
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || nodeName == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<NodeStateRemovedAsyncOperation> operation = make_com<NodeStateRemovedAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        nodeName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndNodeStateRemoved(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(NodeStateRemovedAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginRecoverPartitions(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RecoverPartitionsAsyncOperation> operation = make_com<RecoverPartitionsAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndRecoverPartitions(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RecoverPartitionsAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginRecoverPartition(
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RecoverPartitionAsyncOperation> operation = make_com<RecoverPartitionAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        partitionId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndRecoverPartition(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RecoverPartitionAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginRecoverServicePartitions(
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RecoverServicePartitionsAsyncOperation> operation = make_com<RecoverServicePartitionsAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        serviceName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndRecoverServicePartitions(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RecoverServicePartitionsAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginRecoverSystemPartitions(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RecoverSystemPartitionsAsyncOperation> operation = make_com<RecoverSystemPartitionsAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndRecoverSystemPartitions(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RecoverSystemPartitionsAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginResetPartitionLoad(
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ResetPartitionLoadAsyncOperation> operation = make_com<ResetPartitionLoadAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        partitionId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndResetPartitionLoad(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ResetPartitionLoadAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginToggleVerboseServicePlacementHealthReporting(
    /* [in] */ BOOLEAN enabled,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ToggleVerboseServicePlacementHealthReportingAsyncOperation> operation = make_com<ToggleVerboseServicePlacementHealthReportingAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        enabled,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndToggleVerboseServicePlacementHealthReporting(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ToggleVerboseServicePlacementHealthReportingAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginDeactivateNode(
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ FABRIC_NODE_DEACTIVATION_INTENT intent,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || nodeName == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (intent < FABRIC_NODE_DEACTIVATION_INTENT::FABRIC_NODE_DEACTIVATION_INTENT_PAUSE ||
        intent > FABRIC_NODE_DEACTIVATION_INTENT::FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_NODE)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    ComPointer<DeActivateNodeAsyncOperation> operation = make_com<DeActivateNodeAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        nodeName,
        intent,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndDeactivateNode(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeActivateNodeAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginActivateNode(
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (nodeName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ActivateNodeAsyncOperation> operation = make_com<ActivateNodeAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        nodeName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndActivateNode(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ActivateNodeAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginProvisionFabric(
    /* [in] */ LPCWSTR codeFilepath,
    /* [in] */ LPCWSTR clusterManifestFilepath,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || ((codeFilepath == NULL) && (clusterManifestFilepath == NULL)))
    {
        //
        // codeFilepath and clusterManifestFilepath cannot be both NULL
        //
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    ComPointer<ProvisionFabricAsyncOperation> operation = make_com<ProvisionFabricAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        codeFilepath,
        clusterManifestFilepath,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndProvisionFabric(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ProvisionFabricAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginUpgradeFabric(
    /* [in] */ const FABRIC_UPGRADE_DESCRIPTION *upgradeDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (upgradeDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(upgradeDescription);

    ComPointer<UpgradeFabricAsyncOperation> operation = make_com<UpgradeFabricAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        upgradeDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndUpgradeFabric(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpgradeFabricAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetFabricUpgradeProgress(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetFabricUpgradeProgressAsyncOperation> operation = make_com<GetFabricUpgradeProgressAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetFabricUpgradeProgress(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricUpgradeProgressResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetFabricUpgradeProgressAsyncOperation::End(
        context,
        IID_IFabricUpgradeProgressResult2,
        reinterpret_cast<void**>(result)));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginMoveNextFabricUpgradeDomain(
    /* [in] */ IFabricUpgradeProgressResult2 *progress,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (progress == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<MoveNextUpgradeDomainAsyncOperation> operation = make_com<MoveNextUpgradeDomainAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        progress,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndMoveNextFabricUpgradeDomain(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(MoveNextUpgradeDomainAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginMoveNextFabricUpgradeDomain2(
    /* [in] */ LPCWSTR nextDomain,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (nextDomain == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<MoveNextUpgradeDomainAsyncOperation2> operation = make_com<MoveNextUpgradeDomainAsyncOperation2>(*this);

    HRESULT hr = operation->Initialize(
        nextDomain,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndMoveNextFabricUpgradeDomain2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(MoveNextUpgradeDomainAsyncOperation2::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginStartInfrastructureTask(
    /* [in] */ FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION const* description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (description == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<StartInfrastructureTaskAsyncOperation> operation = make_com<StartInfrastructureTaskAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndStartInfrastructureTask(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ BOOLEAN *isComplete)
{
    return ComUtility::OnPublicApiReturn(StartInfrastructureTaskAsyncOperation::End(context, isComplete));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginFinishInfrastructureTask(
    /* [in] */ LPCWSTR taskId,
    /* [in] */ ULONGLONG instanceId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<FinishInfrastructureTaskAsyncOperation> operation = make_com<FinishInfrastructureTaskAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        taskId,
        instanceId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndFinishInfrastructureTask(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ BOOLEAN *isComplete)
{
    return ComUtility::OnPublicApiReturn(FinishInfrastructureTaskAsyncOperation::End(context, isComplete));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginUnprovisionFabric(
    /* [in] */ LPCWSTR codeVersion,
    /* [in] */ LPCWSTR configVersion,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || ((codeVersion == NULL) && (configVersion == NULL)))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<UnprovisionFabricAsyncOperation> operation = make_com<UnprovisionFabricAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        codeVersion,
        configVersion,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndUnprovisionFabric(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UnprovisionFabricAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetClusterManifest(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetClusterManifestAsyncOperation> operation = make_com<GetClusterManifestAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        NULL,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetClusterManifest(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterManifestAsyncOperation::End(context, result));
}

//
// IFabricClusterManagementClient3
//

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginUpdateFabricUpgrade(
    /* [in] */ const FABRIC_UPGRADE_UPDATE_DESCRIPTION *updateDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (updateDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(updateDescription);

    auto operation = make_com<UpdateFabricUpgradeAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        updateDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndUpdateFabricUpgrade(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpdateFabricUpgradeAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginRollbackFabricUpgrade(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComUtility::OnPublicApiReturn(RollbackFabricUpgradeAsyncOperation::Begin(
        *this,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndRollbackFabricUpgrade(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RollbackFabricUpgradeAsyncOperation::End(context));
}

//
// IFabricClusterManagementClient8
//

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetClusterManifest2(
    /* [in] */ const FABRIC_CLUSTER_MANIFEST_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<GetClusterManifestAsyncOperation> operation = make_com<GetClusterManifestAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetClusterManifest2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterManifestAsyncOperation::End(context, result));
}

//
// IFabricHealthClient methods.
//
HRESULT ComFabricClient::ReportHealth(
    /* [in] */ const FABRIC_HEALTH_REPORT * publicHealthReport)
{
    // Null send options
    return this->ReportHealth2(publicHealthReport, nullptr);
}

HRESULT ComFabricClient::BeginGetClusterHealth(
    /* [in] */ const FABRIC_CLUSTER_HEALTH_POLICY *healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetClusterHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetClusterHealth(
/* [in] */ IFabricAsyncOperationContext *context,
/* [retval][out] */ IFabricClusterHealthResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetNodeHealth(
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ const FABRIC_CLUSTER_HEALTH_POLICY * healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_NODE_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.NodeName = nodeName;
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetNodeHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetNodeHealth(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricNodeHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetNodeHealthOperation::End(context, result));
}

//
// GetApplicationHealth
//

HRESULT ComFabricClient::BeginGetApplicationHealth(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY * healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.ApplicationName = applicationName;
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetApplicationHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetApplicationHealth(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricApplicationHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationHealthOperation::End(context, result));
}

//
// GetServiceHealth
//

HRESULT ComFabricClient::BeginGetServiceHealth(
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY * healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.ServiceName = serviceName;
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetServiceHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetServiceHealth(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricServiceHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetServiceHealthOperation::End(context, result));
}

//
// GetPartitionHealth
//

HRESULT ComFabricClient::BeginGetPartitionHealth(
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY * healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.PartitionId = partitionId;
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetPartitionHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetPartitionHealth(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricPartitionHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetPartitionHealthOperation::End(context, result));
}

//
// GetReplicaHealth
//

HRESULT ComFabricClient::BeginGetReplicaHealth(
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY * healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.PartitionId = partitionId;
    queryDescription.ReplicaOrInstanceId = replicaId;
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetReplicaHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetReplicaHealth(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricReplicaHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetReplicaHealthOperation::End(context, result));
}

//
// GetDeployedApplicationHealth
//

HRESULT ComFabricClient::BeginGetDeployedApplicationHealth(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.ApplicationName = applicationName;
    queryDescription.NodeName = nodeName;
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetDeployedApplicationHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetDeployedApplicationHealth(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricDeployedApplicationHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedApplicationHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedServicePackageHealth(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.ApplicationName = applicationName;
    queryDescription.ServiceManifestName = serviceManifestName;
    queryDescription.NodeName = nodeName;
    queryDescription.HealthPolicy = healthPolicy;
    return this->BeginGetDeployedServicePackageHealth2(&queryDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndGetDeployedServicePackageHealth(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricDeployedServicePackageHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedServicePackageHealthOperation::End(context, result));
}

//
// IFabricHealthClient2 methods.
//
HRESULT ComFabricClient::BeginGetClusterHealth2(
    /* [in] */ const FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetClusterHealthOperation> operation = make_com<GetClusterHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetClusterHealth2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricClusterHealthResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetNodeHealth2(
    /* [in] */ const FABRIC_NODE_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetNodeHealthOperation> operation = make_com<GetNodeHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetNodeHealth2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricNodeHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetNodeHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetApplicationHealth2(
    /* [in] */ const FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetApplicationHealthOperation> operation = make_com<GetApplicationHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationHealth2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricApplicationHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetServiceHealth2(
    /* [in] */ const FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetServiceHealthOperation> operation = make_com<GetServiceHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceHealth2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricServiceHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetServiceHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetPartitionHealth2(
    /* [in] */ const FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetPartitionHealthOperation> operation = make_com<GetPartitionHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetPartitionHealth2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricPartitionHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetPartitionHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetReplicaHealth2(
    /* [in] */ const FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetReplicaHealthOperation> operation = make_com<GetReplicaHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetReplicaHealth2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricReplicaHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetReplicaHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedApplicationHealth2(
    /* [in] */ const FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetDeployedApplicationHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedApplicationHealth2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricDeployedApplicationHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedApplicationHealthOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedServicePackageHealth2(
    /* [in] */ const FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetDeployedServicePackageHealthOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedServicePackageHealth2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricDeployedServicePackageHealthResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedServicePackageHealthOperation::End(context, result));
}

//
// IFabricHealthClient3 methods.
//
HRESULT ComFabricClient::BeginGetClusterHealthChunk(
    /* [in] */ const FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_OPTIONAL_PARAM_RESERVED_FIELD_NOT_NULL(queryDescription);

    auto operation = make_com<GetClusterHealthChunkOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetClusterHealthChunk(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetClusterHealthChunkResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterHealthChunkOperation::End(context, result));
}


//
// IFabricHealthClient4 methods.
//
HRESULT ComFabricClient::ReportHealth2(
    /* [in] */ const FABRIC_HEALTH_REPORT * publicHealthReport,
    /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS * publicSendOptions)
{
    if (publicHealthReport == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HealthReport healthReport;
    Common::ErrorCode error = healthReport.FromPublicApi(*publicHealthReport);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (publicSendOptions != nullptr)
    {
        sendOptions = make_unique<HealthReportSendOptions>();
        error = sendOptions->FromPublicApi(*publicSendOptions);

        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(error));
        }
    }

    return ComUtility::OnPublicApiReturn(healthClient_->ReportHealth(move(healthReport), move(sendOptions)));
}

//
// IFabricPropertyManagementClient methods.
//
HRESULT ComFabricClient::BeginCreateName(
    /* [string][in] */ LPCWSTR name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateNameAsyncOperation> operation = make_com<CreateNameAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndCreateName(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateNameAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginDeleteName(
    /* [string][in] */ LPCWSTR name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<DeleteNameAsyncOperation> operation = make_com<DeleteNameAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndDeleteName(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeleteNameAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginNameExists(
    /* [string][in] */ LPCWSTR name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<NameExistsAsyncOperation> operation = make_com<NameExistsAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndNameExists(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ BOOLEAN * value)
{
    return ComUtility::OnPublicApiReturn(NameExistsAsyncOperation::End(context, value));
}

HRESULT ComFabricClient::BeginEnumerateSubNames(
    /* [string][in] */ LPCWSTR name,
    /* [in] */ IFabricNameEnumerationResult *previousResult,
    /* [in] */ BOOLEAN  recursive,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<EnumerateSubNamesAsyncOperation> operation = make_com<EnumerateSubNamesAsyncOperation>(*this);

    ComPointer<ComNameEnumerationResult> enumCPtr;
    if (previousResult != NULL)
    {
        HRESULT hr = previousResult->QueryInterface(
            CLSID_ComNameEnumerationResult,
            reinterpret_cast<void**>(enumCPtr.InitializationAddress()));
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    }

    HRESULT hr = operation->Initialize(
        name,
        std::move(enumCPtr),
        recursive,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndEnumerateSubNames(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricNameEnumerationResult **batchEnumerator)
{
    return ComUtility::OnPublicApiReturn(EnumerateSubNamesAsyncOperation::End(context, batchEnumerator));
}

HRESULT ComFabricClient::BeginPutPropertyBinary(
    /* [string][in] */ FABRIC_URI name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ ULONG dataLength,
    /* [in] */ BYTE const * data,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComUtility::OnPublicApiReturn(this->BeginPutProperty(
        name,
        propertyName,
        dataLength,
        data,
        FABRIC_PROPERTY_TYPE_BINARY,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT ComFabricClient::EndPutPropertyBinary(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(this->EndPutProperty(context));
}

HRESULT ComFabricClient::BeginPutPropertyInt64(
    /* [string][in] */ FABRIC_URI name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ LONGLONG data,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComUtility::OnPublicApiReturn(this->BeginPutProperty(
        name,
        propertyName,
        static_cast<ULONG>(sizeof(data)),
        reinterpret_cast<BYTE const *>(&data),
        FABRIC_PROPERTY_TYPE_INT64,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT ComFabricClient::EndPutPropertyInt64(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(this->EndPutProperty(context));
}

HRESULT ComFabricClient::BeginPutPropertyDouble(
    /* [string][in] */ FABRIC_URI name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ double data,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComUtility::OnPublicApiReturn(this->BeginPutProperty(
        name,
        propertyName,
        static_cast<ULONG>(sizeof(data)),
        reinterpret_cast<BYTE const *>(&data),
        FABRIC_PROPERTY_TYPE_DOUBLE,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT ComFabricClient::EndPutPropertyDouble(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(this->EndPutProperty(context));
}

HRESULT ComFabricClient::BeginPutPropertyWString(
    /* [string][in] */ FABRIC_URI name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ LPCWSTR data,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComUtility::OnPublicApiReturn(this->BeginPutProperty(
        name,
        propertyName,
        StringUtility::GetDataLength(data),
        reinterpret_cast<BYTE const *>(data),
        FABRIC_PROPERTY_TYPE_WSTRING,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT ComFabricClient::EndPutPropertyWString(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(this->EndPutProperty(context));
}

HRESULT ComFabricClient::BeginPutPropertyGuid(
    /* [string][in] */ FABRIC_URI name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ GUID const * data,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComUtility::OnPublicApiReturn(this->BeginPutProperty(
        name,
        propertyName,
        static_cast<ULONG>(sizeof(*data)),
        reinterpret_cast<BYTE const *>(data),
        FABRIC_PROPERTY_TYPE_GUID,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT ComFabricClient::EndPutPropertyGuid(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(this->EndPutProperty(context));
}

HRESULT ComFabricClient::BeginPutProperty(
    /* [string][in] */ LPCWSTR name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ ULONG dataLength,
    /* [in] */ BYTE const *data,
    /* [in] */ FABRIC_PROPERTY_TYPE_ID typeId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (data == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<PutPropertyAsyncOperation> operation = make_com<PutPropertyAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        propertyName,
        dataLength,
        data,
        typeId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndPutProperty(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(PutPropertyAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginDeleteProperty(
    /* [string][in] */ LPCWSTR name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<DeletePropertyAsyncOperation> operation = make_com<DeletePropertyAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        propertyName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndDeleteProperty(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeletePropertyAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetProperty(
    /* [string][in] */ LPCWSTR name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetPropertyAsyncOperation> operation = make_com<GetPropertyAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        propertyName,
        true,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndGetProperty(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricPropertyValueResult **result)
{
    return ComUtility::OnPublicApiReturn(GetPropertyAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetPropertyMetadata(
    /* [string][in] */ LPCWSTR name,
    /* [string][in] */ LPCWSTR propertyName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetPropertyAsyncOperation> operation = make_com<GetPropertyAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        propertyName,
        false,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndGetPropertyMetadata(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricPropertyMetadataResult **result)
{
    return ComUtility::OnPublicApiReturn(GetPropertyAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginSubmitPropertyBatch(
    /* [in] */ FABRIC_URI name,
    /* [in] */ ULONG operationCount,
    /* [in size_is(operationCount)] */ const FABRIC_PROPERTY_BATCH_OPERATION * operations,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) || (operations == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (operationCount == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    ComPointer<PropertyBatchAsyncOperation> operation = make_com<PropertyBatchAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        operationCount,
        operations,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndSubmitPropertyBatch(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out] */ ULONG * failedOperationIndexInRequest,
    /* [out, retval] */ IFabricPropertyBatchResult ** result)
{
    return ComUtility::OnPublicApiReturn(PropertyBatchAsyncOperation::End(context, failedOperationIndexInRequest, result));
}

HRESULT ComFabricClient::BeginEnumerateProperties(
    /* [in] */ FABRIC_URI name,
    /* [in] */ BOOLEAN includeValues,
    /* [in] */ IFabricPropertyEnumerationResult * previousResult,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<EnumeratePropertiesAsyncOperation> operation = make_com<EnumeratePropertiesAsyncOperation>(*this);

    ComEnumeratePropertiesResultCPtr enumCPtr;
    if (previousResult != NULL)
    {
        HRESULT hr = previousResult->QueryInterface(
            CLSID_ComEnumeratePropertiesResult,
            reinterpret_cast<void**>(enumCPtr.InitializationAddress()));
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    }

    HRESULT hr = operation->Initialize(
        name,
        includeValues ? true : false,
        std::move(enumCPtr),
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndEnumerateProperties(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricPropertyEnumerationResult ** result)
{
    return ComUtility::OnPublicApiReturn(EnumeratePropertiesAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginPutCustomPropertyOperation(
    /* [string][in] */ FABRIC_URI name,
    /* [in] */ FABRIC_PUT_CUSTOM_PROPERTY_OPERATION const * propertyOperation,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [out, retval] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (propertyOperation == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(propertyOperation);

    ComPointer<PutCustomPropertyAsyncOperation> operation = make_com<PutCustomPropertyAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        propertyOperation,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndPutCustomPropertyOperation(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(PutCustomPropertyAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginCreateServiceGroup(
    /* [in] */ const FABRIC_SERVICE_GROUP_DESCRIPTION *description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || description == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(description);

    Naming::ServiceGroupServiceDescription serviceDescription;
    HRESULT hr = serviceDescription.FromServiceGroupDescription(*description);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    ComPointer<CreateServiceGroupAsyncOperation> operation = make_com<CreateServiceGroupAsyncOperation>(*this);

    hr = operation->Initialize(
        serviceDescription.GetRawPointer(),
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateServiceGroup(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateServiceGroupAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginCreateServiceGroupFromTemplate(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ LPCWSTR serviceTypeName,
    /* [in] */ ULONG InitializationDataSize,
    /* [size_is][in] */ BYTE *InitializationData,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (applicationName == NULL) || (serviceName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateServiceGroupFromTemplateAsyncOperation> operation = make_com<CreateServiceGroupFromTemplateAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        applicationName,
        serviceName,
        serviceTypeName,
        InitializationDataSize,
        InitializationData,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateServiceGroupFromTemplate(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateServiceGroupFromTemplateAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginCreateServiceGroupFromTemplate2(
    /* [in] */ const FABRIC_SERVICE_GROUP_FROM_TEMPLATE_DESCRIPTION *serviceGroupFromTemplateDesc,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) ||
        (serviceGroupFromTemplateDesc == NULL) ||
        (serviceGroupFromTemplateDesc->ApplicationName == NULL) ||
        (serviceGroupFromTemplateDesc->ServiceName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateServiceGroupFromTemplateAsyncOperation> operation = make_com<CreateServiceGroupFromTemplateAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(serviceGroupFromTemplateDesc, timeoutMilliseconds, callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateServiceGroupFromTemplate2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateServiceGroupFromTemplateAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginDeleteServiceGroup(
    /* [in] */ FABRIC_URI name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<DeleteServiceGroupAsyncOperation> operation = make_com<DeleteServiceGroupAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndDeleteServiceGroup(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeleteServiceGroupAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetServiceGroupDescription(
    /* [in] */ FABRIC_URI name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || name == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetDescriptionAsyncOperation> operation = make_com<GetDescriptionAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceGroupDescription(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricServiceGroupDescriptionResult **result)
{
    if (context == NULL || result == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    return ComUtility::OnPublicApiReturn(GetDescriptionAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginUpdateServiceGroup(
    /* [in] */ FABRIC_URI name,
    /* [in] */ const FABRIC_SERVICE_GROUP_UPDATE_DESCRIPTION *serviceGroupUpdateDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (serviceGroupUpdateDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(serviceGroupUpdateDescription);

    ComPointer<UpdateServiceGroupAsyncOperation> operation = make_com<UpdateServiceGroupAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        serviceGroupUpdateDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));

}

HRESULT ComFabricClient::EndUpdateServiceGroup(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpdateServiceGroupAsyncOperation::End(context));
}

//
// IFabricServiceManagementClient methods
//

HRESULT ComFabricClient::BeginCreateService(
    /* [in] */ const FABRIC_SERVICE_DESCRIPTION *description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (description == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateServiceOperation> operation = make_com<CreateServiceOperation>(*this);

    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateService(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateServiceOperation::End(context));
}

HRESULT ComFabricClient::BeginCreateServiceFromTemplate(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ LPCWSTR serviceTypeName,
    /* [in] */ ULONG InitializationDataSize,
    /* [size_is][in] */ BYTE *InitializationData,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (applicationName == NULL) || (serviceName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateServiceFromTemplateOperation> operation = make_com<CreateServiceFromTemplateOperation>(*this);

    HRESULT hr = operation->Initialize(
        applicationName,
        serviceName,
        serviceTypeName,
        InitializationDataSize,
        InitializationData,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateServiceFromTemplate(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateServiceFromTemplateOperation::End(context));
}

HRESULT ComFabricClient::BeginCreateServiceFromTemplate2(
    /* [in] */ const FABRIC_SERVICE_FROM_TEMPLATE_DESCRIPTION *serviceFromTemplateDesc,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) ||
        (serviceFromTemplateDesc == NULL) ||
        (serviceFromTemplateDesc->ApplicationName == NULL) ||
        (serviceFromTemplateDesc->ServiceName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateServiceFromTemplateOperation> operation = make_com<CreateServiceFromTemplateOperation>(*this);

    HRESULT hr = operation->Initialize(serviceFromTemplateDesc, timeoutMilliseconds, callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateServiceFromTemplate2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateServiceFromTemplateOperation::End(context));
}

HRESULT ComFabricClient::BeginUpdateService(
    /* [in] */ LPCWSTR serviceName,
    /* [in] */ const FABRIC_SERVICE_UPDATE_DESCRIPTION *description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (serviceName == NULL) || (description == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ServiceUpdateDescription updateDescription;
    auto error = updateDescription.FromPublicApi(*description);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    ComPointer<UpdateServiceOperation> operation = make_com<UpdateServiceOperation>(*this);

    HRESULT hr = operation->Initialize(
        serviceName,
        move(updateDescription),
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUpdateService(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpdateServiceOperation::End(context));
}

HRESULT ComFabricClient::BeginDeleteService(
    /* [in] */ FABRIC_URI name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<DeleteServiceOperation> operation = make_com<DeleteServiceOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndDeleteService(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeleteServiceOperation::End(context));
}


HRESULT ComFabricClient::BeginDeleteService2(
    /* [in] */ const FABRIC_DELETE_SERVICE_DESCRIPTION *deleteDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [out, retval] */ IFabricAsyncOperationContext **context)
{
    return DeleteServiceOperation::BeginOperation(
        *this,
        deleteDescription,
        timeoutMilliseconds,
        callback,
        context);
}

HRESULT ComFabricClient::EndDeleteService2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeleteServiceOperation::End(context));
}

HRESULT ComFabricClient::BeginGetServiceDescription(
    /* [in] */ FABRIC_URI name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || name == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetServiceDescriptionOperation> operation = make_com<GetServiceDescriptionOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        false, // Do not fetch from cache.
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));

}

HRESULT ComFabricClient::EndGetServiceDescription(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricServiceDescriptionResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceDescriptionOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetCachedServiceDescription(
    /* [in] */ FABRIC_URI name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL || name == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetServiceDescriptionOperation> operation = make_com<GetServiceDescriptionOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        true, // fetch from cache.
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));

}

HRESULT ComFabricClient::EndGetCachedServiceDescription(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricServiceDescriptionResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceDescriptionOperation::End(context, result));
}

HRESULT ComFabricClient::RegisterServicePartitionResolutionChangeHandler(
    /* [in] */ FABRIC_URI name,
    /* [in] */ FABRIC_PARTITION_KEY_TYPE keyType,
    /* [in] */ const void *partitionKey,
    /* [in] */ IFabricServicePartitionResolutionChangeHandler *callback,
    /* [retval][out] */ LONGLONG *callbackHandle)
{
    if ((name == NULL) || (callback == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    NamingUri uri;
    HRESULT hr = NamingUri::TryParse(name, Constants::FabricClientTrace, uri);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    Naming::PartitionKey key;
    auto error = key.FromPublicApi(keyType, partitionKey);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    ComPointer<IFabricServicePartitionResolutionChangeHandler> comCallback;
    comCallback.SetAndAddRef(callback);

    LocationChangeCallbackAdapterSPtr callbackAdapter = make_shared<LocationChangeCallbackAdapter>(
        *this,
        move(uri),
        move(key),
        move(comCallback));

    error = callbackAdapter->Register(callbackHandle);
    if (error.IsSuccess())
    {
        AcquireWriteLock lock(serviceLocationChangeTrackerLock_);
        serviceLocationChangeHandlers_.insert(make_pair(*callbackHandle, callbackAdapter));
    }

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComFabricClient::UnregisterServicePartitionResolutionChangeHandler(
    /* [in] */ LONGLONG callbackHandle)
{
    LocationChangeCallbackAdapterSPtr adapter;
    ErrorCode error = ErrorCodeValue::NotFound;
    {
        AcquireWriteLock lock(serviceLocationChangeTrackerLock_);
        if (serviceLocationChangeHandlers_.find(callbackHandle) != serviceLocationChangeHandlers_.end())
        {
            adapter = move(serviceLocationChangeHandlers_[callbackHandle]);
            serviceLocationChangeHandlers_.erase(callbackHandle);
        }
    }

    if (adapter)
    {
        error = adapter->Unregister();
    }

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComFabricClient::BeginResolveServicePartition(
    /* [in] */ FABRIC_URI name,
    /* [in] */ FABRIC_PARTITION_KEY_TYPE partitionKeyType,
    /* [in] */ const void *partitionKey,
    /* [in] */ IFabricResolvedServicePartitionResult *previousResult,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ComResolvedServicePartitionResult> previous;
    if (previousResult != NULL)
    {
        auto previousPtr = reinterpret_cast<ComResolvedServicePartitionResult const *>(previousResult);
        if (previousPtr == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_INVALIDARG);
        }

        previous.SetAndAddRef(const_cast<ComResolvedServicePartitionResult*>(previousPtr));
    }

    ComPointer<ResolveServiceOperation> operation = make_com<ResolveServiceOperation>(*this);

    HRESULT hr = operation->Initialize(
        name,
        partitionKeyType,
        partitionKey,
        move(previous),
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndResolveServicePartition(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricResolvedServicePartitionResult **result)
{
    return ComUtility::OnPublicApiReturn(ResolveServiceOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetServiceManifest(
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR applicationTypeVersion,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (applicationTypeName == NULL) || (applicationTypeVersion == NULL) || (serviceManifestName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetServiceManifestOperation> operation = make_com<GetServiceManifestOperation>(*this);

    HRESULT hr = operation->Initialize(
        applicationTypeName,
        applicationTypeVersion,
        serviceManifestName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceManifest(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceManifestOperation::End(context, result));
}

HRESULT ComFabricClient::BeginRemoveReplica(
    /* [in] */ const FABRIC_REMOVE_REPLICA_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    return ReportFaultAsyncOperation::BeginOperation(
        *this,
        description,
        FABRIC_FAULT_TYPE_PERMANENT,
        timeoutMilliseconds,
        callback,
        context);
}

HRESULT ComFabricClient::EndRemoveReplica(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(ReportFaultAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginRestartReplica(
    /* [in] */ const FABRIC_RESTART_REPLICA_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    return ReportFaultAsyncOperation::BeginOperation(
        *this,
        description,
        FABRIC_FAULT_TYPE_TRANSIENT,
        timeoutMilliseconds,
        callback,
        context);
}

HRESULT ComFabricClient::EndRestartReplica(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(ReportFaultAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginRegisterServiceNotificationFilter(
     /* [in] */ const FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION * description,
     /* [in] */ DWORD timeoutMilliseconds,
     /* [in] */ IFabricAsyncOperationCallback * callback,
     /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    return ComUtility::OnPublicApiReturn(RegisterServiceNotificationFilterAsyncOperation::Begin(
        *this,
        description,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndRegisterServiceNotificationFilter(
     /* [in] */ IFabricAsyncOperationContext * context,
     /* [out, retval] */ LONGLONG * filterId)
{
    return ComUtility::OnPublicApiReturn(RegisterServiceNotificationFilterAsyncOperation::End(
        context,
        filterId));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginUnregisterServiceNotificationFilter(
     /* [in] */ LONGLONG filterId,
     /* [in] */ DWORD timeoutMilliseconds,
     /* [in] */ IFabricAsyncOperationCallback * callback,
     /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    return ComUtility::OnPublicApiReturn(UnregisterServiceNotificationFilterAsyncOperation::Begin(
        *this,
        filterId,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndUnregisterServiceNotificationFilter(
     /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(UnregisterServiceNotificationFilterAsyncOperation::End(
        context));
}

HRESULT ComFabricClient::BeginProvisionApplicationType(
    /* [in] */ LPCWSTR applicationBuildPath,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (applicationBuildPath == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION provisionDescription = { 0 };
    provisionDescription.BuildPath = applicationBuildPath;
    return BeginProvisionApplicationType2(&provisionDescription, timeoutMilliseconds, callback, context);
}

HRESULT ComFabricClient::EndProvisionApplicationType(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ProvisionApplicationTypeOperation::End(context));
}

HRESULT ComFabricClient::BeginProvisionApplicationType2(
    /* [in] */ const FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (description == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ProvisionApplicationTypeOperation> operation = make_com<ProvisionApplicationTypeOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndProvisionApplicationType2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ProvisionApplicationTypeOperation::End(context));
}


HRESULT ComFabricClient::BeginProvisionApplicationType3(
    /* [in] */ const FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (description == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ProvisionApplicationTypeOperation> operation = make_com<ProvisionApplicationTypeOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndProvisionApplicationType3(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ProvisionApplicationTypeOperation::End(context));
}

HRESULT ComFabricClient::BeginUnprovisionApplicationType(
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR applicationTypeVersion,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION description = { 0 };

    description.ApplicationTypeName = applicationTypeName;
    description.ApplicationTypeVersion = applicationTypeVersion;
    description.Async = FALSE;

    return this->BeginUnprovisionApplicationType2(
        &description,
        timeoutMilliseconds,
        callback,
        context);
}

HRESULT ComFabricClient::EndUnprovisionApplicationType(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UnprovisionApplicationTypeOperation::End(context));
}

HRESULT ComFabricClient::BeginUnprovisionApplicationType2(
    /* [in] */ const FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (description == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<UnprovisionApplicationTypeOperation>(*this);

    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUnprovisionApplicationType2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UnprovisionApplicationTypeOperation::End(context));
}

HRESULT ComFabricClient::BeginUpdateApplicationUpgrade(
    /* [in] */ const FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION *updateDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (updateDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(updateDescription);

    auto operation = make_com<UpdateApplicationUpgradeOperation>(*this);

    HRESULT hr = operation->Initialize(
        updateDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUpdateApplicationUpgrade(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpdateApplicationUpgradeOperation::End(context));
}

HRESULT ComFabricClient::BeginRollbackApplicationUpgrade(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComUtility::OnPublicApiReturn(RollbackApplicationUpgradeOperation::Begin(
        *this,
        applicationName,
        timeoutMilliseconds,
        callback,
        context));
}

HRESULT ComFabricClient::EndRollbackApplicationUpgrade(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RollbackApplicationUpgradeOperation::End(context));
}

HRESULT ComFabricClient::CopyApplicationPackage(
    /* [in] */ LPCWSTR imageStoreConnectionString,
    /* [in] */ LPCWSTR applicationPackagePath,
    /* [in] */ LPCWSTR applicationPackagePathInImageStore)
{
    if ((imageStoreConnectionString == NULL) || (applicationPackagePath == NULL) || (applicationPackagePathInImageStore == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ErrorCode error = imageStoreClient_->Upload(
        imageStoreConnectionString,
        applicationPackagePath,
        applicationPackagePathInImageStore,
        TRUE);

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComFabricClient::RemoveApplicationPackage(
    /* [in] */ LPCWSTR imageStoreConnectionString,
    /* [in] */ LPCWSTR applicationPackagePathInImageStore)
{
    if ((imageStoreConnectionString == NULL) || (applicationPackagePathInImageStore == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ErrorCode error = imageStoreClient_->Delete(
        imageStoreConnectionString,
        applicationPackagePathInImageStore);

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComFabricClient::BeginCreateApplication(
    /* [in] */ const FABRIC_APPLICATION_DESCRIPTION *description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) ||
        (description == NULL) ||
        (description->ApplicationName == NULL) ||
        (description->ApplicationTypeName == NULL) ||
        (description->ApplicationTypeVersion == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<CreateApplicationOperation>(*this);

    ApplicationDescriptionWrapper appDesc;
    auto error = appDesc.FromPublicApi(*description);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::InitializeAndComplete(
            error,
            move(operation),
            callback,
            context));
    }

    HRESULT hr = operation->Initialize(
        appDesc,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateApplication(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CreateApplicationOperation::End(context));
}

HRESULT ComFabricClient::BeginUpdateApplication(
    /* [in] */ const FABRIC_APPLICATION_UPDATE_DESCRIPTION * applicationUpdateDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if ((applicationUpdateDescription == NULL) ||
        (applicationUpdateDescription->ApplicationName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ApplicationUpdateDescriptionWrapper appUpdateDesc;
    auto error = appUpdateDesc.FromPublicApi(*applicationUpdateDescription);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<UpdateApplicationOperation> operation = make_com<UpdateApplicationOperation>(*this);

    HRESULT hr = operation->Initialize(appUpdateDesc, timeoutMilliseconds, callback);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUpdateApplication(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(UpdateApplicationOperation::End(context));
}


HRESULT ComFabricClient::BeginUpgradeApplication(
    /* [in] */ const FABRIC_APPLICATION_UPGRADE_DESCRIPTION *upgradeDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (upgradeDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(upgradeDescription);

    auto operation = make_com<UpgradeApplicationOperation>(*this);

    ApplicationUpgradeDescriptionWrapper appUpgradeDesc;
    auto error = appUpgradeDesc.FromPublicApi(*upgradeDescription);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::InitializeAndComplete(
            error,
            move(operation),
            callback,
            context));
    }

    HRESULT hr = operation->Initialize(
        appUpgradeDesc,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUpgradeApplication(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpgradeApplicationOperation::End(context));
}

HRESULT ComFabricClient::BeginDeleteApplication(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) ||
        (applicationName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<DeleteApplicationOperation> operation = make_com<DeleteApplicationOperation>(*this);

    HRESULT hr = operation->Initialize(
        applicationName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndDeleteApplication(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeleteApplicationOperation::End(context));
}

HRESULT ComFabricClient::BeginDeleteApplication2(
    /* [in */ const FABRIC_DELETE_APPLICATION_DESCRIPTION *deleteDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return DeleteApplicationOperation::BeginOperation(
        *this,
        deleteDescription,
        timeoutMilliseconds,
        callback,
        context);
}

HRESULT ComFabricClient::EndDeleteApplication2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeleteApplicationOperation::End(context));
}

HRESULT ComFabricClient::BeginGetApplicationUpgradeProgress(
    /* [in] */ FABRIC_URI applicationName,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (applicationName == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetApplicationUpgradeProgressOperation> operation = make_com<GetApplicationUpgradeProgressOperation>(*this);

    HRESULT hr = operation->Initialize(
        applicationName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationUpgradeProgress(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricApplicationUpgradeProgressResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationUpgradeProgressOperation::End(
        context,
        IID_IFabricApplicationUpgradeProgressResult2,
        reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginMoveNextApplicationUpgradeDomain(
    /* [in] */ IFabricApplicationUpgradeProgressResult2 *upgradeProgress,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (upgradeProgress == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<MoveNextUpgradeDomainOperation> operation = make_com<MoveNextUpgradeDomainOperation>(*this);

    HRESULT hr = operation->Initialize(
        upgradeProgress,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndMoveNextApplicationUpgradeDomain(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(MoveNextUpgradeDomainOperation::End(context));
}

HRESULT ComFabricClient::BeginMoveNextApplicationUpgradeDomain2(
    /* [in] */ FABRIC_URI appName,
    /* [in] */ LPCWSTR nextUpgradeDomain,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<MoveNextUpgradeDomainOperation2> operation = make_com<MoveNextUpgradeDomainOperation2>(*this);

    HRESULT hr = operation->Initialize(
        appName,
        nextUpgradeDomain,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndMoveNextApplicationUpgradeDomain2(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(MoveNextUpgradeDomainOperation2::End(context));
}

HRESULT ComFabricClient::BeginGetApplicationManifest(
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR applicationTypeVersion,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == NULL) || (applicationTypeName == NULL) || (applicationTypeVersion == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetApplicationManifestOperation> operation = make_com<GetApplicationManifestOperation>(*this);

    HRESULT hr = operation->Initialize(
        applicationTypeName,
        applicationTypeVersion,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationManifest(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationManifestOperation::End(context, result));
}

HRESULT ComFabricClient::BeginRunCommand(
    /* [in] */ const LPCWSTR command,
    /* [in] */ FABRIC_PARTITION_ID targetPartitionId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RunCommandAsyncOperation> operation = make_com<RunCommandAsyncOperation>(*this, Guid(targetPartitionId));

    HRESULT hr = operation->Initialize(
        command,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndRunCommand(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(RunCommandAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginInvokeInfrastructureCommand(
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ const LPCWSTR command,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<InvokeInfrastructureCommandAsyncOperation> operation =
        make_com<InvokeInfrastructureCommandAsyncOperation>(*this, true);

    HRESULT hr = operation->Initialize(
        serviceName,
        command,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndInvokeInfrastructureCommand(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricStringResult ** result)
{
    return ComUtility::OnPublicApiReturn(InvokeInfrastructureCommandAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginInvokeInfrastructureQuery(
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ const LPCWSTR command,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<InvokeInfrastructureCommandAsyncOperation> operation =
        make_com<InvokeInfrastructureCommandAsyncOperation>(*this, false);

    HRESULT hr = operation->Initialize(
        serviceName,
        command,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndInvokeInfrastructureQuery(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricStringResult ** result)
{
    return ComUtility::OnPublicApiReturn(InvokeInfrastructureCommandAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginReportStartTaskSuccess(
    /* [in] */ const LPCWSTR taskId,
    /* [in] */ ULONGLONG instanceId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ReportTaskStartAsyncOperation> operation = make_com<ReportTaskStartAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        taskId,
        instanceId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndReportStartTaskSuccess(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ReportTaskStartAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginReportFinishTaskSuccess(
    /* [in] */ const LPCWSTR taskId,
    /* [in] */ ULONGLONG instanceId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ReportTaskFinishAsyncOperation> operation = make_com<ReportTaskFinishAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        taskId,
        instanceId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndReportFinishTaskSuccess(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ReportTaskFinishAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginReportTaskFailure(
    /* [in] */ const LPCWSTR taskId,
    /* [in] */ ULONGLONG instanceId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ReportTaskFailureAsyncOperation> operation = make_com<ReportTaskFailureAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        taskId,
        instanceId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndReportTaskFailure(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ReportTaskFailureAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginStartPartitionDataLoss(
    /* [in] */ const FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION * invokeDataLossDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (invokeDataLossDescription == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<InvokeDataLossAsyncOperation> operation =
        make_com<InvokeDataLossAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        invokeDataLossDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndStartPartitionDataLoss(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(InvokeDataLossAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetPartitionDataLossProgress(
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<GetInvokeDataLossProgressAsyncOperation> operation =
        make_com<GetInvokeDataLossProgressAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        operationId,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetPartitionDataLossProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionDataLossProgressResult **result)
{
    return ComUtility::OnPublicApiReturn(
        GetInvokeDataLossProgressAsyncOperation::End(
        context,
        IID_IFabricPartitionDataLossProgressResult,
        reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginStartPartitionQuorumLoss(
    /* [in] */ const FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION * invokeQuorumLossDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (invokeQuorumLossDescription == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<InvokeQuorumLossAsyncOperation> operation =
        make_com<InvokeQuorumLossAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        invokeQuorumLossDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndStartPartitionQuorumLoss(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(InvokeQuorumLossAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetPartitionQuorumLossProgress(
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<GetInvokeQuorumLossProgressAsyncOperation> operation =
        make_com<GetInvokeQuorumLossProgressAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        operationId,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetPartitionQuorumLossProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionQuorumLossProgressResult **result)
{
    return ComUtility::OnPublicApiReturn(GetInvokeQuorumLossProgressAsyncOperation::End(context, IID_IFabricPartitionQuorumLossProgressResult, reinterpret_cast<void**>(result)));
}

// Restart Partition
HRESULT ComFabricClient::BeginStartPartitionRestart(
    /* [in] */ const FABRIC_START_PARTITION_RESTART_DESCRIPTION * restartPartitionDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (restartPartitionDescription == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RestartPartitionAsyncOperation> operation =
        make_com<RestartPartitionAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        restartPartitionDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndStartPartitionRestart(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(RestartPartitionAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetPartitionRestartProgress(
    /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<GetRestartPartitionProgressAsyncOperation> operation =
        make_com<GetRestartPartitionProgressAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        operationId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetPartitionRestartProgress(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricPartitionRestartProgressResult **result)
{
    return ComUtility::OnPublicApiReturn(GetRestartPartitionProgressAsyncOperation::End(context, IID_IFabricPartitionRestartProgressResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginGetTestCommandStatusList(
    /* [in] */ const FABRIC_TEST_COMMAND_LIST_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetTestCommandStatusListOperation> operation = make_com<GetTestCommandStatusListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetTestCommandStatusList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricTestCommandStatusResult**result)
{
    return ComUtility::OnPublicApiReturn(GetTestCommandStatusListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginCancelTestCommand(
    /* [in] */ const FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CancelTestCommandAsyncOperation> operation =
        make_com<CancelTestCommandAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCancelTestCommand(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(CancelTestCommandAsyncOperation::End(context));
}

//
// IFabricTestManagementClient3 methods
//

HRESULT ComFabricClient::BeginStartNodeTransition(
    /* [in] */ const FABRIC_NODE_TRANSITION_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    Trace.WriteInfo(
        TraceComponent,
        "Enter ComFabricClient::BeginStartNodeTransition");

    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<StartNodeTransitionAsyncOperation> operation =
        make_com<StartNodeTransitionAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndStartNodeTransition(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(StartNodeTransitionAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetNodeTransitionProgress(
    /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<GetNodeTransitionProgressAsyncOperation> operation =
        make_com<GetNodeTransitionProgressAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        operationId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetNodeTransitionProgress(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricNodeTransitionProgressResult **result)
{
    Trace.WriteInfo(
        TraceComponent,
        "ComFabricClient::EndGetNodeTransitionProgress"
        );

    return ComUtility::OnPublicApiReturn(GetNodeTransitionProgressAsyncOperation::End(context, IID_IFabricNodeTransitionProgressResult, reinterpret_cast<void**>(result)));
}


// Chaos

HRESULT ComFabricClient::BeginStartChaos(
    /* [in] */ const FABRIC_START_CHAOS_DESCRIPTION * startChaosDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (startChaosDescription == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<StartChaosAsyncOperation> operation =
    make_com<StartChaosAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        startChaosDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndStartChaos(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(StartChaosAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginStopChaos(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    Trace.WriteInfo(
        TraceComponent,
        "Enter ComFabricClient::BeginStopChaos");

    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<StopChaosAsyncOperation> operation =
        make_com<StopChaosAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
    timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndStopChaos(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(StopChaosAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetChaosReport(
    /* [in] */ const FABRIC_GET_CHAOS_REPORT_DESCRIPTION * getChaosReportDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(
        TraceComponent,
        "Enter ComFabricClient::BeginGetChaosReport");

    if (getChaosReportDescription == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetChaosReportAsyncOperation> operation = make_com<GetChaosReportAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        getChaosReportDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetChaosReport(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricChaosReportResult **result)
{
    return ComUtility::OnPublicApiReturn(GetChaosReportAsyncOperation::End(context, IID_IFabricChaosReportResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginGetChaos(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(
        TraceComponent,
        "Enter ComFabricClient::BeginGetChaos");

    ComPointer<GetChaosAsyncOperation> operation = make_com<GetChaosAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(timeoutMilliseconds, callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetChaos(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricChaosDescriptionResult **result)
{
    return ComUtility::OnPublicApiReturn(
        GetChaosAsyncOperation::End(
            context,
            IID_IFabricChaosDescriptionResult,
            reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginGetChaosSchedule(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval, out] */ IFabricAsyncOperationContext ** context)
{
    Trace.WriteInfo(
        TraceComponent,
        "Enter ComFabricClient::BeginGetChaosSchedule");

    ComPointer<GetChaosScheduleAsyncOperation> operation = make_com<GetChaosScheduleAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetChaosSchedule(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval, out] */ IFabricChaosScheduleDescriptionResult **result)
{
    return ComUtility::OnPublicApiReturn(
        GetChaosScheduleAsyncOperation::End(
            context,
            IID_IFabricChaosScheduleDescriptionResult,
            reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginSetChaosSchedule(
    /* [in] */ const FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION* setChaosScheduleDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval, out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(
        TraceComponent,
        "Enter ComFabricClient::BeginSetChaosSchedule");

    if (setChaosScheduleDescription == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<SetChaosScheduleAsyncOperation> operation = make_com<SetChaosScheduleAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        setChaosScheduleDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndSetChaosSchedule(
    /* [in] */ IFabricAsyncOperationContext *context)
{

    return ComUtility::OnPublicApiReturn(
        SetChaosScheduleAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginGetChaosEvents(
    /* [in] */ const FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION * chaosEventsDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(
        TraceComponent,
        "Enter ComFabricClient::BeginGetChaosEvents");

    if (chaosEventsDescription == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetChaosEventsAsyncOperation> operation =
        make_com<GetChaosEventsAsyncOperation>(*this);

    wstring errorMessage;
    HRESULT hr = operation->Initialize(
        chaosEventsDescription,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetChaosEvents(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricChaosEventsSegmentResult **result)
{
    return ComUtility::OnPublicApiReturn(
        GetChaosEventsAsyncOperation::End(
            context,
            IID_IFabricChaosEventsSegmentResult,
            reinterpret_cast<void**>(result)));
}

///
/// IFabricFaultManagementClient
///

HRESULT ComFabricClient::BeginRestartNode(
    /*[in]*/ const FABRIC_RESTART_NODE_DESCRIPTION2 * description,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    switch (description->Kind)
    {
    case FABRIC_RESTART_NODE_DESCRIPTION_KIND_USING_NODE_NAME:
    {
        auto body = reinterpret_cast<FABRIC_RESTART_NODE_DESCRIPTION_USING_NODE_NAME *>(description->Value);
        if (body == NULL){ return E_POINTER; }

        ComPointer<RestartNodeAsyncOperation> operation = make_com<RestartNodeAsyncOperation>(*this);
        HRESULT hr = operation->Initialize(
            body->NodeName,
            body->NodeInstanceId,
            (body->ShouldCreateFabricDump != 0),
            timeoutMilliseconds,
            callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
    }
    default:
        return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }
}

HRESULT  ComFabricClient::EndRestartNode(
    /*[in]*/ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricRestartNodeResult **result)
{
    return ComUtility::OnPublicApiReturn(RestartNodeAsyncOperation::End(context, IID_IFabricRestartNodeResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginStartNode(
    /*[in]*/ const FABRIC_START_NODE_DESCRIPTION2 * description,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    switch (description->Kind)
    {
        case FABRIC_START_NODE_DESCRIPTION_KIND_USING_NODE_NAME:
        {
            auto body = reinterpret_cast<FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME *>(description->Value);
            if (body == NULL){ return E_POINTER; }

            ComPointer<StartNodeAsyncOperation> operation = make_com<StartNodeAsyncOperation>(*this);
            HRESULT hr = operation->Initialize(
                body,
                timeoutMilliseconds,
                callback);
            if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

            return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
        }
        default:
            return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }
}

HRESULT  ComFabricClient::EndStartNode(
    /*[in]*/ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricStartNodeResult **result)
{
    return ComUtility::OnPublicApiReturn(StartNodeAsyncOperation::End(context, IID_IFabricStartNodeResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginStopNode(
    /*[in]*/ const FABRIC_STOP_NODE_DESCRIPTION2 * description,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    switch (description->Kind)
    {
        case FABRIC_STOP_NODE_DESCRIPTION_KIND_USING_NODE_NAME:
        {
            auto body = reinterpret_cast<FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME *>(description->Value);
            if (body == NULL){ return E_POINTER; }

            ComPointer<StopNodeAsyncOperation> operation = make_com<StopNodeAsyncOperation>(*this);
            HRESULT hr = operation->Initialize(
                body,
                timeoutMilliseconds,
                callback);
            if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

            return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
        }
        default:
            return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }
}

HRESULT  ComFabricClient::EndStopNode(
    /*[in]*/ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricStopNodeResult **result)
{
    return ComUtility::OnPublicApiReturn(StopNodeAsyncOperation::End(context, IID_IFabricStopNodeResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginStopNodeInternal(
    /*[in]*/ const FABRIC_STOP_NODE_DESCRIPTION_INTERNAL * description,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    Trace.WriteInfo(
        TraceComponent,
        "ComFabricClient::BeginStopNodeInternal()");

    ComPointer<StopNodeInternalAsyncOperation> operation =
        make_com<StopNodeInternalAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));

}

HRESULT  ComFabricClient::EndStopNodeInternal(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    Trace.WriteInfo(
        TraceComponent,
        "ComFabricClient::EndStopNodeInternal()");

    return ComUtility::OnPublicApiReturn(StopNodeInternalAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginRestartDeployedCodePackage(
    /*[in]*/ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION2 * description,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    switch (description->Kind)
    {
        case FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_KIND_USING_NODE_NAME:
        {
            auto body = reinterpret_cast<FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME *>(description->Value);
            if (body == NULL){ return E_POINTER; }

            ComPointer<RestartDeployedCodePackageAsyncOperation> operation = make_com<RestartDeployedCodePackageAsyncOperation>(*this);
            HRESULT hr = operation->Initialize(
                body,
                timeoutMilliseconds,
                callback);
            if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

            return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
        }
        default:
            return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }
}

HRESULT  ComFabricClient::EndRestartDeployedCodePackage(
    /*[in]*/ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricRestartDeployedCodePackageResult **result)
{
    return ComUtility::OnPublicApiReturn(RestartDeployedCodePackageAsyncOperation::End(context, IID_IFabricRestartDeployedCodePackageResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginMovePrimary(
    /*[in]*/ const FABRIC_MOVE_PRIMARY_DESCRIPTION2 * description,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    switch (description->Kind)
    {
        case FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND_USING_NODE_NAME:
        {
            auto body = reinterpret_cast<FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME *>(description->Value);
            if (body == NULL){ return E_POINTER; }

            ComPointer<MovePrimaryAsyncOperation> operation = make_com<MovePrimaryAsyncOperation>(*this);
            HRESULT hr = operation->Initialize(
                body,
                timeoutMilliseconds,
                callback);
            if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

            return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
        }
        default:
            return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }
}

HRESULT  ComFabricClient::EndMovePrimary(
    /*[in]*/ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricMovePrimaryResult **result)
{
    return ComUtility::OnPublicApiReturn(MovePrimaryAsyncOperation::End(context, IID_IFabricMovePrimaryResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginMoveSecondary(
    /*[in]*/ const FABRIC_MOVE_SECONDARY_DESCRIPTION2 * description,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    switch (description->Kind)
    {
        case FABRIC_MOVE_SECONDARY_DESCRIPTION_KIND_USING_NODE_NAME:
        {
            auto body = reinterpret_cast<FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME *>(description->Value);
            if (body == NULL){ return E_POINTER; }

            ComPointer<MoveSecondaryAsyncOperation> operation = make_com<MoveSecondaryAsyncOperation>(*this);
            HRESULT hr = operation->Initialize(
                body,
                timeoutMilliseconds,
                callback);
            if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

            return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
        }
        default:
            return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }
}

HRESULT  ComFabricClient::EndMoveSecondary(
    /*[in]*/ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricMoveSecondaryResult **result)
{
    return ComUtility::OnPublicApiReturn(MoveSecondaryAsyncOperation::End(context, IID_IFabricMoveSecondaryResult, reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::GetNthNamingPartitionId(
    /* [in] */ ULONG n,
    /* [retval][out] */ FABRIC_PARTITION_ID * partitionId)
{
    if (partitionId == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    Reliability::ConsistencyUnitId cuid;
    auto error = testClient_->GetNthNamingPartitionId(n, cuid);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

    *partitionId = cuid.Guid.AsGUID();
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComFabricClient::BeginResolvePartition(
    /* [in] */ FABRIC_PARTITION_ID * partitionId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((partitionId == NULL) || (context == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ResolvePartitionAsyncOperation> operation = make_com<ResolvePartitionAsyncOperation>(*this);
    HRESULT hr = operation->Initialize(
        partitionId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndResolvePartition(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricResolvedServicePartitionResult ** result)
{
    return ComUtility::OnPublicApiReturn(ResolvePartitionAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginResolveNameOwner(
    /* [in] */ FABRIC_URI name,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((name == NULL) || (context == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ResolveNameOwnerAsyncOperation> operation = make_com<ResolveNameOwnerAsyncOperation>(*this);
    HRESULT hr = operation->Initialize(
        name,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndResolveNameOwner(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricResolvedServicePartitionResult ** result)
{
    return ComUtility::OnPublicApiReturn(ResolveNameOwnerAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::NodeIdFromNameOwnerAddress(
    /* [in] */ LPCWSTR address,
    _Out_writes_bytes_(sizeof(Federation::Nodeid)) void * federationNodeId)
{
    if ((address == NULL) || (federationNodeId == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    Federation::NodeId nodeId;
    auto error = testClient_->NodeIdFromNameOwnerAddress(address, nodeId);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

    memcpy_s(federationNodeId, sizeof(Federation::NodeId), &nodeId, sizeof(nodeId));
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComFabricClient::NodeIdFromFMAddress(
    /* [in] */ LPCWSTR address,
    _Out_writes_bytes_(sizeof(Federation::Nodeid)) void * federationNodeId)
{
    if ((address == NULL) || (federationNodeId == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    Federation::NodeId nodeId;
    auto error = testClient_->NodeIdFromFMAddress(address, nodeId);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

    memcpy_s(federationNodeId, sizeof(Federation::NodeId), &nodeId, sizeof(nodeId));
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComFabricClient::GetCurrentGatewayAddress(
    /* [retval][out] */ IFabricStringResult ** gatewayAddress)
{
    if (gatewayAddress == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    wstring address;
    auto error = testClient_->GetCurrentGatewayAddress(address);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

    Common::ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(address);
    *gatewayAddress = stringResult.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComFabricClient::BeginGetNodeList(
    /* [in] */ const FABRIC_NODE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetNodeListOperation> operation = make_com<GetNodeListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        false,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetNodeList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetNodeListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetNodeListOperation::End(context, result));
}

HRESULT ComFabricClient::EndGetNodeList2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetNodeListResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetNodeListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetNodeListInternal(
    /* [in] */ const FABRIC_NODE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ BOOLEAN excludeStoppedNodeInfo,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetNodeListOperation> operation = make_com<GetNodeListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        excludeStoppedNodeInfo,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}


HRESULT ComFabricClient::EndGetNodeList2Internal(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetNodeListResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetNodeListOperation::End(context, result));
}


HRESULT ComFabricClient::BeginGetApplicationTypeList(
    /* [in] */ const FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetApplicationTypeListOperation> operation = make_com<GetApplicationTypeListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationTypeList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetApplicationTypeListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationTypeListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetServiceTypeList(
    /* [in] */ const FABRIC_SERVICE_TYPE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetServiceTypeListOperation> operation = make_com<GetServiceTypeListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceTypeList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetServiceTypeListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceTypeListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetServiceGroupMemberTypeList(
    /* [in] */ const FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetServiceGroupMemberTypeListOperation> operation = make_com<GetServiceGroupMemberTypeListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceGroupMemberTypeList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetServiceGroupMemberTypeListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceGroupMemberTypeListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetApplicationList(
    /* [in] */ const FABRIC_APPLICATION_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetApplicationListOperation> operation = make_com<GetApplicationListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetApplicationListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationListOperation::End(context, result));
}

HRESULT ComFabricClient::EndGetApplicationList2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetApplicationListResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetServiceList(
    /* [in] */ const FABRIC_SERVICE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetServiceListOperation> operation = make_com<GetServiceListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetServiceListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceListOperation::End(context, result));
}

HRESULT ComFabricClient::EndGetServiceList2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetServiceListResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetServiceGroupMemberList(
    /* [in] */ const FABRIC_SERVICE_GROUP_MEMBER_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetServiceGroupMemberListOperation> operation = make_com<GetServiceGroupMemberListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceGroupMemberList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetServiceGroupMemberListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceGroupMemberListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetPartitionList(
    /* [in] */ const FABRIC_SERVICE_PARTITION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetPartitionListOperation> operation = make_com<GetPartitionListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetPartitionList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetPartitionListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetPartitionListOperation::End(context, result));
}

HRESULT ComFabricClient::EndGetPartitionList2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetPartitionListResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetPartitionListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetReplicaList(
    /* [in] */ const FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetReplicaListOperation> operation = make_com<GetReplicaListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetReplicaList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetReplicaListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetReplicaListOperation::End(context, result));
}

HRESULT ComFabricClient::EndGetReplicaList2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetReplicaListResult2 **result)
{
    return ComUtility::OnPublicApiReturn(GetReplicaListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedApplicationList(
    /* [in] */ const FABRIC_DEPLOYED_APPLICATION_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetDeployedApplicationListOperation> operation = make_com<GetDeployedApplicationListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedApplicationList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetDeployedApplicationListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedApplicationListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedServicePackageList(
    /* [in] */ const FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetDeployedServiceManifestListOperation> operation = make_com<GetDeployedServiceManifestListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedServicePackageList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetDeployedServicePackageListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedServiceManifestListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedServiceTypeList(
    /* [in] */ const FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetDeployedServiceTypeListOperation> operation = make_com<GetDeployedServiceTypeListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedServiceTypeList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetDeployedServiceTypeListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedServiceTypeListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedCodePackageList(
    /* [in] */ const FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetDeployedCodePackageListOperation> operation = make_com<GetDeployedCodePackageListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedCodePackageList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetDeployedCodePackageListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedCodePackageListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedReplicaList(
    /* [in] */ const FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetDeployedReplicaListOperation> operation = make_com<GetDeployedReplicaListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedReplicaList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetDeployedReplicaListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedReplicaListOperation::End(context, result));
}

HRESULT  ComFabricClient::BeginAddUnreliableTransportBehavior(
    /*[in]*/ LPCWSTR nodeName,
    /*[in]*/ LPCWSTR name,
    /*[in]*/ const FABRIC_UNRELIABLETRANSPORT_BEHAVIOR * unreliableTransportBehavior,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (unreliableTransportBehavior == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<AddUnreliableTransportBehaviorOperation> operation = make_com<AddUnreliableTransportBehaviorOperation>(*this);
    HRESULT hr = operation->Initialize(
        nodeName,
        name,
        *unreliableTransportBehavior,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClient::EndAddUnreliableTransportBehavior(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(AddUnreliableTransportBehaviorOperation::End(context));
}

HRESULT  ComFabricClient::BeginRemoveUnreliableTransportBehavior(
    /*[in]*/ LPCWSTR nodeName,
    /*[in]*/ LPCWSTR name,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    ComPointer<RemoveUnreliableTransportBehaviorOperation> operation = make_com<RemoveUnreliableTransportBehaviorOperation>(*this);
    HRESULT hr = operation->Initialize(
        nodeName,
        name,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClient::EndRemoveUnreliableTransportBehavior(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(RemoveUnreliableTransportBehaviorOperation::End(context));
}

HRESULT ComFabricClient::BeginGetTransportBehaviorList(
    /*[in]*/ LPCWSTR nodeName,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    ComPointer<TransportBehaviorListOperation> operation = make_com<TransportBehaviorListOperation>(*this);
    HRESULT hr = operation->Initialize(
        nodeName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetTransportBehaviorList(
    /*[in]*/ IFabricAsyncOperationContext * context,
    /*[out, retval]*/  IFabricStringListResult ** result)
{
    return ComUtility::OnPublicApiReturn(TransportBehaviorListOperation::End(context, result));
}

HRESULT  ComFabricClient::BeginAddUnreliableLeaseBehavior(
    /*[in]*/ LPCWSTR nodeName,
    /*[in]*/ LPCWSTR behaviorString,
    /*[in]*/ LPCWSTR alias,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    ComPointer<AddUnreliableLeaseBehaviorOperation> operation = make_com<AddUnreliableLeaseBehaviorOperation>(*this);
    HRESULT hr = operation->Initialize(
        nodeName,
        behaviorString,
        alias,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClient::EndAddUnreliableLeaseBehavior(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(AddUnreliableLeaseBehaviorOperation::End(context));
}

HRESULT  ComFabricClient::BeginRemoveUnreliableLeaseBehavior(
    /*[in]*/ LPCWSTR nodeName,
    /*[in]*/ LPCWSTR alias,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    ComPointer<RemoveUnreliableLeaseBehaviorOperation> operation = make_com<RemoveUnreliableLeaseBehaviorOperation>(*this);
    HRESULT hr = operation->Initialize(
        nodeName,
        alias,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClient::EndRemoveUnreliableLeaseBehavior(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(RemoveUnreliableLeaseBehaviorOperation::End(context));
}

HRESULT  ComFabricClient::BeginStopNode(
    /*[in]*/ const FABRIC_STOP_NODE_DESCRIPTION *,
    /*[in]*/ DWORD ,
    /*[in]*/ IFabricAsyncOperationCallback *,
    /*[out, retval]*/ IFabricAsyncOperationContext ** )
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::EndStopNode(
    /*[in]*/ IFabricAsyncOperationContext *)
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::BeginRestartNode(
    /*[in]*/ const FABRIC_RESTART_NODE_DESCRIPTION *,
    /*[in]*/ DWORD ,
    /*[in]*/ IFabricAsyncOperationCallback *,
    /*[out, retval]*/ IFabricAsyncOperationContext **)
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::EndRestartNode(
    /*[in]*/ IFabricAsyncOperationContext *)
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::BeginStartNode(
    /*[in]*/ const FABRIC_START_NODE_DESCRIPTION *,
    /*[in]*/ DWORD,
    /*[in]*/ IFabricAsyncOperationCallback *,
    /*[out, retval]*/ IFabricAsyncOperationContext **)
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::EndStartNode(
    /*[in]*/ IFabricAsyncOperationContext *)
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::BeginRestartNodeInternal(
    /*[in]*/ const FABRIC_RESTART_NODE_DESCRIPTION * restartNodeDescription,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (restartNodeDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    bool isCreateFabricDump = false;

    if (restartNodeDescription->Reserved != nullptr)
    {
        FABRIC_RESTART_NODE_DESCRIPTION_EX1 * casted = static_cast<FABRIC_RESTART_NODE_DESCRIPTION_EX1*>(restartNodeDescription->Reserved);
        isCreateFabricDump = casted->CreateFabricDump ? true : false;
    }

    ComPointer<StopNodeOperation> operation = make_com<StopNodeOperation>(*this);
    HRESULT hr = operation->Initialize(
        restartNodeDescription->NodeName,
        restartNodeDescription->NodeInstanceId,
        true/*RestartNode*/,
        isCreateFabricDump,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClient::EndRestartNodeInternal(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(StopNodeOperation::End(context));
}

HRESULT  ComFabricClient::BeginStartNodeInternal(
    /*[in]*/ const FABRIC_START_NODE_DESCRIPTION * startNodeDescription,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (startNodeDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<StartNodeOperation> operation = make_com<StartNodeOperation>(*this);
    HRESULT hr = operation->Initialize(
        startNodeDescription->NodeName,
        startNodeDescription->NodeInstanceId,
        startNodeDescription->IPAddressOrFQDN,
        startNodeDescription->ClusterConnectionPort,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClient::EndStartNodeInternal(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(StartNodeOperation::End(context));
}

HRESULT ComFabricClient::CopyClusterPackage(
    /* [in] */ LPCWSTR imageStoreConnectionString,
    /* [in] */ LPCWSTR clusterManifestPath,
    /* [in] */ LPCWSTR clusterManifestPathInImageStore,
    /* [in] */ LPCWSTR codePackagePath,
    /* [in] */ LPCWSTR codePackagePathInImageStore)
{
    if (imageStoreConnectionString == NULL || (clusterManifestPath == NULL && codePackagePath == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ErrorCode error;
    if(clusterManifestPath != NULL)
    {
        if(clusterManifestPathInImageStore == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        error = imageStoreClient_->Upload(
            imageStoreConnectionString,
            clusterManifestPath,
            clusterManifestPathInImageStore,
            true);
        if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }
    }

    if(codePackagePath != NULL)
    {
        if(codePackagePathInImageStore == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        error = imageStoreClient_->Upload(
            imageStoreConnectionString,
            codePackagePath,
            codePackagePathInImageStore,
            true);
        if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }
    }

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComFabricClient::RemoveClusterPackage(
    /* [in] */ LPCWSTR imageStoreConnectionString,
    /* [in] */ LPCWSTR clusterManifestPathInImageStore,
    /* [in] */ LPCWSTR codePackagePathInImageStore)
{
    if (imageStoreConnectionString == NULL || (clusterManifestPathInImageStore == NULL && codePackagePathInImageStore == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ErrorCode error;
    if(clusterManifestPathInImageStore != NULL)
    {
        error = imageStoreClient_->Delete(
            imageStoreConnectionString,
            clusterManifestPathInImageStore);
        if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }
    }

    if(codePackagePathInImageStore != NULL)
    {
        error = imageStoreClient_->Delete(
            imageStoreConnectionString,
            codePackagePathInImageStore);
        if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }
    }

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComFabricClient::BeginMovePrimary(
    /*[in]*/ const FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION * movePrimaryDescription,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context
    )
{
    if (movePrimaryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<MovePrimaryOperation> operation = make_com<MovePrimaryOperation>(*this);
    HRESULT hr = operation->Initialize(
        movePrimaryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)){ return ComUtility::OnPublicApiReturn(hr); }
    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndMovePrimary(/*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(MovePrimaryOperation::End(context));
}

HRESULT ComFabricClient::BeginMoveSecondary(
    /*[in]*/ const FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION * moveSecondaryDescription,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context
    )
{
    if (moveSecondaryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    ComPointer<MoveSecondaryOperation> operation = make_com<MoveSecondaryOperation>(*this);
    HRESULT hr = operation->Initialize(moveSecondaryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)){ return ComUtility::OnPublicApiReturn(hr); }
    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndMoveSecondary(/*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(MoveSecondaryOperation::End(context));
}

HRESULT  ComFabricClient::BeginRestartDeployedCodePackage(
    /*[in]*/ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION *,
    /*[in]*/ DWORD ,
    /*[in]*/ IFabricAsyncOperationCallback *,
    /*[out, retval]*/ IFabricAsyncOperationContext **)
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::EndRestartDeployedCodePackage(
    /*[in]*/ IFabricAsyncOperationContext *)
{
    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT  ComFabricClient::BeginRestartDeployedCodePackageInternal(
    /*[in]*/ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION * restartCodePackageDescription,
    /*[in]*/ DWORD timeoutMilliseconds,
    /*[in]*/ IFabricAsyncOperationCallback * callback,
    /*[out, retval]*/ IFabricAsyncOperationContext ** context)
{
    if (restartCodePackageDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (restartCodePackageDescription->NodeName == NULL ||
        restartCodePackageDescription->ApplicationName == NULL ||
        restartCodePackageDescription->ServiceManifestName == NULL ||
        restartCodePackageDescription->CodePackageName == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RestartDeployedCodePackageOperation> operation = make_com<RestartDeployedCodePackageOperation>(*this);
    HRESULT hr = operation->Initialize(
        restartCodePackageDescription->NodeName,
        restartCodePackageDescription->ApplicationName,
        restartCodePackageDescription->ServiceManifestName,
        restartCodePackageDescription->CodePackageName,
        restartCodePackageDescription->CodePackageInstanceId,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClient::EndRestartDeployedCodePackageInternal(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(RestartDeployedCodePackageOperation::End(context));
}

HRESULT ComFabricClient::BeginGetDeployedReplicaDetail(
    /* [in] */ const FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_DESCRIPTION* queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetDeployedServiceReplicaDetailOperation> operation = make_com<GetDeployedServiceReplicaDetailOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedReplicaDetail(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetDeployedServiceReplicaDetailResult **result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedServiceReplicaDetailOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetProvisionedFabricCodeVersionList(
    /* [in] */ const FABRIC_PROVISIONED_CODE_VERSION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetFabricCodeVersionListOperation> operation = make_com<GetFabricCodeVersionListOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetProvisionedFabricCodeVersionList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetProvisionedCodeVersionListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetFabricCodeVersionListOperation::End(context, result));
}


HRESULT ComFabricClient::BeginGetProvisionedFabricConfigVersionList(
    /* [in] */ const FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetFabricConfigVersionListOperation> operation = make_com<GetFabricConfigVersionListOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetProvisionedFabricConfigVersionList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetProvisionedConfigVersionListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetFabricConfigVersionListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginInternalQuery(
    /* [in] */ LPCWSTR queryName,
    /* [in] */ const FABRIC_STRING_PAIR_LIST * queryArgs,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (context == NULL || queryName == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    wstring queryNameString = queryName;
    QueryArgumentMap queryArgsMap;

    if (queryArgs && queryArgs->Count > 0)
    {
        if (queryArgs->Items == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        for (ULONG i = 0; i < queryArgs->Count; ++i)
        {
            if (queryArgs->Items[i].Name == NULL || queryArgs->Items[i].Value == NULL)
            {
                return ComUtility::OnPublicApiReturn(E_POINTER);
            }

            queryArgsMap.Insert(queryArgs->Items[i].Name, queryArgs->Items[i].Value);
        }
    }

    auto operation = make_com<QueryOperation>(*this);

    auto hr = operation->Initialize(
        queryNameString,
        queryArgsMap,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndInternalQuery(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IInternalFabricQueryResult ** result)
{
    return ComUtility::OnPublicApiReturn(QueryOperation::End(context, result));
}

HRESULT ComFabricClient::EndInternalQuery2(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IInternalFabricQueryResult2 ** result)
{
    return ComUtility::OnPublicApiReturn(QueryOperation::End(context, result));
}

HRESULT ComFabricClient::BeginCreateRepairTask(
    /* [in] */ const FABRIC_REPAIR_TASK * repairTask,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (repairTask == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateRepairRequestAsyncOperation> operation = make_com<CreateRepairRequestAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        repairTask,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateRepairTask(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion)
{
    return ComUtility::OnPublicApiReturn(CreateRepairRequestAsyncOperation::End(context, commitVersion));
}

HRESULT ComFabricClient::BeginCancelRepairTask(
    /* [in] */ const FABRIC_REPAIR_CANCEL_DESCRIPTION * requestDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (requestDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CancelRepairRequestAsyncOperation> operation = make_com<CancelRepairRequestAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        requestDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCancelRepairTask(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion)
{
    return ComUtility::OnPublicApiReturn(CancelRepairRequestAsyncOperation::End(context, commitVersion));
}

HRESULT ComFabricClient::BeginForceApproveRepairTask(
    /* [in] */ const FABRIC_REPAIR_APPROVE_DESCRIPTION * requestDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (requestDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<ForceApproveRepairAsyncOperation> operation = make_com<ForceApproveRepairAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        requestDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndForceApproveRepairTask(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion)
{
    return ComUtility::OnPublicApiReturn(ForceApproveRepairAsyncOperation::End(context, commitVersion));
}

HRESULT ComFabricClient::BeginDeleteRepairTask(
    /* [in] */ const FABRIC_REPAIR_DELETE_DESCRIPTION * requestDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (requestDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<DeleteRepairRequestAsyncOperation> operation = make_com<DeleteRepairRequestAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        requestDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndDeleteRepairTask(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(DeleteRepairRequestAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginUpdateRepairExecutionState(
    /* [in] */ const FABRIC_REPAIR_TASK * repairTask,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (repairTask == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<UpdateRepairExecutionStateAsyncOperation> operation = make_com<UpdateRepairExecutionStateAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        repairTask,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUpdateRepairExecutionState(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion)
{
    return ComUtility::OnPublicApiReturn(UpdateRepairExecutionStateAsyncOperation::End(context, commitVersion));
}

HRESULT ComFabricClient::BeginGetRepairTaskList(
    /* [in] */ const FABRIC_REPAIR_TASK_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetRepairTaskListAsyncOperation> operation = make_com<GetRepairTaskListAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetRepairTaskList(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricGetRepairTaskListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetRepairTaskListAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginUpdateRepairTaskHealthPolicy(
    /* [in] */ const FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION * requestDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (requestDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<UpdateRepairTaskHealthPolicyAsyncOperation> operation = make_com<UpdateRepairTaskHealthPolicyAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        requestDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUpdateRepairTaskHealthPolicy(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion)
{
    return ComUtility::OnPublicApiReturn(UpdateRepairTaskHealthPolicyAsyncOperation::End(context, commitVersion));
}

HRESULT ComFabricClient::BeginSetAcl(
    /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
    /* [in] */ const FABRIC_SECURITY_ACL *acl,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<SetAclOperation> operation = make_com<SetAclOperation>(*this);
    HRESULT hr = operation->Initialize(
        resource,
        acl,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndSetAcl(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(SetAclOperation::End(context));
}

HRESULT ComFabricClient::BeginGetAcl(
    /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<GetAclOperation> operation = make_com<GetAclOperation>(*this);
    HRESULT hr = operation->Initialize(
        resource,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(std::move(operation), context));
}

HRESULT ComFabricClient::EndGetAcl(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetAclResult **acl)
{
    return ComUtility::OnPublicApiReturn(GetAclOperation::End(context, acl));
}

HRESULT ComFabricClient::BeginGetClusterLoadInformation(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{

    ComPointer<GetClusterLoadInformationOperation> operation = make_com<GetClusterLoadInformationOperation>(*this);
    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetClusterLoadInformation(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetClusterLoadInformationResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterLoadInformationOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetPartitionLoadInformation(
    /* [in] */ const FABRIC_PARTITION_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetPartitionLoadInformationOperation> operation = make_com<GetPartitionLoadInformationOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetPartitionLoadInformation(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetPartitionLoadInformationResult **result)
{
    return ComUtility::OnPublicApiReturn(GetPartitionLoadInformationOperation::End(context, result));
}

HRESULT ComFabricClient::BeginDeployServicePackageToNode(
      /* [in] */ LPCWSTR applicationTypeName,
      /* [in] */ LPCWSTR applicationTypeVersion,
      /* [in] */ LPCWSTR serviceManifestName,
      /* [in] */ const FABRIC_PACKAGE_SHARING_POLICY_LIST * sharedPackages,
      /* [in] */ LPCWSTR nodeName,
      /* [in] */ DWORD timeoutMilliseconds,
      /* [in] */ IFabricAsyncOperationCallback * callback,
      /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    ComPointer<DeployServicePackageToNodeOperation> operation = make_com<DeployServicePackageToNodeOperation>(*this);
    HRESULT hr = operation->Initialize(
        applicationTypeName,
        applicationTypeVersion,
        serviceManifestName,
        sharedPackages,
        nodeName,
        timeoutMilliseconds,
        callback);
     if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

     return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndDeployServicePackageToNode(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeployServicePackageToNodeOperation::End(context));
}

HRESULT ComFabricClient::BeginGetNodeLoadInformation(
    /* [in] */ const FABRIC_NODE_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetNodeLoadInformationOperation> operation = make_com<GetNodeLoadInformationOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetNodeLoadInformation(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetNodeLoadInformationResult **result)
{
    return ComUtility::OnPublicApiReturn(GetNodeLoadInformationOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetReplicaLoadInformation(
    /* [in] */ const FABRIC_REPLICA_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetReplicaLoadInformationOperation> operation = make_com<GetReplicaLoadInformationOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetReplicaLoadInformation(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetReplicaLoadInformationResult **result)
{
    return ComUtility::OnPublicApiReturn(GetReplicaLoadInformationOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetUnplacedReplicaInformation(
    /* [in] */ const FABRIC_UNPLACED_REPLICA_INFORMATION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetUnplacedReplicaInformationOperation> operation = make_com<GetUnplacedReplicaInformationOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetUnplacedReplicaInformation(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetUnplacedReplicaInformationResult **result)
{
    return ComUtility::OnPublicApiReturn(GetUnplacedReplicaInformationOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetApplicationLoadInformation(
    /* [in] */ const FABRIC_APPLICATION_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetApplicationLoadInformationOperation> operation = make_com<GetApplicationLoadInformationOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationLoadInformation(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetApplicationLoadInformationResult **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationLoadInformationOperation::End(context, result));
}

//
// IUpgradeOrchestrationClient methods
//

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginUpgradeConfiguration(
    /* [in] */ const FABRIC_START_UPGRADE_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (description == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<StartClusterConfigurationUpgradeAsyncOperation> operation = make_com<StartClusterConfigurationUpgradeAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        description,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage));
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndUpgradeConfiguration(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(StartClusterConfigurationUpgradeAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetClusterConfigurationUpgradeStatus(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    ComPointer<GetClusterConfigurationUpgradeStatusAsyncOperation> operation = make_com<GetClusterConfigurationUpgradeStatusAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage));
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetClusterConfigurationUpgradeStatus(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricOrchestrationUpgradeStatusResult ** result)
{
    return ComUtility::OnPublicApiReturn(
        GetClusterConfigurationUpgradeStatusAsyncOperation::End(
        context,
        IID_IFabricOrchestrationUpgradeStatusResult,
        reinterpret_cast<void**>(result)));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetClusterConfiguration(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetClusterConfigurationAsyncOperation> operation = make_com<GetClusterConfigurationAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        NULL,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetClusterConfiguration(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterConfigurationAsyncOperation::End(context, result));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetClusterConfiguration2(
    /* [in] */ LPCWSTR apiVersion,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetClusterConfigurationAsyncOperation> operation = make_com<GetClusterConfigurationAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        apiVersion,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetClusterConfiguration2(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(GetClusterConfigurationAsyncOperation::End(context, result));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetUpgradeOrchestrationServiceState(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetUpgradeOrchestrationServiceStateAsyncOperation> operation = make_com<GetUpgradeOrchestrationServiceStateAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetUpgradeOrchestrationServiceState(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **result)
{
    return ComUtility::OnPublicApiReturn(GetUpgradeOrchestrationServiceStateAsyncOperation::End(context, result));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginSetUpgradeOrchestrationServiceState(
    /* [in] */ LPCWSTR state,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (state == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<SetUpgradeOrchestrationServiceStateAsyncOperation> operation = make_com<SetUpgradeOrchestrationServiceStateAsyncOperation>(*this);

    HRESULT hr = operation->Initialize(
        state,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndSetUpgradeOrchestrationServiceState(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricUpgradeOrchestrationServiceStateResult **result)
{
    return ComUtility::OnPublicApiReturn(SetUpgradeOrchestrationServiceStateAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetUpgradesPendingApproval(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    ComPointer<GetUpgradesPendingApprovalAsyncOperation> operation = make_com<GetUpgradesPendingApprovalAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage));
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetUpgradesPendingApproval(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(GetUpgradesPendingApprovalAsyncOperation::End(context));
}

HRESULT ComFabricClient::BeginStartApprovedUpgrades(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<StartApprovedUpgradesAsyncOperation> operation = make_com<StartApprovedUpgradesAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage));
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndStartApprovedUpgrades(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(StartApprovedUpgradesAsyncOperation::End(context));
}

//
// IFabricSecretStoreClient methods
//

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetSecrets(
    /* [in] */ const FABRIC_SECRET_REFERENCE_LIST *secretReferences,
    /* [in] */ BOOLEAN includeValue,
    /* [in] */  DWORD timeoutMilliseconds,
    /* [in] */  IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetSecretsAsyncOperation> operation = make_com<GetSecretsAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        secretReferences,
        includeValue,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage)); 
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetSecrets(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricSecretsResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetSecretsAsyncOperation::End(context, result));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginSetSecrets(
    /* [in] */ const FABRIC_SECRET_LIST *secrets,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<SetSecretsAsyncOperation> operation = make_com<SetSecretsAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        secrets,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage));
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndSetSecrets(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricSecretsResult **result)
{
    return ComUtility::OnPublicApiReturn(SetSecretsAsyncOperation::End(context, result));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginRemoveSecrets(
    /* [in] */ const FABRIC_SECRET_REFERENCE_LIST *secretReferences,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RemoveSecretsAsyncOperation> operation = make_com<RemoveSecretsAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        secretReferences,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage));
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndRemoveSecrets(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricSecretReferencesResult **result)
{
    return ComUtility::OnPublicApiReturn(RemoveSecretsAsyncOperation::End(context, result));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginGetSecretVersions(
    /* [in] */ const FABRIC_SECRET_REFERENCE_LIST *secretReferences,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetSecretVersionsAsyncOperation> operation = make_com<GetSecretVersionsAsyncOperation>(*this);

    wstring errorMessage;

    HRESULT hr = operation->Initialize(
        secretReferences,
        timeoutMilliseconds,
        callback,
        errorMessage);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr, move(errorMessage));
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::EndGetSecretVersions(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricSecretReferencesResult **result)
{
    return ComUtility::OnPublicApiReturn(GetSecretVersionsAsyncOperation::End(context, result));
}

HRESULT ComFabricClient::Upload(
    /* [in] */ LPCWSTR sourceFullPath,
    /* [in] */ LPCWSTR destinationRelativePath,
    /* [in] */ BOOLEAN shouldOverwrite)
{
    if (sourceFullPath == NULL || destinationRelativePath == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ErrorCode error = imageStoreClient_->Upload(
        L"fabric:ImageStore",
        sourceFullPath,
        destinationRelativePath,
        shouldOverwrite == TRUE);

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComFabricClient::Delete(
    /* [in] */ LPCWSTR relativePath)
{
    if (relativePath == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ErrorCode error = imageStoreClient_->Delete(
        L"fabric:ImageStore",
        relativePath);

    return ComUtility::OnPublicApiReturn(move(error));
}

//
// IFabricQueryClient8 methods.
//

HRESULT ComFabricClient::BeginGetServiceName(
    /* [in] */ const FABRIC_SERVICE_NAME_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetServiceNameOperation> operation = make_com<GetServiceNameOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetServiceName(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetServiceNameResult **result)
{
    return ComUtility::OnPublicApiReturn(GetServiceNameOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetApplicationName(
    /* [in] */ const FABRIC_APPLICATION_NAME_QUERY_DESCRIPTION *queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetApplicationNameOperation> operation = make_com<GetApplicationNameOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationName(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetApplicationNameResult **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationNameOperation::End(context, result));
}

//
// IFabricQueryClient9 methods.
//

HRESULT ComFabricClient::BeginGetApplicationTypePagedList(
    /* [in] */ const PAGED_FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetApplicationTypePagedListOperation> operation = make_com<GetApplicationTypePagedListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationTypePagedList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetApplicationTypePagedListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationTypePagedListOperation::End(context, result));
}

HRESULT STDMETHODCALLTYPE ComFabricClient::BeginCreateComposeDeployment(
    /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION *applicationDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (applicationDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (applicationDescription->DeploymentName == NULL ||
        applicationDescription->ComposeFilePath == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<CreateComposeDeploymentOperation> operation = make_com<CreateComposeDeploymentOperation>(*this);
    HRESULT hr = operation->Initialize(
        applicationDescription->DeploymentName,
        applicationDescription->ComposeFilePath,
        applicationDescription->ContainerRepositoryUserName,
        applicationDescription->ContainerRepositoryPassword,
        applicationDescription->IsPasswordEncrypted,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateComposeDeployment(
    /*[in]*/ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(CreateComposeDeploymentOperation::End(context));
}

HRESULT ComFabricClient::BeginGetComposeDeploymentStatusList(
    /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if (queryDescription == nullptr)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetComposeDeploymentStatusListOperation> operation = make_com<GetComposeDeploymentStatusListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetComposeDeploymentStatusList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetComposeDeploymentStatusListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetComposeDeploymentStatusListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginDeleteComposeDeployment(
    /* [in] */ const FABRIC_DELETE_COMPOSE_DEPLOYMENT_DESCRIPTION *description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == nullptr) ||
        (description == nullptr) ||
        (description->DeploymentName == nullptr))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<DeleteComposeDeploymentOperation> operation = make_com<DeleteComposeDeploymentOperation>(*this);

    // TODO: take description class after compose structure is updated
    HRESULT hr = operation->Initialize(
        description->DeploymentName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndDeleteComposeDeployment(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeleteComposeDeploymentOperation::End(context));
}

HRESULT ComFabricClient::BeginUpgradeComposeDeployment(
    /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION * upgradeDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    if ((context == nullptr) ||
        (upgradeDescription == nullptr) ||
        (upgradeDescription->DeploymentName == nullptr) ||
        (upgradeDescription->ComposeFilePaths.Count == 0))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<UpgradeComposeDeploymentOperation> operation = make_com<UpgradeComposeDeploymentOperation>(*this);

    HRESULT hr = operation->Initialize(
        upgradeDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndUpgradeComposeDeployment(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(UpgradeComposeDeploymentOperation::End(context));
}

HRESULT ComFabricClient::BeginGetComposeDeploymentUpgradeProgress(
    /* [in] */ LPCWSTR deploymentName,
    /* [in] */ DWORD timeoutMilliSeconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == nullptr || deploymentName == nullptr)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<GetComposeDeploymentUpgradeProgressOperation> operation = make_com<GetComposeDeploymentUpgradeProgressOperation>(*this);

    HRESULT hr = operation->Initialize(
        deploymentName,
        timeoutMilliSeconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetComposeDeploymentUpgradeProgress(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricComposeDeploymentUpgradeProgressResult **result)
{
    return ComUtility::OnPublicApiReturn(GetComposeDeploymentUpgradeProgressOperation::End(
        context,
        IID_IFabricComposeDeploymentUpgradeProgressResult,
        reinterpret_cast<void**>(result)));
}

HRESULT ComFabricClient::BeginRollbackComposeDeployment(
    /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_ROLLBACK_DESCRIPTION *rollbackDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if ((context == nullptr) ||
        (rollbackDescription == nullptr) ||
        (rollbackDescription->DeploymentName == nullptr))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<RollbackComposeDeploymentOperation> operation = make_com<RollbackComposeDeploymentOperation>(*this);

    HRESULT hr = operation->Initialize(
        rollbackDescription->DeploymentName,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndRollbackComposeDeployment(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RollbackComposeDeploymentOperation::End(context));
}


//
// IFabricQueryClient10 methods.
//

HRESULT ComFabricClient::BeginGetDeployedApplicationPagedList(
    /* [in] */ const FABRIC_PAGED_DEPLOYED_APPLICATION_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (queryDescription == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(queryDescription);

    ComPointer<GetDeployedApplicationPagedListOperation> operation = make_com<GetDeployedApplicationPagedListOperation>(*this);
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedApplicationPagedList(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetDeployedApplicationPagedListResult **result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedApplicationPagedListOperation::End(context, result));
}

//
// IFabricNetworkManagementClient methods
//

HRESULT ComFabricClient::BeginCreateNetwork(
    /* [in] */ LPCWSTR networkName,
    /* [in] */ const FABRIC_NETWORK_DESCRIPTION * description,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (description == NULL))				
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<CreateNetworkOperation>(*this);

    NetworkDescription networkDescription;
    auto error = networkDescription.FromPublicApi(*description);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::InitializeAndComplete(
            error,
            move(operation),
            callback,
            context));
    }

    wstring networkNameWstring;
    HRESULT hr = StringUtility::LpcwstrToWstring(
        networkName,
        false, // dont accept null
        networkNameWstring);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    hr = operation->Initialize(
        networkNameWstring,
        networkDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndCreateNetwork(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(CreateNetworkOperation::End(context));
}

HRESULT ComFabricClient::BeginDeleteNetwork(
    /* [in] */ const FABRIC_DELETE_NETWORK_DESCRIPTION * deleteDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (deleteDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<DeleteNetworkOperation>(*this);

    DeleteNetworkDescription deleteNetworkDescription;
    auto error = deleteNetworkDescription.FromPublicApi(*deleteDescription);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::InitializeAndComplete(
            error,
            move(operation),
            callback,
            context));
    }

    HRESULT hr = operation->Initialize(
        deleteNetworkDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));

}

HRESULT ComFabricClient::EndDeleteNetwork(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(DeleteNetworkOperation::End(context));
}


HRESULT ComFabricClient::BeginGetNetworkList(
    /* [in] */ const FABRIC_NETWORK_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (queryDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetNetworkListOperation>(*this);
    
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetNetworkList(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricGetNetworkListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetNetworkListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetNetworkApplicationList(
    /* [in] */ const FABRIC_NETWORK_APPLICATION_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (queryDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetNetworkApplicationListOperation>(*this);
    
    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetNetworkApplicationList(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricGetNetworkApplicationListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetNetworkApplicationListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetNetworkNodeList(
    /* [in] */ const FABRIC_NETWORK_NODE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (queryDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetNetworkNodeListOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetNetworkNodeList(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricGetNetworkNodeListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetNetworkNodeListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetApplicationNetworkList(
    /* [in] */ const FABRIC_APPLICATION_NETWORK_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (queryDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetApplicationNetworkListOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetApplicationNetworkList(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricGetApplicationNetworkListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetApplicationNetworkListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedNetworkList(
    /* [in] */ const FABRIC_DEPLOYED_NETWORK_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (queryDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetDeployedNetworkListOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedNetworkList(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricGetDeployedNetworkListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedNetworkListOperation::End(context, result));
}

HRESULT ComFabricClient::BeginGetDeployedNetworkCodePackageList(
    /* [in] */ const FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if ((context == NULL) ||
        (queryDescription == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto operation = make_com<GetDeployedNetworkCodePackageListOperation>(*this);

    HRESULT hr = operation->Initialize(
        queryDescription,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricClient::EndGetDeployedNetworkCodePackageList(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ IFabricGetDeployedNetworkCodePackageListResult ** result)
{
    return ComUtility::OnPublicApiReturn(GetDeployedNetworkCodePackageListOperation::End(context, result));
}