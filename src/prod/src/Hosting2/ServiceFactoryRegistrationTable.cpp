// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

// ********************************************************************************************************************
// ServiceFactoryRegistrationTable::Entry Implementation
//

class ServiceFactoryRegistrationTable::Entry
{
    DENY_COPY(Entry)

public:
    Entry(ServiceFactoryRegistrationSPtr const & registration)
        : Registration(registration),
        IsValid(false),
        EntryValidAsyncSPtr(nullptr)
    {
    }

    ServiceFactoryRegistrationSPtr const Registration;
    bool IsValid;
    //Keeps track of the primary AsyncOperation to create a replica/instance for a ServiceTypeId
    AsyncOperationSPtr EntryValidAsyncSPtr;
};

// ********************************************************************************************************************
// ServiceFactoryRegistrationTable::EntryValidationLinkedAsyncOperation Implementation
//
class ServiceFactoryRegistrationTable::EntryValidationLinkedAsyncOperation
    : public LinkableAsyncOperation
{
    DENY_COPY(EntryValidationLinkedAsyncOperation);

public:
    EntryValidationLinkedAsyncOperation(
        __in ServiceFactoryRegistrationTable & owner,
        ServiceTypeIdentifier const & serviceTypeId,
        ErrorCode const & errorCode,
        AsyncOperationSPtr const & primary,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(primary, callback, parent),
        owner_(owner),
        serviceTypeId_(serviceTypeId),
        errorCode_(errorCode)
    {
    }

    EntryValidationLinkedAsyncOperation(
        ServiceFactoryRegistrationTable & owner,
        ServiceTypeIdentifier const & serviceTypeId,
        ErrorCode const & errorCode,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(callback, parent),
        owner_(owner),
        serviceTypeId_(serviceTypeId),
        errorCode_(errorCode)
    {
    }

    __declspec(property(get = get_ServiceTypeIdentifier)) ServiceTypeIdentifier const & ServiceTypeId;
    inline ServiceTypeIdentifier const & get_ServiceTypeIdentifier() const { return serviceTypeId_; }

    virtual ~EntryValidationLinkedAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<EntryValidationLinkedAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr)
    {
        //Complete the Async Operation if the serviceType is not present or is valid
        if (!errorCode_.IsError(ErrorCodeValue::HostingServiceTypeValidationInProgress))
        {
            thisSPtr->TryComplete(thisSPtr, errorCode_);
        }
    }

private:
    ServiceFactoryRegistrationTable & owner_;
    ServiceTypeIdentifier const serviceTypeId_;
    ErrorCode errorCode_;
};

// ********************************************************************************************************************
// ServiceFactoryRegistrationTable Implementation
//

ServiceFactoryRegistrationTable::ServiceFactoryRegistrationTable()
    : lock_(),
    map_()
{
}

ErrorCode ServiceFactoryRegistrationTable::Add(ServiceFactoryRegistrationSPtr const & registration)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(registration->ServiceTypeId);
        if (iter != map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingServiceTypeAlreadyRegistered);
        }

        map_.insert(
            make_pair(
            registration->ServiceTypeId,
            make_shared<Entry>(registration)));
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServiceFactoryRegistrationTable::Remove(ServiceTypeIdentifier const & serviceTypeId, bool removeInvalid)
{
    AsyncOperationSPtr operation = nullptr;
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(serviceTypeId);

        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered);
        }

        if ((!iter->second->IsValid) && !removeInvalid)
        {
            return ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered);
        }

        // Swap the pending primary Find Async Operation before removing the serviceType entry
        operation.swap(iter->second->EntryValidAsyncSPtr);

        map_.erase(iter);
    }

    if (operation != nullptr)
    {
		//Post the completion of the primary linakbleAsyncOperation on the different thread
		Threadpool::Post([operation]() {
			operation->TryComplete(operation, ErrorCodeValue::HostingServiceTypeNotRegistered);
		});
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServiceFactoryRegistrationTable::Validate(ServiceTypeIdentifier const & serviceTypeId)
{
    AsyncOperationSPtr operation = nullptr;
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(serviceTypeId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered);
        }

        iter->second->IsValid = true;

        operation.swap(iter->second->EntryValidAsyncSPtr);
    }

    if (operation != nullptr)
    {
        //Post the completion of the primary linakbleAsyncOperation on the different thread
        Threadpool::Post([operation]() {
            operation->TryComplete(operation, ErrorCode(ErrorCodeValue::Success));
        });
    }

    return ErrorCode(ErrorCodeValue::Success);
}

AsyncOperationSPtr ServiceFactoryRegistrationTable::BeginFind(
    ServiceTypeIdentifier const & serviceTypeId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    AsyncOperationSPtr operation;
    {
        AcquireWriteLock lock(lock_);

        auto iter = map_.find(serviceTypeId);
        if (iter == map_.end())
        {
            operation = AsyncOperation::CreateAndStart<EntryValidationLinkedAsyncOperation>(
                *this,
                serviceTypeId,
                ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered),
                callback,
                parent);
        }
        else if (!(iter->second->IsValid))
        {
            if (iter->second->EntryValidAsyncSPtr != nullptr)
            {
                //secondary
                operation = AsyncOperation::CreateAndStart<EntryValidationLinkedAsyncOperation>(
                    *this,
                    serviceTypeId,
                    ErrorCode(ErrorCodeValue::HostingServiceTypeValidationInProgress),
                    iter->second->EntryValidAsyncSPtr,
                    callback,
                    parent);
            }
            else
            {
                //primary
                operation = AsyncOperation::CreateAndStart<EntryValidationLinkedAsyncOperation>(
                    *this,
                    serviceTypeId,
                    ErrorCode(ErrorCodeValue::HostingServiceTypeValidationInProgress),
                    callback,
                    parent);

                iter->second->EntryValidAsyncSPtr = operation;
            }
        }
        else
        {
            operation = AsyncOperation::CreateAndStart<EntryValidationLinkedAsyncOperation>(
                *this,
                serviceTypeId,
                ErrorCode(ErrorCodeValue::Success),
                callback,
                parent);
        }
    }

    AsyncOperation::Get<EntryValidationLinkedAsyncOperation>(operation)->ResumeOutsideLock(operation);

    return operation;
}

ErrorCode ServiceFactoryRegistrationTable::EndFind(
    AsyncOperationSPtr const & operation,
    __out ServiceFactoryRegistrationSPtr & registration,
    __out ServiceTypeIdentifier & serviceTypeId)
{
    auto linkableOperation = static_cast<EntryValidationLinkedAsyncOperation*>(operation.get());
    serviceTypeId = linkableOperation->ServiceTypeId;

    //Do a lookup to get the Registration for the ServiceTypeId
    {
        AcquireReadLock lock(lock_);
        auto iter = map_.find(serviceTypeId);
        if (iter != map_.end())
        {
            registration = iter->second->Registration;
        }
    }

    return EntryValidationLinkedAsyncOperation::End(operation);
}

bool ServiceFactoryRegistrationTable::Test_IsServiceTypeInvalid(ServiceTypeIdentifier const & serviceTypeId)
{
    {
        AcquireReadLock lock(lock_);
        auto iter = map_.find(serviceTypeId);
        if ((iter == map_.end()) || (iter->second->IsValid))
        {
            return false;
        }
    }

    return true;
}
