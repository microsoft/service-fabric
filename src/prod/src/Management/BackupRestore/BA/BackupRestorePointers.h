// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BAFederationWrapper;
        typedef std::unique_ptr<BAFederationWrapper> FederationWrapperUPtr;

        class IBaToBapTransport;
        typedef std::unique_ptr<IBaToBapTransport> IBaToBapTransportUPtr;

        class BackupRestoreAgent;
        typedef std::unique_ptr<BackupRestoreAgent> BackupRestoreAgentUPtr;

        // Define BAP and factory function
        struct BackupRestoreAgentProxyConstructorParameters;
        class IBackupRestoreAgentProxy;
        class BackupRestoreAgentProxy;
        typedef std::unique_ptr<IBackupRestoreAgentProxy> IBackupRestoreAgentProxyUPtr;

        class GetPolicyRequestMessageBody;
        typedef std::unique_ptr<GetPolicyRequestMessageBody> GetPolicyRequestMessageBodyUPtr;

        class GetPolicyReplyMessageBody;
        typedef std::unique_ptr<GetPolicyReplyMessageBody> GetPolicyReplyMessageBodyUPtr;

        class BAMessage;
        typedef std::unique_ptr<BAMessage> BAMessageUPtr;

        class BackupPolicy;
        typedef std::unique_ptr<BackupPolicy> BackupPolicyUPtr;
        typedef std::shared_ptr<BackupPolicy> BackupPolicySPtr;

        class BackupPartitionRequestMessageBody;
        typedef std::unique_ptr<BackupPartitionRequestMessageBody> BackupPartitionRequestMessageBodyUPtr;
        typedef std::shared_ptr<BackupPartitionRequestMessageBody> BackupPartitionRequestMessageBodySPtr;

        class BackupCopierProxy;
        typedef std::shared_ptr<BackupCopierProxy> BackupCopierProxySPtr;

        class UploadBackupMessageBody;
        typedef std::unique_ptr<UploadBackupMessageBody> UploadBackupMessageBodyUPtr;
        typedef std::shared_ptr<UploadBackupMessageBody> UploadBackupMessageBodySPtr;

        class DownloadBackupMessageBody;
        typedef std::unique_ptr<DownloadBackupMessageBody> DownloadBackupMessageBodyUPtr;
        typedef std::shared_ptr<DownloadBackupMessageBody> DownloadBackupMessageBodySPtr;
    }
}
