// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BAMessage
        {
        public:

            static Common::GlobalWString IpcFailureAction;
            static Common::GlobalWString OperationSuccessAction;

            static Common::GlobalWString RegisterBAPAction;
            static Common::GlobalWString BackupNowAction;
            static Common::GlobalWString BackupCompleteAction;
            static Common::GlobalWString RestoreCompleteAction;
            static Common::GlobalWString GetBackupPolicyAction;
            static Common::GlobalWString GetRestorePointAction;

            static Common::GlobalWString UploadBackupAction;
            static Common::GlobalWString DownloadBackupAction;

            static Common::GlobalWString ForwardToBAPAction;
            static Common::GlobalWString ForwardToBRSAction;
            
            static Common::GlobalWString UpdatePolicyAction;
            
            static BAMessage const & GetRegisterBAP() { return RegisterBAP; };
            static BAMessage const & GetBackupNow() { return BackupNow; };
            static BAMessage const & GetBackupComplete() { return BackupComplete; };

            // Creates a message with the specified body

            template <class TBody>
            static Transport::MessageUPtr GetUserServiceOperationSuccess(TBody const& body)
            {
                return CreateMessage(OperationSuccessAction, body);
            }

            static Transport::MessageUPtr CreateBackupNowMessage(FABRIC_BACKUP_OPERATION_ID operationId, FABRIC_BACKUP_CONFIGURATION const& backupConfiguration, Common::Guid partitionId, Common::NamingUri serviceUri);
            static Transport::MessageUPtr CreateUpdatePolicyMessage(Common::Guid partitionId, Common::NamingUri serviceUri);
            static Transport::MessageUPtr CreateUpdatePolicyMessage(BackupPolicy const& backupPolicy, Common::Guid partitionId, Common::NamingUri serviceUri);
            static Transport::MessageUPtr CreateSuccessReply(Common::ActivityId const&);

            static Transport::MessageUPtr CreateGetPolicyMessage(wstring serviceName, Common::Guid partitionId);
            static Transport::MessageUPtr CreateGetRestorePointMessage(wstring serviceName, Common::Guid partitionId);
            static Transport::MessageUPtr CreateBackupOperationResultMessage(const FABRIC_BACKUP_OPERATION_RESULT& operationResult);
            static Transport::MessageUPtr CreateRestoreOperationResultMessage(const FABRIC_RESTORE_OPERATION_RESULT& operationResult);
            static Transport::MessageUPtr CreateUploadBackupMessage(const FABRIC_BACKUP_UPLOAD_INFO& uploadInfo, const FABRIC_BACKUP_STORE_INFORMATION& storeInfo);
            static Transport::MessageUPtr CreateDownloadBackupMessage(const FABRIC_BACKUP_DOWNLOAD_INFO& downloadInfo, const FABRIC_BACKUP_STORE_INFORMATION& storeInfo);

            static Transport::MessageUPtr CreateIpcFailureMessage(SystemServices::IpcFailureBody const & body, Common::ActivityId const & activityId);

            static Common::ErrorCode ValidateIpcReply(__in Transport::Message &);

            static void WrapForBA(Transport::Message & message);
            static void UnwrapForBA(Transport::Message & message);

        private:

            // Creates a message without a body
            static Transport::MessageUPtr CreateMessageForFabricBRS(std::wstring const &action)
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
                SetBAHeaders(*message, Transport::Actor::BRS, action);

                return std::move(message);
            }

            static Transport::MessageUPtr CreateMessageForFabricBRS(std::wstring const &action, Common::ActivityId const &activityId)
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
                SetBAHeaders(*message, Transport::Actor::BRS, action, activityId);

                return std::move(message);
            }

            static Transport::MessageUPtr CreateMessageForBAP(std::wstring const &action)
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
                SetBAHeaders(*message, Transport::Actor::BAP, action);

                return std::move(message);
            }

            static Transport::MessageUPtr CreateMessageForBAP(std::wstring const &action, Common::ActivityId const &activityId)
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
                SetBAHeaders(*message, Transport::Actor::BAP, action, activityId);

                return std::move(message);
            }

            template <class TBody>
            static Transport::MessageUPtr CreateMessageForFabricBRS(std::wstring const& action, TBody const& body)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));

                SetBAHeaders(*message, Transport::Actor::BRS, action);

                return std::move(message);
            }

            template <class TBody>
            static Transport::MessageUPtr CreateMessageForBAP(std::wstring const& action, TBody const& body)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));

                SetBAHeaders(*message, Transport::Actor::BAP, action);

                return std::move(message);
            }

            template <class TBody>
            static Transport::MessageUPtr CreateMessageForFabricBRS(std::wstring const& action, TBody const& body, Common::ActivityId const & activityId)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));

                SetBAHeaders(*message, Transport::Actor::BRS, action, activityId);

                return std::move(message);
            }

            template <class TBody>
            static Transport::MessageUPtr CreateMessageForBAP(std::wstring const& action, TBody const& body, Common::ActivityId const & activityId)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));

                SetBAHeaders(*message, Transport::Actor::BAP, action, activityId);

                return std::move(message);
            }

            static Transport::MessageUPtr CreateMessage(std::wstring const& action, Common::ActivityId const& activityId)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>());

                SetBAHeaders(*message, Transport::Actor::BA, action, activityId);          // TODO: Using default BA here

                return std::move(message);
            }

            template <class TBody>
            static Transport::MessageUPtr CreateMessage(std::wstring const& action, TBody const& body)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));

                SetBAHeaders(*message, Transport::Actor::BA, action);          // TODO: Using default BA here

                return std::move(message);
            }

            template <class TBody>
            static Transport::MessageUPtr CreateMessage(std::wstring const& action, TBody const& body, Common::ActivityId const & activityId)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));

                SetBAHeaders(*message, Transport::Actor::BA, action, activityId);          // TODO: Using default BA here

                return std::move(message);
            }

            static void SetBAHeaders(Transport::Message & message, Transport::Actor::Enum actor, std::wstring const &);
            static void SetBAHeaders(Transport::Message & message, Transport::Actor::Enum actor, std::wstring const & action, Common::ActivityId const & activityId);
            
            static Common::Global<BAMessage> RegisterBAP;
            static Common::Global<BAMessage> BackupNow;
            static Common::Global<BAMessage> BackupComplete;
        };
    }
}
