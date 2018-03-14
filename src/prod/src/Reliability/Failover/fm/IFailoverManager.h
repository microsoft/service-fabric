// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class IFailoverManager
        {
        public:
            __declspec (property(get=get_IsReady)) bool IsReady;
            virtual bool get_IsReady() const  = 0;

            __declspec (property(get=get_IsMaster)) bool IsMaster;
            virtual bool get_IsMaster() const  = 0;

            __declspec (property(get=get_Id)) std::wstring const& Id;
            virtual std::wstring const& get_Id() const { return id_; }            


            virtual SerializedGFUMUPtr Close(bool isStoreCloseNeeded) = 0;

            virtual void Open(
                Common::EventHandler const & readyEvent, 
                Common::EventHandler const & failureEvent) = 0;

            virtual ~IFailoverManager()
            {
            }

        protected:
            IFailoverManager(std::wstring const & id)
            : id_(id)
            {
            }

            std::wstring id_;
        };

        struct FailoverManagerConstructorParameters
        {
            Federation::FederationSubsystemSPtr Federation;
            Client::HealthReportingComponentSPtr HealthClient;
            Common::FabricNodeConfigSPtr NodeConfig;
        };

        typedef IFailoverManagerSPtr FMFunctionPtr(FailoverManagerConstructorParameters &);
        FMFunctionPtr FMMFactory;
    }
}

