// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace RepairManager
    {
        /// <summary>
        /// This class is used for serialization/deserialization of the UpdateRepairTaskHealthPolicy
        // method parameters between the client and the service replica.
        /// </summary>
        /// <remarks>
        /// For PowerShell and managed clients, the FromPublicApi method of this class is used for conversion from
        /// FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION.
        /// Whereas for REST requests, this class is used directly via the Json serializer/deserializer
        /// in httpgateway by bypassing FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION.
        ///
        /// Below is the format of the REST body when we send the http POST
        /// E.g. if you are trying a rest command of UpdateRepairTaskHealthPolicy via the Fiddler tool
        /// then the Uri would be
        /// http://localhost:19007/$/UpdateRepairTaskHealthPolicy?api-version=1.0
        /// and the POST body would be
        /// 
        /// {
        ///     "Version": "0",
        ///     "TaskId" : "FabricClient/3718597a-87ce-4e94-94b0-041aaeca408c",
        ///     "PerformPreparingHealthCheck": true,
        ///     "PerformRestoringHealthCheck": true
        /// }
        /// 
        /// To verify if POST worked correctly, you could get repair tasks via GET. The Uri is
        /// http://localhost:19007/$/GetRepairTaskList?api-version=1.0
        /// 
        /// To leave a boolean untouched (e.g. PerformRestoringHealthCheck) from its original value, omit it in the Json body
        /// 
        /// Note:
        /// In the above case, 19007 is a port no of the HttpGatewayEndpoint. 
        /// Please check clustermanifest.xml to get the actual number in your case and check to see if this is correctly enabled
        /// 
        /// For SF REST documentation, see https://msdn.microsoft.com/en-us/library/azure/dn707692.aspx
        /// 
        /// </remarks>
        class UpdateRepairTaskHealthPolicyMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            UpdateRepairTaskHealthPolicyMessageBody();

            UpdateRepairTaskHealthPolicyMessageBody(
                RepairScopeIdentifierBaseSPtr const & scope,
                std::wstring const & taskId,
                int64 version,
                std::shared_ptr<bool> const & performPreparingHealthCheckSPtr,
                std::shared_ptr<bool> const & performRestoringHealthCheckSPtr);
            
            __declspec(property(get=get_Scope)) RepairScopeIdentifierBaseConstSPtr Scope;
            __declspec(property(get=get_TaskId)) std::wstring const & TaskId;
            __declspec(property(get=get_Version)) int64 Version;

            // To be consistent with other places, we'll not suffix SPtr to the properties or its backing methods. 
            // Just the member variables.
            __declspec(property(get=get_PerformPreparingHealthCheck)) std::shared_ptr<bool> const & PerformPreparingHealthCheck;
            __declspec(property(get=get_PerformRestoringHealthCheck)) std::shared_ptr<bool> const & PerformRestoringHealthCheck;

            RepairScopeIdentifierBaseConstSPtr get_Scope() const { return scope_; }
            std::wstring const & get_TaskId() const { return taskId_; }
            int64 get_Version() const { return version_; }            
            std::shared_ptr<bool> const & get_PerformPreparingHealthCheck() const { return performPreparingHealthCheckSPtr_; }
            std::shared_ptr<bool> const & get_PerformRestoringHealthCheck() const { return performRestoringHealthCheckSPtr_; }

            void ReplaceScope(RepairScopeIdentifierBaseSPtr const & newScope);

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION const & publicDescription);

            FABRIC_FIELDS_05(scope_, taskId_, version_, performPreparingHealthCheckSPtr_, performRestoringHealthCheckSPtr_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Scope, scope_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::TaskId, taskId_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Version, version_)                
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PerformPreparingHealthCheck, performPreparingHealthCheckSPtr_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PerformRestoringHealthCheck, performRestoringHealthCheckSPtr_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            RepairScopeIdentifierBaseSPtr scope_;
            std::wstring taskId_;
            int64 version_;

            /// <summary>
            /// The shared_ptr is to allow 3 states for this value. If the user wishes to not change the existing state, they
            /// pass in null. Otherwise, a true/false value updates the current state.
            /// Similarly with performRestoringHealthCheckSPtr_
            /// </summary>
            std::shared_ptr<bool> performPreparingHealthCheckSPtr_;
            std::shared_ptr<bool> performRestoringHealthCheckSPtr_;
        };
    }
}
