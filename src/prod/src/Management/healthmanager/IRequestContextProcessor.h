// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class IQueryProcessor
        {
        public:
            virtual void StartProcessQuery(Common::AsyncOperationSPtr const & thisSPtr) = 0;
        };

        class ReportRequestContext;
        class IReportRequestContextProcessor
        {
        public:
            IReportRequestContextProcessor() {}
            virtual ~IReportRequestContextProcessor() {}

            virtual void OnReportProcessed(
                Common::AsyncOperationSPtr const & thisSPtr,
                ReportRequestContext const & requestContext,
                Common::ErrorCode const & error) = 0;
        };

        class SequenceStreamRequestContext;
        class ISequenceStreamRequestContextProcessor
        {
        public:
            ISequenceStreamRequestContextProcessor() {}
            virtual ~ISequenceStreamRequestContextProcessor() {}

            virtual void OnSequenceStreamProcessed(
                Common::AsyncOperationSPtr const & thisSPtr,
                SequenceStreamRequestContext const & requestContext,
                Common::ErrorCode const & error) = 0;
        };
    }
}
