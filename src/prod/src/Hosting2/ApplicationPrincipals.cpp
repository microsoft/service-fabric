// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceApplicationPrincipals("ApplicationPrincipals");

GlobalWString ApplicationPrincipals::WinFabAplicationGroupCommentPrefix = make_global<wstring>(L"WinFabApplication");
GlobalWString ApplicationPrincipals::WinFabAplicationLocalGroupComment = make_global<wstring>(L"WinFabApplication-LocalGroup");
GlobalWString ApplicationPrincipals::GroupNamePrefix = make_global<wstring>(L"WF-App-");

ApplicationPrincipals::ApplicationPrincipals(ConfigureSecurityPrincipalRequest && request)
    : StateMachine(Inactive),
    applicationId_(request.TakeApplicationId()),
    nodeId_(request.TakeNodeId()),
    principalsDescription_(request.TakePrincipalsDescription()),
    applicationPackageCounter_(request.ApplicationPackageCounter),
    groups_(),
    users_(),
    removePrincipals_(true),
    applicationGroupName_(ApplicationPrincipals::GetApplicationLocalGroupName(nodeId_, request.ApplicationPackageCounter)),
    allowedUserFailureCount_(request.AllowedUserCreationFailureCount),
    userCreationRetryTimer_()
{
}

ApplicationPrincipals::~ApplicationPrincipals()
{
}

ErrorCode ApplicationPrincipals::SetupApplicationPrincipals(
    __inout vector<SecurityUserDescription> & failedUsers)
{
    WriteInfo(
        TraceApplicationPrincipals,
        "{0}: SetupApplicationPrincipals for PrincipalsDescription {1}: creating new users and groups",
        applicationId_,
        principalsDescription_);

    bool configureAppGroup = false;
    for (auto it = principalsDescription_.Groups.begin(); it != principalsDescription_.Groups.end(); ++it)
    {
        if(HostingConfig().GetConfig().NTLMAuthenticationEnabled && !it->NTLMAuthenticationPolicy.IsEnabled)
        {
            it->NTLMAuthenticationPolicy.IsEnabled = true;
        }

        SecurityGroupSPtr group = make_shared<SecurityGroup>(applicationId_, *it);
        auto error = CreateOrLoadSecurityPrincipal(group, false, (*it).Name, applicationPackageCounter_);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceApplicationPrincipals,
                "{0}: CreateGroup for {1} failed with {2}",
                applicationId_,
                group->Name,
                error);
            return error;
        }

        applicationGroupNameMap_.insert(make_pair(group->Name, group->AccountName));
        groups_.push_back(move(group));
    }

    for (auto it = principalsDescription_.Users.begin(); it != principalsDescription_.Users.end(); ++it)
    {
        if (it->AccountType == SecurityPrincipalAccountType::LocalUser)
        {
            configureAppGroup = true;
        }

        auto processUserError = this->ProcessUser(*it);
        if(!processUserError.IsSuccess())
        {
            // Currently CertificateNotFound is the only error on
            // which we will retry user creation in the background
            if(processUserError.IsError(ErrorCodeValue::CertificateNotFound) &&
                failedUsers.size() < allowedUserFailureCount_)
            {
                WriteInfo(
                    TraceApplicationPrincipals,
                    "{0}: ProcessUser for {1} failed with {2}",
                    applicationId_,
                    it->Name,
                    processUserError);
                failedUsers.push_back(*it);
            }
            else
            {
                return processUserError;
            }
        }
    }

    if (configureAppGroup)
    {
        return ConfigureApplicationGroupAndMemberShip();
    }

    return ErrorCode::Success();
}

ErrorCode ApplicationPrincipals::ScheduleUserCreation(std::vector<ServiceModel::SecurityUserDescription> && failedUsers)
{
    auto error = this->TransitionToRetryScheduling();
    if (!CheckTransitionError(error, L"RetryScheduling", true))
    {
        return error;
    }

    auto root = this->CreateComponentRoot();
    userCreationRetryTimer_ = Timer::Create("Hosting.RetryUserCreation", [this, root, failedUsers](TimerSPtr const & timer)
    {
        timer->Cancel();
        this->RetryUserCreation(failedUsers);
    },
        false);

    error = this->TransitionToRetryScheduled();
    if (!CheckTransitionError(error, L"RetryScheduled", true))
    {
        return error;
    }

    auto delay = HostingConfig::GetConfig().FileStoreServiceUserCreationRetryTimeout;
    WriteInfo(
        TraceApplicationPrincipals,
        "Scheduling RetryUserCreation for application {0}, nodeId {1} to run after {2}",
        applicationId_,
        nodeId_,
        delay);
    userCreationRetryTimer_->Change(delay);

    return ErrorCodeValue::Success;
}

void ApplicationPrincipals::RetryUserCreation(std::vector<ServiceModel::SecurityUserDescription> failedUsers)
{
    auto error = this->TransitionToUserCreationRetrying();
    if (!CheckTransitionError(error, L"Updating", true))
    {
        return;
    }

    bool configureAppGroup = false;
    vector<SecurityUserDescription> newFailedUsers;
    for(auto it = failedUsers.begin(); it != failedUsers.end(); ++it)
    {
        auto processUserError = this->ProcessUser(*it);

        WriteTrace(
            processUserError.ToLogLevel(),
            TraceApplicationPrincipals,
            "{0}: Retry of ProcessUser for {1} completed with {2}",
            applicationId_,
            it->Name,
            processUserError);

        if(processUserError.IsSuccess())
        {
            configureAppGroup = true;
        }
        else
        {
            newFailedUsers.push_back(*it);
        }
    }

    if(configureAppGroup)
    {
        error = this->ConfigureApplicationGroupAndMemberShip();
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceApplicationPrincipals,
                "{0}: ConfigureApplicationGroupAndMemberShip during retry on nodeId {1} completed with {2}. Scheduling retry of this operation.",
                applicationId_,
                nodeId_,
                error);
        }
    }

    if(newFailedUsers.size() > 0 || !error.IsSuccess())
    {
        this->ScheduleUserCreation(move(newFailedUsers));
    }
    else
    {
        WriteInfo(
            TraceApplicationPrincipals,
            "{0}: Retry of failed users on nodeId {1} completed successfully. Transitioning to Opened state.",
            applicationId_,
            nodeId_);

        error = this->TransitionToOpened();
        CheckTransitionError(error, L"Opened", true);
    }
}

Common::ErrorCode ApplicationPrincipals::UpdateApplicationPrincipals(
    ConfigureSecurityPrincipalRequest && request)
{
    // step 1: argument validation
    if (!request.UpdateExisting)
    {
        WriteWarning(
            TraceApplicationPrincipals,
            "AppId: {0}: NodeId {1}: UpdateApplicationPrincipals called but UpdateExisting not set in request: InvalidState",
            applicationId_,
            nodeId_);
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    if (request.ApplicationId != applicationId_ || request.NodeId != nodeId_)
    {
        WriteWarning(
            TraceApplicationPrincipals,
            "UpdateApplicationPrincipals: incoming request is for different appid/nodeid. Current {0}/{1}, incoming {2}/{3}. Invalid argument",
            applicationId_,
            nodeId_,
            request.ApplicationId,
            request.NodeId);
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    auto principals = request.TakePrincipalsDescription();

    if (principals.Groups.size() != principalsDescription_.Groups.size())
    {
        WriteWarning(
            TraceApplicationPrincipals,
            "AppId: {0}: NodeId {1}: UpdateApplicationPrincipals: principal groups do not match size, current {2}, new {3}",
            applicationId_,
            nodeId_,
            principals.Groups.size(),
            principalsDescription_.Groups.size());
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    for (auto it = principals.Groups.begin(); it != principals.Groups.end(); ++it)
    {
        auto const & newGroup = *it;
        auto it2 = find_if(
            principalsDescription_.Groups.begin(),
            principalsDescription_.Groups.end(),
            [&newGroup](SecurityGroupDescription const & group) { return group.Name == newGroup.Name; });
        if (it2 == principalsDescription_.Groups.end())
        {
            WriteWarning(
                TraceApplicationPrincipals,
                "AppId: {0}: Group {1} not found in existing groups",
                applicationId_,
                nodeId_,
                it->Name);
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }
    }

    // Step 2: transition to UpdatePrincipal
    auto error = this->TransitionToUpdatingPrincipal();
    if (!error.IsSuccess())
    {
        return error;
    }

    // Step 3: update principals
    bool configureAppGroup = false;
    for (auto it = principals.Users.begin(); it != principals.Users.end(); ++it)
    {
        // Check that the user doesn't already exist
        auto & name = it->Name;
        auto itCrt = find_if(principalsDescription_.Users.begin(), principalsDescription_.Users.end(), [&name](SecurityUserDescription const & user) { return user.Name == name; });
        if (itCrt != principalsDescription_.Users.end())
        {
            WriteInfo(
                TraceApplicationPrincipals,
                "AppId: {0} NodeId {1}: skip setup user {2}, since it's already created",
                applicationId_,
                nodeId_,
                name);
            continue;
        }

        // Create the user and add it to the local users_, to be used to configure the group membership
        auto processUserError = this->ProcessUser(*it);
       
        if (processUserError.IsSuccess())
        {
            WriteInfo(
                TraceApplicationPrincipals,
                "AppId {0} NodeId {1}: UpdateApplicationPrincipals: ProcessUser for {2} completed with {3}",
                applicationId_,
                nodeId_,
                it->Name,
                processUserError);
            configureAppGroup = true;
            principalsDescription_.Users.push_back(*it);
        }
        else
        {
            WriteError(
                TraceApplicationPrincipals,
                "Unexpected error. AppId {0} NodeId {1}: UpdateApplicationPrincipals: ProcessUser for {2} failed with {3}",
                applicationId_,
                nodeId_,
                it->Name,
                processUserError);
            this->TransitionToFailed();
            return ErrorCode(ErrorCodeValue::ApplicationPrincipalAbortableError);
        }
    }

    if (configureAppGroup)
    {
        auto configureGroupMembershipError = this->ConfigureApplicationGroupAndMemberShip();
        if (!configureGroupMembershipError.IsSuccess())
        {
            WriteWarning(
                TraceApplicationPrincipals,
                "{0}: UpdateApplicationPrincipals: ConfigureApplicationGroupAndMemberShip on nodeId {1} completed with {2}, transition to failed.",
                applicationId_,
                nodeId_,
                configureGroupMembershipError);
            this->TransitionToFailed();
            return ErrorCode(ErrorCodeValue::ApplicationPrincipalAbortableError);
        }
    }

    // Done, transition to Opened
    error = this->TransitionToOpened();
    if (!CheckTransitionError(error, L"Opened", true))
    {
        return ErrorCode(ErrorCodeValue::ApplicationPrincipalAbortableError);
    }

    return error;
}

Common::ErrorCode ApplicationPrincipals::GetSecurityPrincipalInformation(
    __inout vector<SecurityPrincipalInformationSPtr> & principalInformation)
{
    uint64 previousState;
    auto error = this->TransitionToGettingPrincipal(previousState);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = InternalGetSecurityPrincipalInformation(principalInformation);

    // Transition to previous state
    auto transitionError = TransitionToGettingPrincipalPreviousState(previousState);
    if (!transitionError.IsSuccess() && error.IsSuccess())
    {
        error.Overwrite(move(transitionError));
    }

    return error;
}

Common::ErrorCode ApplicationPrincipals::InternalGetSecurityPrincipalInformation(
    __inout vector<SecurityPrincipalInformationSPtr> & principalInformation)
{
    ErrorCode error(ErrorCodeValue::Success);
    for (auto iter = users_.begin(); iter != users_.end(); ++iter)
    {
        wstring sid;
        error = (*iter)->SidObj->ToString(sid);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceApplicationPrincipals,
                nodeId_,
                "Failed to retrieve sid for user {0}. Error {1}",
                (*iter)->Name,
                error);
            break;
        }

        bool isLocal = (*iter)->AccountType == ServiceModel::SecurityPrincipalAccountType::LocalUser;
        auto principalInfo = make_shared<SecurityPrincipalInformation>((*iter)->Name, (*iter)->Name, sid, isLocal);
        principalInformation.push_back(move(principalInfo));
    }

    if (error.IsSuccess())
    {
        for (auto it = groups_.begin(); it != groups_.end(); ++it)
        {
            wstring sid;
            error = (*it)->SidObj->ToString(sid);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceApplicationPrincipals,
                    nodeId_,
                    "Failed to retrieve sid for group {0}. Error {1}",
                    (*it)->Name,
                    error);
                break;
            }

            auto principalInfo = make_shared<SecurityPrincipalInformation>((*it)->Name, (*it)->AccountName, sid, false);
            principalInformation.push_back(move(principalInfo));
        }
    }

    return error;
}

Common::ErrorCode ApplicationPrincipals::TransitionToGettingPrincipalPreviousState(uint64 const previousState)
{
    ErrorCode transitionError;
    if (previousState == Opened)
    {
        transitionError = this->TransitionToOpened();
    }
    else if (previousState == RetryScheduled)
    {
        transitionError = this->TransitionToRetryScheduled();
    }
    else
    {
        Assert::CodingError("GetSecurityPrincipalInformation: Invalid previous state: {0}", StateToString(previousState));
    }

    CheckTransitionError(transitionError, StateToString(previousState), true);
    return transitionError;
}

bool ApplicationPrincipals::CheckTransitionError(
    Common::ErrorCode const & error,
    std::wstring const & targetState,
    bool transitionToFailedOnError)
{
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceApplicationPrincipals,
            "AppId:{0} in NodeId:{1}: transition to state {2} returned error {3}",
            applicationId_,
            nodeId_,
            targetState,
            error);
        if (transitionToFailedOnError)
        {
            this->TransitionToFailed();
        }

        return false;
    }

    return true;
}

ErrorCode ApplicationPrincipals::ProcessUser(SecurityUserDescription & securityUser)
{
    if(HostingConfig().GetConfig().NTLMAuthenticationEnabled && !securityUser.NTLMAuthenticationPolicy.IsEnabled)
    {
        securityUser.NTLMAuthenticationPolicy.IsEnabled = true;
        securityUser.NTLMAuthenticationPolicy.PasswordSecret =
            HostingConfig().GetConfig().NTLMAuthenticationPasswordSecretEntry.IsEncrypted() ?
            HostingConfig().GetConfig().NTLMAuthenticationPasswordSecretEntry.GetEncryptedValue() : HostingConfig().GetConfig().NTLMAuthenticationPasswordSecret.GetPlaintext();
        securityUser.NTLMAuthenticationPolicy.IsPasswordSecretEncrypted = HostingConfig().GetConfig().NTLMAuthenticationPasswordSecretEntry.IsEncrypted();
    }

    std::vector<wstring> actualParentApplicationGroupNames;
    for(auto groupNameIter = securityUser.ParentApplicationGroups.begin(); groupNameIter != securityUser.ParentApplicationGroups.end(); ++groupNameIter)
    {
        auto mapIter = applicationGroupNameMap_.find(*groupNameIter);
#if !defined(PLATFORM_UNIX)
        ASSERT_IF(mapIter == applicationGroupNameMap_.end(), "{0} group nanme not found in applicationGroupNameMap", *groupNameIter);
        actualParentApplicationGroupNames.push_back(mapIter->second);
#else
        actualParentApplicationGroupNames.push_back(mapIter == applicationGroupNameMap_.end()? *groupNameIter : mapIter->second);
#endif
    }

    // Replace userDescription.ParentApplicationGroup names with the actual names
    SecurityUserDescription userDescription = securityUser;
    userDescription.ParentApplicationGroups = actualParentApplicationGroupNames;
    SecurityUserSPtr user = SecurityUser::CreateSecurityUser(applicationId_, userDescription);
    if(user->AccountType == SecurityPrincipalAccountType::LocalUser)
    {
        auto localUser = shared_ptr<ISecurityPrincipal>(dynamic_pointer_cast<ISecurityPrincipal>(user));

        auto error = CreateOrLoadSecurityPrincipal(localUser, true, userDescription.Name, applicationPackageCounter_);
        if(!error.IsSuccess())
        {
            return error;
        }
    }
    else
    {
        auto error = user->ConfigureAccount();
        if(!error.IsSuccess())
        {
            return error;
        }

        error = user->LoadAccount(user->AccountName);
        if(!error.IsSuccess())
        {
            return error;
        }
    }

    users_.push_back(move(user));

    return ErrorCodeValue::Success;
}

ErrorCode ApplicationPrincipals::CreateOrLoadSecurityPrincipal(
    ISecurityPrincipalSPtr const& principal,
    bool isSecurityUser,
    std::wstring const& principalName,
    ULONG applicationPackageCounter)
{
    if(principal->NTLMAuthenticationPolicy.IsEnabled)
    {
        bool canSkipAddCounter = (applicationPackageCounter == ApplicationIdentifier::FabricSystemAppId->ApplicationNumber);
        wstring accountName = AccountHelper::GetAccountName(principalName, applicationPackageCounter, canSkipAddCounter);
        MutexHandleUPtr mutex;
        wstring mutexName = PrincipalsProvider::GlobalMutexName;
#if !defined(PLATFORM_UNIX)
        mutexName.append(accountName);
#else
        mutexName.append(L"User");
#endif
        auto error = SecurityPrincipalHelper::AcquireMutex(mutexName, mutex);
        if (!error.IsSuccess())
        {
            return error;
        }

        wstring comment;
        error = isSecurityUser ? SecurityPrincipalHelper::GetUserComment(accountName, comment) : SecurityPrincipalHelper::GetGroupComment(accountName, comment);
        if(error.IsSuccess())
        {
            WriteInfo(
                TraceApplicationPrincipals,
                nodeId_,
                "{0}: CreateOrLoadSecurityPrincipal.GetComment for principal {1} successful",
                applicationId_,
                accountName);
            error = AddNodeToComment(comment);
            if(!error.IsSuccess())
            {
                if (error.IsError(ErrorCodeValue::InvalidState))
                {
                    auto err = SecurityPrincipalHelper::DeleteUserAccount(accountName);
                    WriteInfo(
                        TraceApplicationPrincipals,
                        nodeId_,
                        "{0}: CreateOrLoadSecurityPrincipal.GetComment for principal {1} failed with error {2} cleanup of account returned {3}",
                        applicationId_,
                        accountName,
                        error,
                        err);
                }
                return error;
            }

            error = isSecurityUser ? SecurityPrincipalHelper::UpdateUserComment(accountName, comment) : SecurityPrincipalHelper::UpdateGroupComment(accountName, comment);

            if(error.IsSuccess())
            {
                error = principal->LoadAccount(accountName);
                WriteInfo(
                    TraceApplicationPrincipals,
                    nodeId_,
                    "{0}: LoadAccount for {1}:{2} completed with error {3}",
                    applicationId_,
                    accountName,
                    comment,
                    error);
                ASSERT_IF(error.IsError(ErrorCodeValue::NotFound), "LoadExistingAccount cannot fail with NotFound");
                return error;
            }
            else
            {
                WriteInfo(
                    TraceApplicationPrincipals,
                    nodeId_,
                    "{0}: UpdateComment for {1} failed with {2}",
                    applicationId_,
                    accountName,
                    error);
                ASSERT_IF(error.IsError(ErrorCodeValue::NotFound), "UpdateComment cannot return NotFound error for name {0}", accountName);
                return error;
            }
        }
        else if(error.IsError(ErrorCodeValue::NotFound))
        {
            comment = ApplicationPrincipals::CreateComment(principalName, nodeId_, applicationId_);
            error = principal->CreateAccountwithName(accountName, comment);
            WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceApplicationPrincipals,
                nodeId_,
                "{0}: CreateAccountwithName with comment '{1}' and account {2} completed with {3}",
                applicationId_,
                comment,
                accountName,
                error);
            return error;
        }
        else
        {
            WriteWarning(
                TraceApplicationPrincipals,
                nodeId_,
                "{0}: GetComment for principal {1} failed with {2}",
                applicationId_,
                accountName,
                error);
            return error;
        }
    }
    else
    {
        wstring comment = ApplicationPrincipals::CreateComment(principalName, nodeId_, applicationId_);
        auto error = principal->CreateAccountwithRandomName(comment);
        WriteInfo(
            TraceApplicationPrincipals,
            nodeId_,
            "{0}: CreateAccountwithRandomName for {1} with comment {2} completed with {3}",
            applicationId_,
            principalName,
            comment,
            error);
        return error;
    }
}

void ApplicationPrincipals::CleanupApplicationPrincipals()
{
    WriteInfo(
        TraceApplicationPrincipals,
        nodeId_,
        "{0}: CleanupApplicationPrincipals invoked",
        applicationId_);

    if (!removePrincipals_)
    {
        return;
    }


    ErrorCode error;
    for (auto it = groups_.begin(); it != groups_.end(); ++it)
    {
        error = DeleteOrUnloadSecurityPrincipal(*it, false);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceApplicationPrincipals,
                nodeId_,
                "{0}: DeleteGroup {1} failed with {2}",
                applicationId_,
                (*it)->AccountName,
                error);
        }
    }

    for (auto it = users_.begin(); it != users_.end(); ++it)
    {
        if((*it)->AccountType == SecurityPrincipalAccountType::LocalUser)
        {
            auto localUser = shared_ptr<ISecurityPrincipal>(dynamic_pointer_cast<ISecurityPrincipal>(*it));

            error = DeleteOrUnloadSecurityPrincipal(localUser, true);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceApplicationPrincipals,
                    nodeId_,
                    "{0}: DeleteOrUnloadSecurityUser {1} failed with {2}",
                    applicationId_,
                    (*it)->AccountName,
                    error);
            }
        }
    }
    users_.clear();
    groups_.clear();

}

ErrorCode ApplicationPrincipals::DeleteOrUnloadSecurityPrincipal(ISecurityPrincipalSPtr const& principal, bool isSecurityUser)
{
    ErrorCode error;
    if(principal->NTLMAuthenticationPolicy.IsEnabled)
    {
        MutexHandleUPtr mutex;
        wstring mutexName = PrincipalsProvider::GlobalMutexName;
        mutexName.append(principal->AccountName);
        error = SecurityPrincipalHelper::AcquireMutex(mutexName, mutex);
        if (!error.IsSuccess())
        {
            return error;
        }

        wstring comment;
        error = isSecurityUser ? SecurityPrincipalHelper::GetUserComment(principal->AccountName, comment) : SecurityPrincipalHelper::GetGroupComment(principal->AccountName, comment);
        if(error.IsSuccess())
        {
            WriteInfo(
                TraceApplicationPrincipals,
                nodeId_,
                "{0}: DeleteOrUnloadSecurityPrincipal.GetComment for principal {1} successful",
                applicationId_,
                principal->AccountName);
            bool isLastNodeInComment = false;
            ASSERT_IFNOT(
                IsPrincipalOwned(comment, nodeId_, applicationId_, false /*removeAll*/, isLastNodeInComment),
                "Comment '{0}' for user '{1}' should have nodeId '{2}'",
                comment,
                principal->AccountName,
                nodeId_);
            if(isLastNodeInComment)
            {
                error = principal->DeleteAccount();
                WriteInfo(
                    TraceApplicationPrincipals,
                    nodeId_,
                    "{0}: DeleteAccount {1} completed with {2}",
                    applicationId_,
                    principal->AccountName,
                    error);
                return error;
            }
            else
            {
                RemoveNodeFromComment(comment, nodeId_, false /*removeAll*/);
                error = isSecurityUser ? SecurityPrincipalHelper::UpdateUserComment(principal->AccountName, comment) : SecurityPrincipalHelper::UpdateGroupComment(principal->AccountName, comment);
                WriteInfo(
                    TraceApplicationPrincipals,
                    nodeId_,
                    "{0}: SecurityPrincipalHelper::UpdateComment '{1}' during unload {2} completed with {3}",
                    applicationId_,
                    comment,
                    principal->AccountName,
                    error);
                principal->UnloadAccount();
                return error;
            }
        }
        else
        {
            ASSERT_IF(error.IsError(ErrorCodeValue::NotFound), "GetUserComment for user '{0}' cannot fail with NotFound", principal->AccountName);
            WriteInfo(
                TraceApplicationPrincipals,
                nodeId_,
                "{0}: DeleteOrUnloadSecurityPrincipal.GetComment for principal {1} failed with {2}",
                applicationId_,
                principal->AccountName,
                error);
            return error;
        }
    }
    else
    {
        error = principal->DeleteAccount();
        WriteInfo(
            TraceApplicationPrincipals,
            nodeId_,
            "{0}: DeleteAccount {1} completed with {2}",
            applicationId_,
            principal->AccountName,
            error);
        return error;
    }
}

void ApplicationPrincipals::CleanupGroups(std::wstring const& nodeId, std::wstring const& appId)
{
#if !defined(PLATFORM_UNIX)
    ErrorCode error;

    LPLOCALGROUP_INFO_1 pBuff = NULL;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;
    DWORD_PTR resumeHandle = 0;
    NET_API_STATUS status = ::NetLocalGroupEnum(
        NULL /*local server*/,
        1 /*LOCALGROUP_INFO level*/,
        (LPBYTE *) &pBuff,
        MAX_PREFERRED_LENGTH,
        &entriesRead,
        &totalEntries,
        &resumeHandle /*resume handle*/);
    if (status == NERR_Success)
    {
        LPLOCALGROUP_INFO_1 p;
        if ((p = pBuff) != NULL)
        {
            for (DWORD i = 0; i < entriesRead; i++)
            {
                wstring comment(p->lgrpi1_comment);
                wstring name(p->lgrpi1_name);
                bool isLastNodeInComment = false;

                MutexHandleUPtr mutex;
                wstring mutexName = PrincipalsProvider::GlobalMutexName;
                mutexName.append(name);
                error = SecurityPrincipalHelper::AcquireMutex(mutexName, mutex);
                if (error.IsSuccess())
                {
                    if(ApplicationPrincipals::IsPrincipalOwned(comment, nodeId, appId, false /*removeAll*/, isLastNodeInComment))
                    {
                        if(isLastNodeInComment)
                        {
                            error = SecurityPrincipalHelper::DeleteGroupAccount(name);
                            if(!error.IsSuccess())
                            {
                                WriteWarning(
                                    TraceApplicationPrincipals,
                                    nodeId,
                                    "{0}: SecurityPrincipalHelper::DeleteGroupAccount {1} failed with {2}",
                                    appId,
                                    name,
                                    error);
                            }
                        }
                        else
                        {
                            RemoveNodeFromComment(comment, nodeId, false /*removeAll*/);
                            error = SecurityPrincipalHelper::UpdateGroupComment(name, comment);
                            if(!error.IsSuccess())
                            {
                                WriteWarning(
                                    TraceApplicationPrincipals,
                                    nodeId,
                                    "{0}: SecurityPrincipalHelper::UpdateGroupComment {1} failed with {2}",
                                    appId,
                                    name,
                                    error);
                            }
                        }
                    }
                }

                ++p;
            }
        }
    }
    else
    {
        error = ErrorCode::FromWin32Error(status);
        WriteWarning(
            TraceApplicationPrincipals,
            nodeId,
            "{0}: NetLocalGroupEnum failed with {1}",
            appId,
            error);
    }

    if (pBuff != NULL)
    {
        NetApiBufferFree(pBuff);
    }
#endif
}

void ApplicationPrincipals::CleanupUsers(std::wstring const& nodeId, std::wstring const& appId)
{
#if !defined(PLATFORM_UNIX)
    ErrorCode error;

    PNET_DISPLAY_USER pBuff = NULL;
    PNET_DISPLAY_USER p;

    DWORD i = 0;
    DWORD entriesRequested = 1000;
    DWORD entryCount;
    DWORD status;
    do
    {
        status = ::NetQueryDisplayInformation(
            NULL /*local server*/,
            1 /*level - user*/,
            i,
            entriesRequested,
            MAX_PREFERRED_LENGTH,
            &entryCount,
            (PVOID*) &pBuff);

        if ((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA))
        {
            p = pBuff;
            for(; entryCount > 0; entryCount--)
            {
                wstring comment(p->usri1_comment);
                wstring name(p->usri1_name);
                MutexHandleUPtr mutex;
                wstring mutexName = PrincipalsProvider::GlobalMutexName;
                mutexName.append(name);
                error = SecurityPrincipalHelper::AcquireMutex(mutexName, mutex);
                if (error.IsSuccess())
                {
                    bool isLastNodeInComment = false;
                    if(ApplicationPrincipals::IsPrincipalOwned(comment, nodeId, appId, appId.empty() /*Remove all of specified NodeIds if appId is empty*/, isLastNodeInComment))
                    {
                        if(isLastNodeInComment)
                        {
                            error = SecurityPrincipalHelper::DeleteUserAccountIgnoreDeleteProfileError(name);
                            if(!error.IsSuccess())
                            {
                                WriteWarning(
                                    TraceApplicationPrincipals,
                                    nodeId,
                                    "{0}: SecurityPrincipalHelper::DeleteUserAccountIgnoreDeleteProfileError {1} failed with {2}",
                                    appId,
                                    name,
                                    error);
                            }
                        }
                        else
                        {
                            RemoveNodeFromComment(comment, nodeId, appId.empty() /*Remove all of specified NodeIds if appId is empty*/);
                            error = SecurityPrincipalHelper::UpdateUserComment(name, comment);
                            if(!error.IsSuccess())
                            {
                                WriteWarning(
                                    TraceApplicationPrincipals,
                                    nodeId,
                                    "{0}: SecurityPrincipalHelper::UpdateUserComment {1} failed with {2}",
                                    appId,
                                    name,
                                    error);
                            }
                        }
                    }
                }

                // Set the index if there is more data
                i = p->usri1_next_index;
                p++;
            }

            NetApiBufferFree(pBuff);
        }
        else
        {
            error = ErrorCode::FromWin32Error(status);
            WriteWarning(
                TraceApplicationPrincipals,
                nodeId,
                "{0}: NetQueryDisplayInformation failed with {1}",
                appId,
                error);
        }

        // Continue while there is more data.
    } while (status == ERROR_MORE_DATA);
#endif
}

void ApplicationPrincipals::CleanupEnvironment(std::wstring const& nodeId, std::wstring const& appId)
{
    WriteInfo(
        TraceApplicationPrincipals,
        nodeId,
        "{0}: CleanupEnvironment invoked",
        appId);
    ApplicationPrincipals::CleanupGroups(nodeId, appId);
    ApplicationPrincipals::CleanupUsers(nodeId, appId);
}

wstring ApplicationPrincipals::CreateComment(wstring const& actualAccountName, wstring const& nodeIds, wstring const& appId)
{
    wstring comment = *(ApplicationPrincipals::WinFabAplicationGroupCommentPrefix) + L"|" +
        actualAccountName + L"|" +
        nodeIds + L"|" +
        appId;

    return comment;
}

ErrorCode ApplicationPrincipals::AddNodeToComment(wstring & comment)
{
    if(StringUtility::StartsWith<wstring>(comment, ApplicationPrincipals::WinFabAplicationGroupCommentPrefix))
    {
        StringCollection collection;
        StringUtility::Split<wstring>(comment, collection, L"|");
        if(collection.size() == 4)
        {
            StringCollection nodeIds;
            StringUtility::Split<wstring>(collection[2], nodeIds, L",");
            ASSERT_IF(nodeIds.size() == 0, "nodeIds in comment '{0}' cannot be empty", comment);

            ASSERT_IF(collection[3] != applicationId_, "AppId {0} did not match in comment '{1}'", applicationId_, comment);

            // Assert that NodeId is not already present for Non-SystemApplication comments            
            auto nodeIdIter = find(nodeIds.begin(), nodeIds.end(), nodeId_);
            ASSERT_IF(
                !StringUtility::AreEqualCaseInsensitive(applicationId_, ApplicationIdentifier::FabricSystemAppId->ToString()) && nodeIdIter != nodeIds.end(),
                "Current nodeId {0} cannot be in comment '{1}'", nodeId_, comment);

            if(nodeIdIter == nodeIds.end())
            {
                wstring newNodeIds = collection[2] + L"," + nodeId_;
                comment = CreateComment(collection[1], newNodeIds, applicationId_);
            }

            return ErrorCode::Success();
        }
    }

    WriteError(
        TraceApplicationPrincipals,
        nodeId_,
        "{0}: Comment '{1}' is of invalid format. Cleanup of of stale comments required",
        applicationId_,
        comment);
    return ErrorCode(ErrorCodeValue::InvalidState);
}

ErrorCode ApplicationPrincipals::RemoveNodeFromComment(
    wstring & comment,
    wstring const& nodeId,
    bool removeAll /*Indicates if all of entries for the given NodeId should be removed*/)
{
    if(StringUtility::StartsWith<wstring>(comment, ApplicationPrincipals::WinFabAplicationGroupCommentPrefix))
    {
        StringCollection collection;
        StringUtility::Split<wstring>(comment, collection, L"|");
        if(collection.size() == 4)
        {
            StringCollection nodeIds;
            StringUtility::Split<wstring>(collection[2], nodeIds, L",");
            ASSERT_IF(nodeIds.size() < 2, "nodeIds in comment '{0}' cannot be of size less than 2", comment);

            if(removeAll)
            {
                StringCollection remainingNodeIds;
                for(auto nodeIdIter = nodeIds.begin(); nodeIdIter != nodeIds.end(); ++nodeIdIter)
                {
                    if(nodeId == *nodeIdIter)
                    {
                        continue;
                    }

                    remainingNodeIds.push_back(*nodeIdIter);
                }

                nodeIds = remainingNodeIds;
            }
            else
            {
                auto nodeIdIter = find(nodeIds.begin(), nodeIds.end(), nodeId);
                ASSERT_IF(nodeIdIter == nodeIds.end(), "Current nodeId {0} should be in comment '{1}'", nodeId, comment);
                nodeIds.erase(nodeIdIter);
            }

            auto iter = nodeIds.begin();
            wstring newNodeIds = *iter;
            ++iter;
            for(; iter != nodeIds.end(); ++iter)
            {
                newNodeIds = newNodeIds + L"," + *iter;
            }
            comment = CreateComment(collection[1], newNodeIds, collection[3]);
            return ErrorCode::Success();
        }
    }

    WriteError(
        TraceApplicationPrincipals,
        nodeId,
        "Comment '{1}' is of invalid format. Cleanup of of stale comments required",
        comment);
    return ErrorCode(ErrorCodeValue::InvalidState);
}

bool ApplicationPrincipals::IsPrincipalOwned(
    wstring const& comment,
    wstring const& nodeId,
    wstring const& appId,
    bool removeAll /*Indicates if all of entries for the given NodeId should be considered for computing 'isLastNodeInComment'*/,
    bool & isLastNodeInComment)
{
    isLastNodeInComment = false;
    if(StringUtility::StartsWith<wstring>(comment, ApplicationPrincipals::WinFabAplicationGroupCommentPrefix))
    {
        StringCollection collection;
        StringUtility::Split<wstring>(comment, collection, L"|");
        if(collection.size() != 4 || collection[0] != ApplicationPrincipals::WinFabAplicationGroupCommentPrefix)
        {
            return false;
        }

        // If NodeId is empty, then ignore the nodes in the comments
        bool match = nodeId.empty();
        isLastNodeInComment = true;
        if(!nodeId.empty())
        {
            StringCollection nodeIds;
            StringUtility::Split<wstring>(collection[2], nodeIds, L",");
            ASSERT_IF(nodeIds.size() == 0, "nodeIds in comment '{0}' cannot be empty", comment);

            for(auto nodeIdIter = nodeIds.begin(); nodeIdIter != nodeIds.end(); ++nodeIdIter)
            {
                if(nodeId == *nodeIdIter)
                {
                    match = true;
                    if(!removeAll)
                    {
                        isLastNodeInComment = (nodeIds.size() == 1);
                        break;
                    }
                }
                else
                {
                    isLastNodeInComment = false;
                }
            }
        }

        if(match && !appId.empty())
        {
            match = collection[3] == appId ? true : false;
        }

        return match;
    }

    return false;
}

ErrorCode ApplicationPrincipals::GetSecurityUser(wstring const & name, __out SecurityUserSPtr & secUser)
{
    uint64 previousState;
    auto error = this->TransitionToGettingPrincipal(previousState);
    if (!error.IsSuccess())
    {
        return error;
    }

    bool isFound = false;
    for (auto iter = this->users_.begin(); iter != this->users_.end(); ++iter)
    {
        if (StringUtility::AreEqualCaseInsensitive((*iter)->Name, name))
        {
            secUser = *iter;
            isFound = true;
            break;
        }
    }

    error = TransitionToGettingPrincipalPreviousState(previousState);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (!isFound)
    {
        return ErrorCode(ErrorCodeValue::ApplicationPrincipalDoesNotExist);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationPrincipals::GetSecurityGroup(wstring const & name, __out SecurityGroupSPtr & secUser)
{
    uint64 previousState;
    auto error = this->TransitionToGettingPrincipal(previousState);
    if (!error.IsSuccess())
    {
        return error;
    }

    bool isFound = false;
    for (auto iter = this->groups_.begin(); iter != this->groups_.end(); ++iter)
    {
        if (StringUtility::AreEqualCaseInsensitive((*iter)->Name, name))
        {
            secUser = *iter;
            isFound = true;
            break;
        }
    }

    error = TransitionToGettingPrincipalPreviousState(previousState);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (!isFound)
    {
        return ErrorCode(ErrorCodeValue::ApplicationPrincipalDoesNotExist);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationPrincipals::ConfigureApplicationGroupAndMemberShip()
{
    auto error = SecurityPrincipalHelper::CreateGroupAccount(applicationGroupName_, ApplicationPrincipals::WinFabAplicationLocalGroupComment);

    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::AlreadyExists))
    {
        WriteWarning(
            TraceApplicationPrincipals,
            "CreateGroup for application {0}, nodeId {1} returned error {2}",
            applicationId_,
            nodeId_,
            error);
        return error;
    }
    else
    {
        WriteNoise(
            TraceApplicationPrincipals,
            "CreateGroup for application {0}, nodeId {1} returned error {2}",
            applicationId_,
            nodeId_,
            error);
    }

    vector<PSID> members;
    set<wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> uniquemembers;

    for (auto iter = users_.begin(); iter != users_.end(); ++iter)
    {
        if ((*iter)->AccountType == ServiceModel::SecurityPrincipalAccountType::LocalUser)
        {
            wstring sid;
            error = (*iter)->SidObj->ToString(sid);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceApplicationPrincipals,
                    "Convert sid to sidstring failed returned error {0}, appId {1} nodeId {2}",
                    error,
                    applicationId_,
                    nodeId_);
                return error;
            }

            auto it = uniquemembers.find(sid);
            if (it == uniquemembers.end())
            {
                uniquemembers.insert(sid);
                members.push_back((*iter)->SidObj->PSid);
            }
        }
    }

#if defined(PLATFORM_UNIX)
    // sfuser is always in app groups
    Common::SidSPtr sfuser;
    BufferedSid::CreateSPtr(WinNetworkServiceSid, sfuser);
    members.push_back(sfuser->PSid);
#endif

    if (!members.empty())
    {
        error = SecurityPrincipalHelper::SetLocalGroupMembers(applicationGroupName_, members);

        WriteTrace(
            error.ToLogLevel(),
            TraceApplicationPrincipals,
            "SetLocalGroupMembers for application {0}, nodeId {1} returned error {2}",
            applicationId_,
            nodeId_,
            error);
    }

    return error;
}

wstring ApplicationPrincipals::GetApplicationLocalGroupName(wstring const & nodeId, ULONG applicationCounter)
{
    return wformatString("{0}{1}-{2}",
        ApplicationPrincipals::GroupNamePrefix,
        StringUtility::ToWString<ULONG>(applicationCounter),
        nodeId);
}

ErrorCode ApplicationPrincipals::Open()
{
	if (this->GetState() == ApplicationPrincipals::Opened)
	{
		WriteInfo(
			TraceApplicationPrincipals,
			"AppId:{0} in NodeId:{1}: ApplicationPrincipals is already in Opened state. Returning success.",
			applicationId_,
			nodeId_);

		return ErrorCode::Success();
	}

    auto error = this->TransitionToOpening();
    if (!CheckTransitionError(error, L"Opening", false))
    {
        return error;
    }

    vector<SecurityUserDescription> failedUsers;
    error = this->SetupApplicationPrincipals(failedUsers);

    if (error.IsSuccess())
    {
        if (failedUsers.size() > 0)
        {
            error = this->ScheduleUserCreation(move(failedUsers));
        }
        else
        {
            error = this->TransitionToOpened();
        }
    }

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceApplicationPrincipals,
            "AppId:{0} in NodeId:{1}: ApplicationPrincipals::Open() fails. Error is: {2}",
            applicationId_,
            nodeId_,
            error);

        this->TransitionToFailed();
        return ErrorCode(ErrorCodeValue::ApplicationPrincipalAbortableError);
    }

    return error;
}

ErrorCode ApplicationPrincipals::Close()
{
    auto error = this->TransitionToClosing();
    if (!CheckTransitionError(error, L"Closing", false))
    {
        return error;
    }

    if(this->userCreationRetryTimer_)
    {
        this->userCreationRetryTimer_->Cancel();
    }

    this->CleanupApplicationPrincipals();

    error = this->TransitionToClosed();
    if (!CheckTransitionError(error, L"Closed", true))
    {
        return error;
    }

    return ErrorCode::Success();
}

void ApplicationPrincipals::OnAbort()
{
    if(this->userCreationRetryTimer_)
    {
        this->userCreationRetryTimer_->Cancel();
    }

    this->CleanupApplicationPrincipals();
}

ErrorCode ApplicationPrincipals::CloseApplicationPrincipals(bool removePrincipals)
{
    removePrincipals_ = removePrincipals;
    return Close();
}
