// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;
using namespace ImageModel;

StringLiteral const TraceNodeEnvironmentManager("NodeEnvironmentManager");

NodeEnvironmentManager::NodeEnvironmentManager(
    ComponentRoot const & root,
    wstring const & nodeId)
    : RootedObject(root),
    nodeId_(nodeId),
    applicationPrincipalsMap_(),
    lock_()
{
}

NodeEnvironmentManager::~NodeEnvironmentManager()
{
}

ErrorCode NodeEnvironmentManager::SetupSecurityPrincipals(
    ConfigureSecurityPrincipalRequest && request,
    __inout vector<SecurityPrincipalInformationSPtr> & principalInformation)
{
    if (request.HasEmptyGroupsAndUsers())
    {
        WriteInfo(
            TraceNodeEnvironmentManager,
            Root.TraceId,
            "NodeId {0}: SetupSecurityPrincipals, No users or groups to create",
            nodeId_);
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto applicationId = request.ApplicationId;
    bool isConfiguredForNode = request.IsConfiguredForNode;
	bool isUpdateRequest = request.UpdateExisting;

    ErrorCode error(ErrorCodeValue::Success);
    ApplicationPrincipalsSPtr appPrincipals;
    bool isNewAppPrincipal;
    { // lock
        AcquireWriteLock lock(lock_);
        ApplicationPrincipalsMap & principalsMap = isConfiguredForNode ? nodePrincipalsMap_ : applicationPrincipalsMap_;

        auto iter = principalsMap.find(applicationId);
        if(iter == principalsMap.end())
        {
            appPrincipals = make_shared<ApplicationPrincipals>(move(request));
            principalsMap.insert(make_pair(applicationId, appPrincipals));
            isNewAppPrincipal = true;
        }
        else
        {
            appPrincipals = iter->second;
            isNewAppPrincipal = false;
        }
    } // endlock

    if (isNewAppPrincipal || !isUpdateRequest)
    {
		//
		// If this is not a new entry and it is not an update request, it means it is retry request to setup
		// the app principals. This can happen for the case when setting up app principal takes long enough
		// to timeout the initial activation operation which will then retry the operation.
		//
        error = appPrincipals->Open();
    }
    else
    {
        error = appPrincipals->UpdateApplicationPrincipals(move(request));
    }

    if (error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError))
    {
        ApplicationPrincipalsSPtr updatedAppPrincipals;

        { // lock
            AcquireWriteLock lock(lock_);
            ApplicationPrincipalsMap & principalsMap = isConfiguredForNode ? nodePrincipalsMap_ : applicationPrincipalsMap_;

            auto iter = principalsMap.find(applicationId);
            if(iter != principalsMap.end())
            {
                updatedAppPrincipals = move(iter->second);
                principalsMap.erase(iter);
            }
        } // endlock

        if (updatedAppPrincipals)
        {
            WriteWarning(
                TraceNodeEnvironmentManager,
                Root.TraceId,
                "{0}: App {1}: SetupSecurityPrincipals failed with {2}, abort application principals",
                nodeId_,
                applicationId,
                error);
            updatedAppPrincipals->Abort();
        }
        else
        {
            WriteWarning(
                TraceNodeEnvironmentManager,
                Root.TraceId,
                "{0}: App {1}: SetupSecurityPrincipals failed with {2}. Cleanup application principals did not found the principal, skip.",
                nodeId_,
                applicationId,
                error);
        }
    }
    else
    {
        auto getPrincipalsError = appPrincipals->GetSecurityPrincipalInformation(principalInformation);
        if (!getPrincipalsError.IsSuccess() && error.IsSuccess())
        {
            error = move(getPrincipalsError);
        }

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceNodeEnvironmentManager,
                Root.TraceId,
                "NodeId {0}: SetupSecurityPrincipals succeeds",
                nodeId_);
        }
    }

    return error;
}


ErrorCode NodeEnvironmentManager::Cleanup()
{
    return Cleanup(false);
}

ErrorCode NodeEnvironmentManager::Cleanup(bool removePrincipals)
{
    vector<ApplicationPrincipalsSPtr> appPrincipals;
    {
        AcquireWriteLock lock(lock_);
        for(auto iter = this->applicationPrincipalsMap_.begin(); iter != this->applicationPrincipalsMap_.end(); ++iter)
        {
            if(iter->second)
            {
                appPrincipals.push_back(move(iter->second));
            }
        }
        
        this->applicationPrincipalsMap_.clear();        

        for(auto iter = this->nodePrincipalsMap_.begin(); iter != this->nodePrincipalsMap_.end(); ++iter)
        {
            if(iter->second)
            {
                appPrincipals.push_back(move(iter->second));
            }
        }

        this->nodePrincipalsMap_.clear();
    }

    for (auto iter = appPrincipals.begin(); iter != appPrincipals.end(); ++iter)
    {
        auto error = (*iter)->CloseApplicationPrincipals(removePrincipals);
        WriteTrace(error.ToLogLevel(),
            TraceNodeEnvironmentManager,
            Root.TraceId,
            "Close ApplicationPrincipals for appId {0}. Error {1}",
            (*iter)->ApplicationId,
            error);
    }

    WriteInfo(
        TraceNodeEnvironmentManager,
        Root.TraceId,
        "Environment cleanup completed for NodeId {0}.",
        nodeId_);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode NodeEnvironmentManager::Cleanup(wstring const & applicationId, bool const cleanupForNode)
{
    ApplicationPrincipalsSPtr appPrincipal = nullptr;
    {
        AcquireWriteLock lock(lock_);

        std::map<std::wstring, ApplicationPrincipalsSPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> & principalsMap =
            cleanupForNode ? nodePrincipalsMap_ : applicationPrincipalsMap_;

        auto iter = principalsMap.find(applicationId);
        if(iter == principalsMap.end())
        {
            return ErrorCode(ErrorCodeValue::ApplicationPrincipalDoesNotExist);
        }
        appPrincipal = move(iter->second);
        principalsMap.erase(iter);
    }

    if(appPrincipal)
    {
        auto error = appPrincipal->Close();
        WriteTrace(error.ToLogLevel(),
            TraceNodeEnvironmentManager,
            Root.TraceId,
            "Close ApplicationPrincipals for appId {0}. Error {1}",
            (appPrincipal)->ApplicationId,
            error);
    }    

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode NodeEnvironmentManager::GetSecurityUser(
    wstring const & applicationId,
    wstring const & name,
    __out SecurityUserSPtr & secUser)
{
    AcquireReadLock lock(lock_);
    auto it = this->applicationPrincipalsMap_.find(applicationId);
    if(it == this->applicationPrincipalsMap_.end())
    {
        WriteWarning(
            TraceNodeEnvironmentManager,
            Root.TraceId,
            "Failed to get application id {0} from map.",
            applicationId);
        return ErrorCode(ErrorCodeValue::ApplicationNotFound);
    }
    auto error = it->second->GetSecurityUser(name, secUser);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceNodeEnvironmentManager,
            Root.TraceId,
            "Failed to get user {0} from map. Error {1}",
            name,
            error);
    }

    return error;
}

ErrorCode NodeEnvironmentManager::GetSecurityGroup(
    wstring const & applicationId,
    wstring const & name,
    __out SecurityGroupSPtr & secGroup)
{
    AcquireReadLock lock(lock_);
    auto it = this->applicationPrincipalsMap_.find(applicationId);
    if(it == this->applicationPrincipalsMap_.end())
    {
        WriteWarning(
            TraceNodeEnvironmentManager,
            Root.TraceId,
            "Failed to get application id {0} from map.",
            applicationId);
        return ErrorCode(ErrorCodeValue::ApplicationNotFound);
    }
    auto error = it->second->GetSecurityGroup(name, secGroup);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceNodeEnvironmentManager,
            Root.TraceId,
            "Failed to get group {0} from map. Error {1}",
            name,
            error);
    }

    return error;
}


