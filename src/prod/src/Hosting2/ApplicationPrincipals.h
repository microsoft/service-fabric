// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationPrincipals :
        public Common::ComponentRoot,
        public Common::StateMachine,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationPrincipals)

            using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        STATEMACHINE_STATES_11(Inactive, Opening, Opened, RetryScheduling, RetryScheduled, UserCreationRetrying, UpdatingPrincipal, GettingPrincipal, Closing, Closed, Failed);
        STATEMACHINE_TRANSITION(Opening, Inactive);
        STATEMACHINE_TRANSITION(Opened, Opening | UserCreationRetrying | UpdatingPrincipal | GettingPrincipal);
        STATEMACHINE_TRANSITION(RetryScheduling, Opening | UserCreationRetrying);
        STATEMACHINE_TRANSITION(RetryScheduled, RetryScheduling | GettingPrincipal);
        STATEMACHINE_TRANSITION(UserCreationRetrying, RetryScheduled);
        STATEMACHINE_TRANSITION(UpdatingPrincipal, Opened);
        STATEMACHINE_TRANSITION(GettingPrincipal, Opened | RetryScheduled);
        STATEMACHINE_TRANSITION(Closing, Opened | RetryScheduled);
        STATEMACHINE_TRANSITION(Closed, Closing);
        STATEMACHINE_TRANSITION(Failed, Opening | Closing);
        STATEMACHINE_ABORTED_TRANSITION(Opened | RetryScheduled | Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted | Closed);

    public:
        explicit ApplicationPrincipals(ConfigureSecurityPrincipalRequest && request);

        virtual ~ApplicationPrincipals();

        __declspec(property(get=get_ApplicationId)) std::wstring ApplicationId;
        std::wstring const & get_ApplicationId() const { return applicationId_; }

        __declspec(property(get = get_ApplicationGroupName)) std::wstring ApplicationGroupName;
        std::wstring const & get_ApplicationGroupName() const { return applicationGroupName_; }

        Common::ErrorCode Open();

        Common::ErrorCode Close();

        // Update security principals based on new principal request information.
        Common::ErrorCode UpdateApplicationPrincipals(ConfigureSecurityPrincipalRequest && request);

        // Get current security principal.
        // The state transitions back to previous state at the end of the method.
        Common::ErrorCode GetSecurityPrincipalInformation(
            __inout std::vector<SecurityPrincipalInformationSPtr> & principalInformation);

        Common::ErrorCode GetSecurityUser(
            std::wstring const & name,
            __out SecurityUserSPtr & secUser);

        Common::ErrorCode GetSecurityGroup(
            std::wstring const & name,
            __out SecurityGroupSPtr & secGroup);
        // Cleanup of all princiapls belonging to specified NodeId and ApplicationId (if not empty)
        static void CleanupEnvironment(std::wstring const& nodeId = L"", std::wstring const& appId = L"");

        Common::ErrorCode CloseApplicationPrincipals(bool removePrincipals);

        static std::wstring GetApplicationLocalGroupName(std::wstring const & nodeId, ULONG applicationCounter);

        static Common::GlobalWString WinFabAplicationGroupCommentPrefix;
        static Common::GlobalWString WinFabAplicationLocalGroupComment;
        static Common::GlobalWString GroupNamePrefix;

    protected:
        virtual void OnAbort();

    private:
        // Setup of principals specified as part of the description
        Common::ErrorCode SetupApplicationPrincipals(
            __inout std::vector<ServiceModel::SecurityUserDescription> & failedUsers);

        // Cleanup of principals created during setup
        void CleanupApplicationPrincipals();

        Common::ErrorCode ProcessUser(ServiceModel::SecurityUserDescription & securirtyUser);

        Common::ErrorCode TransitionToGettingPrincipalPreviousState(uint64 const previousState);

        // Gets the security principal information without any state checks.
        // State validation must be performed by the caller.
        Common::ErrorCode InternalGetSecurityPrincipalInformation(
            __inout std::vector<SecurityPrincipalInformationSPtr> & principalInformation);

        // Checks that transition to targetState was successful.
        // If not, it will log an warning and, if trasitionToFailedOnError is specified, transitions to Failed state.
        bool CheckTransitionError(Common::ErrorCode const & error, std::wstring const & targetState, bool transitionToFailedOnError);

        Common::ErrorCode CreateOrLoadSecurityPrincipal(
            ISecurityPrincipalSPtr const& principal,
            bool isSecurityUser,
            std::wstring const& principalName,
            ULONG applicationPackageCounter);
        Common::ErrorCode DeleteOrUnloadSecurityPrincipal(ISecurityPrincipalSPtr const& principal, bool isSecurityUser);

        Common::ErrorCode ConfigureApplicationGroupAndMemberShip();

        static std::wstring CreateComment(std::wstring const& actualAccountName, std::wstring const& nodeIds, std::wstring const& applicationId);
        Common::ErrorCode AddNodeToComment(std::wstring & comment);
        static Common::ErrorCode RemoveNodeFromComment(std::wstring & comment, std::wstring const& nodeId, bool const removeAll);

        static void CleanupGroups(std::wstring const& nodeId, std::wstring const& appId);
        static void CleanupUsers(std::wstring const& nodeId, std::wstring const& appId);
        static bool IsPrincipalOwned(
            std::wstring const& comment,
            std::wstring const& nodeId,
            std::wstring const& appId,
            bool removeAll /*Indicates if all of entries for the given NodeId should be considered for computing 'isLastNodeInComment'*/,
            bool & isLastNodeInComment);

        Common::ErrorCode ScheduleUserCreation(std::vector<ServiceModel::SecurityUserDescription> && failedUsers);
        void RetryUserCreation(std::vector<ServiceModel::SecurityUserDescription> failedUsers);

        Common::TimerSPtr userCreationRetryTimer_;
        std::wstring const applicationId_;

        std::wstring const nodeId_;
        ULONG applicationPackageCounter_;

        std::wstring applicationGroupName_;

        ServiceModel::PrincipalsDescription principalsDescription_; // track principal config requests
        std::vector<SecurityGroupSPtr> groups_;
        std::vector<SecurityUserSPtr> users_;
        bool removePrincipals_;
        int allowedUserFailureCount_;

        std::map<std::wstring, std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> applicationGroupNameMap_;
    };
}
