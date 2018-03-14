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

StringLiteral const TraceSecurityPrincipalsProvider("SecurityPrincipalsProvider");

SecurityPrincipalsProvider::SecurityPrincipalsProvider(ComponentRoot const & root)
    : RootedObject(root),
    FabricComponent(),
    envManagerMap_(),
    lock_()
{
}

SecurityPrincipalsProvider::~SecurityPrincipalsProvider()
{
}

ErrorCode SecurityPrincipalsProvider::AddGetNodeEnvManager(
    wstring const & nodeId,
    __out NodeEnvironmentManagerSPtr & envMgr)
{
    auto error = this->envManagerMap_.Get(nodeId, envMgr);
    if(error.IsSuccess())
    {
        return error;
    }
    if(error.IsError(ErrorCodeValue::NotFound))
    {
        envMgr = make_shared<NodeEnvironmentManager>(this->Root, nodeId);
        AcquireWriteLock lock(lock_);
        error = this->envManagerMap_.Add(nodeId, envMgr);
        if(!error.IsSuccess())
        {
            if(error.IsError(ErrorCodeValue::AlreadyExists))
            {
                return this->envManagerMap_.Get(nodeId, envMgr);
            }
        }
    }
    return error;
}

ErrorCode SecurityPrincipalsProvider::Setup(
    ConfigureSecurityPrincipalRequest && request,
    __inout vector<SecurityPrincipalInformationSPtr> & principalInformation)
{
    if (State.Value != FabricComponentState::Opened)
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    if (request.HasEmptyGroupsAndUsers())
    {
        WriteInfo(
            TraceSecurityPrincipalsProvider,
            Root.TraceId,
            "Node {0}: App {1}: No users or groups to create",
            request.NodeId,
            request.ApplicationId);
        return ErrorCode(ErrorCodeValue::Success);
    }

    NodeEnvironmentManagerSPtr envMgr;
    auto error = AddGetNodeEnvManager(request.NodeId, envMgr);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceSecurityPrincipalsProvider,
            Root.TraceId,
            "Failed to create NodeEnvironmentManager for Node {0}. Error {1}",
            request.NodeId,
            error);
        return error;
    }

    error = envMgr->SetupSecurityPrincipals(move(request), principalInformation);
    return error;
}

ErrorCode SecurityPrincipalsProvider::Cleanup(
    wstring const & nodeId,
    wstring const & applicationId,
    bool const cleanupForNode)
{
    NodeEnvironmentManagerSPtr envMgr;
    auto error = this->envManagerMap_.Get(nodeId, envMgr);
    if(!error.IsSuccess())
    {
        if(error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceSecurityPrincipalsProvider,
                Root.TraceId,
                "NodeEnvMgr for NodeId {0} is not present in the map",
                nodeId);
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            WriteWarning(
                TraceSecurityPrincipalsProvider,
                Root.TraceId,
                "Failed to get NodeEnvMgr for NodeId {0} error {1}",
                nodeId,
                error);
            return error;
        }
    }

    return envMgr->Cleanup(applicationId, cleanupForNode);
}

ErrorCode SecurityPrincipalsProvider::Cleanup(
    std::wstring const & nodeId)
{
    NodeEnvironmentManagerSPtr envMgr;
    auto error = this->envManagerMap_.Remove(nodeId, envMgr);
    if(!error.IsSuccess())
    {
        if(error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceSecurityPrincipalsProvider,
                Root.TraceId,
                "NodeEnvMgr for NodeId {0} is not present in the map, calling cleanup directly",
                nodeId);
            if(HostingConfig::GetConfig().RunAsPolicyEnabled)
            {
                ApplicationPrincipals::CleanupEnvironment(nodeId);
            }
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            WriteWarning(
                TraceSecurityPrincipalsProvider,
                Root.TraceId,
                "Failed to remove NodeEnvMgr for NodeId {0} from map. Error {1}",
                nodeId,
                error);
            return error;
        }
    }
    return envMgr->Cleanup();
}

ErrorCode SecurityPrincipalsProvider::GetSecurityUser(
    wstring const & nodeId,
    wstring const & applicationId,
    wstring const & name,
    __out SecurityUserSPtr & secUser)
{
    NodeEnvironmentManagerSPtr envMgr;
    auto error = this->envManagerMap_.Get(nodeId, envMgr);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceSecurityPrincipalsProvider,
            Root.TraceId,
            "Failed to get NodeEnvMgr for NodeId {0} from map. Error {1}",
            nodeId,
            error);
        return error;
    }
    return envMgr->GetSecurityUser(applicationId, name, secUser);
}

ErrorCode SecurityPrincipalsProvider::GetSecurityGroup(
    wstring const & nodeId,
    wstring const & applicationId,
    wstring const & name,
    __out SecurityGroupSPtr & secGroup)
{
    NodeEnvironmentManagerSPtr envMgr;
    auto error = this->envManagerMap_.Get(nodeId, envMgr);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceSecurityPrincipalsProvider,
            Root.TraceId,
            "Failed to get NodeEnvMgr for NodeId {0} from map. Error {1}",
            nodeId,
            error);
        return error;
    }
    return envMgr->GetSecurityGroup(applicationId, name, secGroup);
}

ErrorCode SecurityPrincipalsProvider::OnOpen()
{
    WriteNoise(
        TraceSecurityPrincipalsProvider,
        Root.TraceId,
        "SecurityPrincipalsProvider opened");
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SecurityPrincipalsProvider::OnClose()
{
    vector<NodeEnvironmentManagerSPtr> envMgrs = envManagerMap_.Close(); 
    for(auto iter = envMgrs.begin(); iter != envMgrs.end(); ++iter)
    {
        (*iter)->Cleanup(true);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void SecurityPrincipalsProvider::OnAbort()
{
    vector<NodeEnvironmentManagerSPtr> envMgrs = envManagerMap_.Close(); 
    for(auto iter = envMgrs.begin(); iter != envMgrs.end(); ++iter)
    {
        (*iter)->Cleanup();
    }
}


