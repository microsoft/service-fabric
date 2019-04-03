// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricInstallerService
{
    struct FabricUpgradeContext
    {
        std::wstring fabricDataRoot;
        std::wstring fabricLogRoot;
        std::wstring fabricCodePath;
        std::wstring fabricRoot;
        std::wstring restartValidationFilePath;
        std::wstring targetInformationFilePath;
        ServiceModel::TargetInformationFileDescription targetInformationFileDescription;
        
        std::wstring ToString()
        {
            return Common::wformatString("FabricDataRoot:{0}, FabricLogRoot:{1}, FabricCodePath:{2}, FabricRoot:{3}, TargetInformationFilePath:{4}, TargetInformationDescription:{5}",
                fabricDataRoot,
                fabricLogRoot,
                fabricCodePath,
                fabricRoot,
                targetInformationFilePath,
                targetInformationFileDescription);
        }
    };

    class FabricUpgradeManager
        : public Common::ComponentRoot
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::FabricInstallerService>
    {
        DENY_COPY(FabricUpgradeManager)

    public:
        FabricUpgradeManager(bool autoclean);
        virtual ~FabricUpgradeManager();

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:
        class OpenAsyncOperation;
        class UpgradeAsyncOperation;
        class CloseAsyncOperation;
        Common::AutoResetEvent closeEvent_;
        bool autoclean_;
    };
}
