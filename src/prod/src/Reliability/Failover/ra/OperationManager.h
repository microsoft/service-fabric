// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        typedef Infrastructure::LookupTable<Common::ApiMonitoring::ApiName::Enum, Common::ApiMonitoring::ApiName::COUNT> CompatibleOperationTable;
        typedef Infrastructure::LookupTableBuilder<Common::ApiMonitoring::ApiName::Enum, Common::ApiMonitoring::ApiName::COUNT> CompatibleOperationTableBuilder;

        class OperationManager
        {
            DENY_COPY(OperationManager);

            enum State
            {
                Opened,
                Closed,
                Aborted
            };

        public:
            explicit OperationManager(
                CompatibleOperationTable const & compatibilityTable);

            // Operation Manager Lifecycle API
            void CloseForBusiness(
                bool isAbort);

            void OpenForBusiness();

            // Start and stop operations that are single instance
            bool CheckIfOperationCanBeStarted(
                Common::ApiMonitoring::ApiName::Enum operation) const;

            bool TryStartOperation(
                Common::ApiMonitoring::ApiCallDescriptionSPtr const & operation);

            bool TryContinueOperation(
                Common::ApiMonitoring::ApiName::Enum operation, 
                Common::AsyncOperationSPtr const & asyncOperation);

            Common::ApiMonitoring::ApiCallDescriptionSPtr FinishOperation(
                Common::ApiMonitoring::ApiName::Enum operation, Common::ErrorCode const & error);

            void CancelOperations();

            bool IsOperationRunning(
                Common::ApiMonitoring::ApiName::Enum operation) const;

            // Start and stop multi instance operations
            // The actual operation is stored in storage
            bool TryStartMultiInstanceOperation(
                Common::ApiMonitoring::ApiCallDescriptionSPtr const & operation,
                ExecutingOperation & storage);

            bool TryContinueMultiInstanceOperation(
                Common::ApiMonitoring::ApiName::Enum operationName,
                ExecutingOperation & storage, 
                Common::AsyncOperationSPtr const & asyncOperation) const;

            Common::ApiMonitoring::ApiCallDescriptionSPtr FinishMultiInstanceOperation(
                Common::ApiMonitoring::ApiName::Enum operationName,
                ExecutingOperation & storage, 
                Common::ErrorCode const & error);

            void CancelOrMarkOperationForCancel(
                Common::ApiMonitoring::ApiName::Enum operation);

            void RemoveOperationForCancel(
                Common::ApiMonitoring::ApiName::Enum operation);            

            // Query
            void GetDetailsForQuery(
                __out std::vector<Common::ApiMonitoring::ApiName::Enum> & operations,
                __out Common::DateTime & startTime) const;

            void WriteTo(
                Common::TextWriter & writer, 
                Common::FormatOptions const & options) const;

        private:
            bool IsCompatibleOperation(Common::ApiMonitoring::ApiName::Enum operationToRun) const;
            bool IsAllowedOperation(Common::ApiMonitoring::ApiName::Enum operationToRun) const;
            bool CheckIfOperationCanBeStartedCallerHoldsLock(Common::ApiMonitoring::ApiName::Enum operation) const;

            mutable Common::RwLock lock_;
            State state_;
            std::set<Common::ApiMonitoring::ApiName::Enum> disallowedOperations_;
            ExecutingOperationList outstandingOperations_;
            CompatibleOperationTable const & compatibilityTable_;
        };
    }
}


