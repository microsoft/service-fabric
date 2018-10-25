// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DockerProcessTerminatedNotification : public Serialization::FabricSerializable
    {
    public:
        DockerProcessTerminatedNotification();
        DockerProcessTerminatedNotification(
            DWORD exitCode,
            ULONG continuousFailureCount,
            Common::TimeSpan nextStartTime);

        __declspec(property(get=get_ExitCode)) DWORD ExitCode;
        DWORD get_ExitCode() const { return exitCode_; }

        __declspec(property(get = get_ContinuousFailureCount)) ULONG ContinuousFailureCount;
        ULONG get_ContinuousFailureCount() const { return continuousFailureCount_; }

        __declspec(property(get = get_NextStartTime)) Common::TimeSpan const& NextStartTime;
        Common::TimeSpan get_NextStartTime() const { return nextStartTime_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(exitCode_, continuousFailureCount_, nextStartTime_);

    private:
        DWORD exitCode_;
        ULONG continuousFailureCount_;
        Common::TimeSpan nextStartTime_;
    };
}
