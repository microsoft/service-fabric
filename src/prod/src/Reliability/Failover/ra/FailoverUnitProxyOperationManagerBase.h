// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class FailoverUnitProxyOperationManagerBase
        {
            DENY_COPY(FailoverUnitProxyOperationManagerBase);

        public:
            FailoverUnitProxyOperationManagerBase(
                FailoverUnitProxy & owner,
                Common::ApiMonitoring::InterfaceName::Enum interfaceName,
                CompatibleOperationTable const & compatibilityTable);

            virtual ~FailoverUnitProxyOperationManagerBase() {}

            bool CheckIfOperationCanBeStarted(
                Common::ApiMonitoring::ApiName::Enum operation) const;

            Common::ApiMonitoring::ApiCallDescriptionSPtr TryStartOperation(
                Common::ApiMonitoring::ApiName::Enum operation);

            Common::ApiMonitoring::ApiCallDescriptionSPtr TryStartOperation(
                Common::ApiMonitoring::ApiName::Enum operation,
                std::wstring const & metadata);

            bool TryContinueOperation(
                Common::ApiMonitoring::ApiName::Enum operation, 
                Common::AsyncOperationSPtr const & asyncOperation);

            void FinishOperation(
                Common::ApiMonitoring::ApiName::Enum operation, 
                Common::ErrorCode const & error);

            bool TryStartMultiInstanceOperation(
                Common::ApiMonitoring::ApiName::Enum apiName,
                std::wstring const & metadata,
                Reliability::ReplicaDescription const & replicaDescription,
                __out ExecutingOperation & storage);

            bool TryContinueMultiInstanceOperation(
                Common::ApiMonitoring::ApiName::Enum operation,
                ExecutingOperation & storage,
                Common::AsyncOperationSPtr const & asyncOperation) const;

            void FinishMultiInstanceOperation(
                Common::ApiMonitoring::ApiName::Enum operation,
                ExecutingOperation & storage,
                Common::ErrorCode const & error);

            bool IsOperationRunning(
                Common::ApiMonitoring::ApiName::Enum operation) const;

            void CloseForBusiness(bool isAbort);
            void OpenForBusiness();

            void CancelOperations();

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            std::string ToString() const;

        protected:
            Common::ApiMonitoring::ApiCallDescriptionSPtr CreateApiCallDescription(
                Common::ApiMonitoring::ApiName::Enum operation,
                std::wstring const & metadata,
                Reliability::ReplicaDescription const & replicaDesc) const;

            OperationManager opMgr_;
            FailoverUnitProxy & owner_;

        private:
            virtual void GetApiCallDescriptionProperties(
                Common::ApiMonitoring::ApiName::Enum operation,
                __out bool & isSlowHealthReportRequired,
                __out bool & traceServiceType) const
            {
                // by default -> for each op slow health report and service type trace is required
                // derived class can override if needed
                UNREFERENCED_PARAMETER(operation);
                UNREFERENCED_PARAMETER(isSlowHealthReportRequired);
                UNREFERENCED_PARAMETER(traceServiceType);
            }

            Common::ApiMonitoring::InterfaceName::Enum interface_;
        };
    }
}


