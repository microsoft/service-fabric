// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace KtlLogger
{
    class KtlLoggerNode : public Common::AsyncFabricComponent,
                          public std::enable_shared_from_this<KtlLoggerNode>,
                          public Common::RootedObject,
                          public Common::TextTraceComponent<Common::TraceTaskCodes::KtlLoggerNode>
                          
    {
    public:
        KtlLoggerNode(Common::ComponentRoot const & root, 
                      std::wstring const & fabricDataRoot,
                      Common::FabricNodeConfigSPtr const & fabricNodeConfig);

        ~KtlLoggerNode();

        __declspec(property(get = get_ApplicationSharedLogSettings)) KtlLogger::SharedLogSettingsSPtr ApplicationSharedLogSettings;
        KtlLogger::SharedLogSettingsSPtr get_ApplicationSharedLogSettings() const { return applicationSharedLogSettings_; }

        __declspec(property(get = get_SystemServicesSharedLogSettings)) KtlLogger::SharedLogSettingsSPtr SystemServicesSharedLogSettings;
        KtlLogger::SharedLogSettingsSPtr get_SystemServicesSharedLogSettings() const { return systemServicesSharedLogSettings_; }

        __declspec(property(get = get_KtlSystem)) KtlSystem & KtlSystemObject;
        KtlSystem & get_KtlSystem() const;

        void WriteTraceInfo(ULONG A, ULONG B, ULONG C);
        void WriteTraceError(ULONG A, ULONG B, NTSTATUS Status);

        
    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &) override ;

        Common::ErrorCode OnEndOpen(__in Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & , 
            Common::AsyncOperationSPtr const &) override ;

        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) override ;

        void OnAbort();

    private:
        void TryUnregister();
        NTSTATUS TryDeactivateKtlSystem(Common::TimeSpan const timeout);
        KAllocator & GetAllocator();
        void PostThreadDeactivateKtlLoggerAndSystem(Common::TimeSpan const timeout);
    
        std::wstring fabricDataRoot_;
        Common::FabricNodeConfigSPtr fabricNodeConfig_;
        KtlLogger::SharedLogSettingsSPtr applicationSharedLogSettings_;
        KtlLogger::SharedLogSettingsSPtr systemServicesSharedLogSettings_;
        bool const useGlobalKtlSystem_;
        std::shared_ptr<KtlSystemBase> ktlSystem_;
        bool isRegistered_;

    public:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class InitializeKtlLoggerAsyncOperation;

        // ********************************************************************************************************************
        // KtlLoggerNode::OpenAsyncOperation Implementation
        //
        class OpenAsyncOperation : 
            public Common::AsyncOperation
        {
            DENY_COPY(OpenAsyncOperation)

        public:
            OpenAsyncOperation(
                KtlLoggerNode & owner,
                std::wstring const & fabricDataRoot,
                Common::FabricNodeConfigSPtr const & fabricNodeConfig,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
                : Common::AsyncOperation(callback, parent)
                , owner_(owner)
                , fabricDataRoot_(fabricDataRoot)
                , fabricNodeConfig_(fabricNodeConfig)
                , timeoutHelper_(timeout)
            {
            }

            virtual ~OpenAsyncOperation()
            {
            }

            static Common::ErrorCode End(
                __in Common::AsyncOperationSPtr const & operation,
                __out std::unique_ptr<KtlLogManager::SharedLogContainerSettings>& applicationSharedLogSettings,
                __out std::unique_ptr<KtlLogManager::SharedLogContainerSettings>& systemServicesSharedLogSettings)
            {
                auto thisPtr = Common::AsyncOperation::End<OpenAsyncOperation>(operation);
                
                if (thisPtr->Error.IsSuccess())
                {
                    applicationSharedLogSettings = std::move(thisPtr->applicationSharedLogSettings_);
                    systemServicesSharedLogSettings = std::move(thisPtr->systemServicesSharedLogSettings_);
                }
                
                return thisPtr->Error;
            }

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr)  override ;

        private:
            void OnInitializeKtlLoggerCompleted(Common::AsyncOperationSPtr const& operation);

            void FinishInitializeKtlLogger(Common::AsyncOperationSPtr const& operation);
            
            KtlLoggerNode & owner_;
            std::wstring fabricDataRoot_;
            Common::FabricNodeConfigSPtr fabricNodeConfig_;         
            Common::TimeoutHelper timeoutHelper_;
            std::unique_ptr<KtlLogManager::SharedLogContainerSettings> applicationSharedLogSettings_;
            std::unique_ptr<KtlLogManager::SharedLogContainerSettings> systemServicesSharedLogSettings_;
        };

        class CloseAsyncOperation
            : public Common::AsyncOperation
        {
            DENY_COPY(CloseAsyncOperation)

        public:
            CloseAsyncOperation(
                KtlLoggerNode & owner,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
                : Common::AsyncOperation(callback, parent)
                , owner_(owner)
                , timeoutHelper_(timeout)
            {
            }

            virtual ~CloseAsyncOperation() 
            { 
            }

            static Common::ErrorCode CloseAsyncOperation::End(Common::AsyncOperationSPtr const & operation)
            {
                auto thisPtr = Common::AsyncOperation::End<CloseAsyncOperation>(operation);
                return thisPtr->Error;
            }  ;

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            KtlLoggerNode & owner_;
            Common::TimeoutHelper timeoutHelper_;
        };
    };
}
