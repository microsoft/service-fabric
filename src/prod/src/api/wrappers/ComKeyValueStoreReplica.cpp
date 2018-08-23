// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Store;

StringLiteral const TraceComponent("ComKeyValueStoreReplica");

// ********************************************************************************************************************
// ComKeyValueStoreReplica::RestoreOperationContext Implementation
//

// {566BE0DD-D55B-4E9C-B058-0A467E7F40CB}
static const GUID CLSID_ComKeyValueStoreReplica_RestoreOperationContext =
{ 0x566be0dd, 0xd55b, 0x4e9c, { 0xb0, 0x58, 0xa, 0x46, 0x7e, 0x7f, 0x40, 0xcb } };

class ComKeyValueStoreReplica::RestoreOperationContext : public ComAsyncOperationContext
{
    DENY_COPY(RestoreOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        RestoreOperationContext,
        CLSID_ComKeyValueStoreReplica_RestoreOperationContext,
        RestoreOperationContext,
        ComAsyncOperationContext)

public:
    RestoreOperationContext(__in ComKeyValueStoreReplica & owner)
        : ComAsyncOperationContext()
        , owner_(owner)
        , settings_()
    {
    }

    virtual ~RestoreOperationContext()
    {
    }

    HRESULT Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in std::wstring const & backupDir,
        __in FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS * settings,
        __in IFabricAsyncOperationCallback * callback)
    {
        backupDir_ = backupDir;

        if (settings != NULL)
        {
            auto error = settings_.FromPublicApi(*settings);
            if (!error.IsSuccess()) { return error.ToHResult(); }
        }

        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (!context) { return E_POINTER; }

        ComPointer<RestoreOperationContext> thisOperation(context, CLSID_ComKeyValueStoreReplica_RestoreOperationContext);
        auto hr = thisOperation->Result;

        return hr;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.impl_->BeginRestore(
            backupDir_,
            settings_,
            [this](AsyncOperationSPtr const & operation){ this->FinishRestore(operation); },
            proxySPtr);
    }

private:
    void FinishRestore(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.impl_->EndRestore(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComKeyValueStoreReplica & owner_;
    std::wstring backupDir_;
    Store::RestoreSettings settings_;
};

// ********************************************************************************************************************
// ComKeyValueStoreReplica::BackupOperationContext Implementation
//

// {AEE97E9F-052B-4F1B-9F34-8BD274AA8AC0}
static const GUID CLSID_ComKeyValueStoreReplica_BackupOperationContext =
{ 0xaee97e9f, 0x52b, 0x4f1b, { 0x9f, 0x34, 0x8b, 0xd2, 0x74, 0xaa, 0x8a, 0xc0 } };

class ComKeyValueStoreReplica::BackupOperationContext :
public ComAsyncOperationContext
{
    DENY_COPY(BackupOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        BackupOperationContext,
        CLSID_ComKeyValueStoreReplica_BackupOperationContext,
        BackupOperationContext,
        ComAsyncOperationContext)

public:
    BackupOperationContext(__in ComKeyValueStoreReplica & owner)
        : ComAsyncOperationContext()
        , owner_(owner)
    {
    }

    virtual ~BackupOperationContext()
    {
    }

    HRESULT Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in std::wstring const & backupDir,
        __in FABRIC_STORE_BACKUP_OPTION backupOption,
        __in IFabricStorePostBackupHandler * postBackupHandler,
        __in IFabricAsyncOperationCallback * callback)
    {
        backupDir_ = backupDir;
        backupOption_ = backupOption;

        postBackupHandler_ = Api::WrapperFactory::create_rooted_com_proxy(postBackupHandler);
                
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (!context) { return E_POINTER; }

        ComPointer<BackupOperationContext> thisOperation(context, CLSID_ComKeyValueStoreReplica_BackupOperationContext);
        auto hr = thisOperation->Result;

        return hr;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.impl_->BeginBackup(
            backupDir_,
            backupOption_,
            postBackupHandler_,
            [this](AsyncOperationSPtr const & operation){ this->FinishBackup(operation); },
            proxySPtr);
    }

private:
    void FinishBackup(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.impl_->EndBackup(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComKeyValueStoreReplica & owner_;
    std::wstring backupDir_;
    FABRIC_STORE_BACKUP_OPTION backupOption_;
    Api::IStorePostBackupHandlerPtr postBackupHandler_;
};

// ********************************************************************************************************************
// ComKeyValueStoreReplica::ComKeyValueStoreReplica Implementation
//

ComKeyValueStoreReplica::ComKeyValueStoreReplica(IKeyValueStoreReplicaPtr const & impl)
    : IFabricKeyValueStoreReplica6(),
    ComStatefulServiceReplica(IStatefulServiceReplicaPtr((IStatefulServiceReplica *)impl.get(), impl.get_Root())),
    impl_(impl)
{
}

ComKeyValueStoreReplica::~ComKeyValueStoreReplica()
{
}

HRESULT ComKeyValueStoreReplica::BeginOpen( 
    /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
    /* [in] */ IFabricStatefulServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComStatefulServiceReplica::BeginOpen(
            openMode,
            partition,
            callback,
            context);
}

HRESULT ComKeyValueStoreReplica::EndOpen( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricReplicator **replicator)
{
    return ComStatefulServiceReplica::EndOpen(
            context,
            replicator);

}

HRESULT ComKeyValueStoreReplica::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComStatefulServiceReplica::BeginChangeRole(
            newRole,
            callback,
            context);
}

HRESULT ComKeyValueStoreReplica::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    return ComStatefulServiceReplica::EndChangeRole(
            context,
            serviceAddress);
}

HRESULT ComKeyValueStoreReplica::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    return ComStatefulServiceReplica::BeginClose(
            callback,
            context);
}

HRESULT ComKeyValueStoreReplica::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComStatefulServiceReplica::EndClose(
            context);
}

void ComKeyValueStoreReplica::Abort(void)
{
    ComStatefulServiceReplica::Abort();
}

HRESULT ComKeyValueStoreReplica::GetCurrentEpoch( 
    /* [out] */ FABRIC_EPOCH *currentEpoch)
{
    if (currentEpoch == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto error = impl_->GetCurrentEpoch(*currentEpoch);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::UpdateReplicatorSettings( 
    /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings)
{
    if (replicatorSettings == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    FABRIC_REPLICATOR_SETTINGS_EX1 * replicatorSettingsEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1 *) replicatorSettings->Reserved;

    if (replicatorSettingsEx1 != NULL)
    {
        FABRIC_REPLICATOR_SETTINGS_EX2 * replicatorSettingsEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2 *)replicatorSettingsEx1->Reserved;

        if (replicatorSettingsEx2 != NULL)
        {
            FABRIC_REPLICATOR_SETTINGS_EX3 * replicatorSettingsEx3 = (FABRIC_REPLICATOR_SETTINGS_EX3 *)replicatorSettingsEx2->Reserved;

            if (replicatorSettingsEx3 != NULL)
            {
                FABRIC_REPLICATOR_SETTINGS_EX4 * replicatorSettingsEx4 = (FABRIC_REPLICATOR_SETTINGS_EX4 *)replicatorSettingsEx3->Reserved;
                FAIL_IF_OPTIONAL_PARAM_RESERVED_FIELD_NOT_NULL(replicatorSettingsEx4);
            }
        }
    }

    auto error = impl_->UpdateReplicatorSettings(*replicatorSettings);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::CreateTransaction( 
    /* [retval][out] */ IFabricTransaction **transaction)
{
    if (transaction == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ITransactionPtr transactionImpl;
    auto error = impl_->CreateTransaction(FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT, transactionImpl);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    auto transactionCPtr = WrapperFactory::create_com_wrapper(transactionImpl);
    *transaction = transactionCPtr.DetachNoRelease();
    
    return S_OK;
}

HRESULT ComKeyValueStoreReplica::CreateTransaction2( 
    const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS * settings,
    /* [retval][out] */ IFabricTransaction **transaction)
{
    if (transaction == NULL || settings == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ITransactionPtr transactionImpl;
    auto error = impl_->CreateTransaction(FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT, *settings, transactionImpl);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    auto transactionCPtr = WrapperFactory::create_com_wrapper(transactionImpl);
    *transaction = transactionCPtr.DetachNoRelease();
    
    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::Add( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [in] */ LONG valueSizeInBytes,
    /* [size_is][in] */ const BYTE *value)
{
    if (transaction == NULL || value == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if ((keyStr.length() == 0) || (valueSizeInBytes == 0))
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    auto error = impl_->Add(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        valueSizeInBytes,
        value);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::Remove( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber)
{
    if (transaction == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    auto error = impl_->Remove(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        checkSequenceNumber);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::Update( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [in] */ LONG valueSizeInBytes,
    /* [size_is][in] */ const BYTE *value,
    /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber)
{
    if (transaction == NULL || value == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if ((keyStr.length() == 0) || (valueSizeInBytes == 0))
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    auto error = impl_->Update(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        valueSizeInBytes,
        value,
        checkSequenceNumber);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::Get( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [retval][out] */ IFabricKeyValueStoreItemResult **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemResultPtr resultImpl;
    auto error = impl_->Get(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::GetMetadata( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemMetadataResultPtr resultImpl;
    auto error = impl_->GetMetadata(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::Contains( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [retval][out] */ BOOLEAN *result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemResultPtr resultImpl;
    auto error = impl_->Contains(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        *result);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::Enumerate( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result)
{
    if (transaction == NULL || result == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IKeyValueStoreItemEnumeratorPtr resultImpl;
    auto error = impl_->Enumerate(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::EnumerateByKey( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR keyPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, false /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyPrefixStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateByKey(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyPrefixStr,
        true, // strictPrefix
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::EnumerateMetadata( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result)
{
    if (transaction == NULL || result == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IKeyValueStoreItemMetadataEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateMetadata(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::EnumerateMetadataByKey( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR keyPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, false /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyPrefixStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemMetadataEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateMetadataByKey(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyPrefixStr,
        true, // strictPrefix
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}

HRESULT ComKeyValueStoreReplica::Backup( 
    /* [in] */ LPCWSTR backupDirectory)
{
    wstring dir;
    HRESULT hr = StringUtility::LpcwstrToWstring(backupDirectory, false /* acceptsNull */, dir);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (dir.empty())
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    auto error = impl_->Backup(dir);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}


HRESULT ComKeyValueStoreReplica::Restore( 
    /* [in] */ LPCWSTR backupDirectory)
{
    wstring dir;
    HRESULT hr = StringUtility::LpcwstrToWstring(backupDirectory, false /* acceptsNull */, dir);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (dir.empty())
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    auto error = impl_->Restore(dir);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

HRESULT ComKeyValueStoreReplica::BeginBackup(
    /* [in] */ LPCWSTR backupDir,
    /* [in] */ FABRIC_STORE_BACKUP_OPTION backupOption,
    /* [in] */ IFabricStorePostBackupHandler *postBackupHandler,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    bool allowNull = (backupOption == FABRIC_STORE_BACKUP_OPTION_TRUNCATE_LOGS_ONLY) ? true : false;
    
    wstring dir;
    HRESULT hr = StringUtility::LpcwstrToWstring(backupDir, allowNull, dir);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    ComPointer<IUnknown> rootCPtr;
    hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<BackupOperationContext> operation
        = make_com<BackupOperationContext>(*this);
        
    hr = operation->Initialize(
        root,
        dir,
        backupOption,
        postBackupHandler,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    hr = ComAsyncOperationContext::StartAndDetach(move(operation), context);

    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComKeyValueStoreReplica::EndBackup(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    HRESULT hr = BackupOperationContext::End(context);

    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComKeyValueStoreReplica::BeginRestore(
    /* [in] */ LPCWSTR backupDir,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    wstring dir;
    HRESULT hr = StringUtility::LpcwstrToWstring(backupDir, false, dir);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    ComPointer<IUnknown> rootCPtr;
    hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<RestoreOperationContext> operation
        = make_com<RestoreOperationContext>(*this);

    hr = operation->Initialize(
        root,
        dir,
        NULL,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    hr = ComAsyncOperationContext::StartAndDetach(move(operation), context);

    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComKeyValueStoreReplica::BeginRestore2(
    /* [in] */ LPCWSTR backupDir,
    /* [in] */ FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS * settings,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    wstring dir;
    HRESULT hr = StringUtility::LpcwstrToWstring(backupDir, false, dir);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    ComPointer<IUnknown> rootCPtr;
    hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<RestoreOperationContext> operation
        = make_com<RestoreOperationContext>(*this);

    hr = operation->Initialize(
        root,
        dir,
        settings,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    hr = ComAsyncOperationContext::StartAndDetach(move(operation), context);

    return ComUtility::OnPublicApiReturn(hr);
}

HRESULT ComKeyValueStoreReplica::EndRestore(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    HRESULT hr = RestoreOperationContext::End(context);

    return ComUtility::OnPublicApiReturn(hr);
}

// 
// IFabricKeyValueStoreReplica5 methods
// 

HRESULT ComKeyValueStoreReplica::TryAdd( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [in] */ LONG valueSizeInBytes,
    /* [size_is][in] */ const BYTE *value,
    /* [retval][out] */ BOOLEAN *added)
{
    if (transaction == NULL || value == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if ((keyStr.length() == 0) || (valueSizeInBytes == 0))
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    bool internalAdded = false;
    auto error = impl_->TryAdd(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        valueSizeInBytes,
        value,
        internalAdded);

    *added = (internalAdded ? TRUE : FALSE);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::TryRemove( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
    /* [retval][out] */ BOOLEAN *exists)
{
    if (transaction == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    bool internalExists = false;
    auto error = impl_->TryRemove(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        checkSequenceNumber,
        internalExists);

    *exists = (internalExists ? TRUE : FALSE);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::TryUpdate( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [in] */ LONG valueSizeInBytes,
    /* [size_is][in] */ const BYTE *value,
    /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
    /* [retval][out] */ BOOLEAN *exists)
{
    if (transaction == NULL || value == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if ((keyStr.length() == 0) || (valueSizeInBytes == 0))
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    bool internalExists = false;
    auto error = impl_->TryUpdate(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        valueSizeInBytes,
        value,
        checkSequenceNumber,
        internalExists);

    *exists = (internalExists ? TRUE : FALSE);

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}
        
HRESULT ComKeyValueStoreReplica::TryGet( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [retval][out] */ IFabricKeyValueStoreItemResult **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemResultPtr resultImpl;
    auto error = impl_->TryGet(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    if (resultImpl.get() != nullptr)
    {
        auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
        *result = resultCPtr.DetachNoRelease();
    }
    else
    {
        *result = NULL;
    }

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::TryGetMetadata( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR key,
    /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(key, false /* acceptsNull */, keyStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemMetadataResultPtr resultImpl;
    auto error = impl_->TryGetMetadata(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyStr,
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    if (resultImpl.get() != nullptr)
    {
        auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
        *result = resultCPtr.DetachNoRelease();
    }
    else
    {
        *result = NULL;
    }

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::EnumerateByKey2( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR keyPrefix,
    /* [in] */ BOOLEAN strictPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, false /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyPrefixStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateByKey(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyPrefixStr,
        (strictPrefix != FALSE),
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreReplica::EnumerateMetadataByKey2( 
    /* [in] */ IFabricTransactionBase *transaction,
    /* [in] */ LPCWSTR keyPrefix,
    /* [in] */ BOOLEAN strictPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result)
{
    if (transaction == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, false /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    if (keyPrefixStr.length() == 0)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG); 
    }

    IKeyValueStoreItemMetadataEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateMetadataByKey(
        WrapperFactory::create_com_proxy_wrapper(transaction),
        keyPrefixStr,
        (strictPrefix != FALSE),
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
