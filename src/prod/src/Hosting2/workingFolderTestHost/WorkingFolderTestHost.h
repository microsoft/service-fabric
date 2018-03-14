// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

class WorkingFolderTestHost :
    public Common::ComponentRoot,
    public Common::FabricComponent,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
    DENY_COPY(WorkingFolderTestHost)

public:
    WorkingFolderTestHost(std::wstring const & testName, bool isSetupEntryPoint);
    virtual ~WorkingFolderTestHost();

    Common::ErrorCode OnOpen();
    Common::ErrorCode OnClose();
    void OnAbort();

private:
    Common::ErrorCode VerifyWorkingFolder();

private:
    Common::ComPointer<IFabricRuntime> runtime_;
    Common::ComPointer<IFabricCodePackageActivationContext2> activationContext_;
    std::wstring traceId_;
    std::wstring const testName_;
    std::wstring const typeName_;
    bool const isSetupEntryPoint_;
    Common::RealConsole console_;

    static Common::GlobalWString WorkingFolderAppId;
};
