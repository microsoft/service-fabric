// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestHostSession;

    class FabricTestHostDispatcher: public FederationTestCommon::CommonDispatcher
    {
        DENY_COPY(FabricTestHostDispatcher)
    public: 
        FabricTestHostDispatcher();

        virtual bool Open();

        bool ExecuteCommand(std::wstring command);

        void PrintHelp(std::wstring const & command);

        std::wstring GetState(std::wstring const & param);

        Common::ErrorCode InitializeClientSecuritySettings(Transport::SecurityProvider::Enum);

        void SetAsSinglePackageHost(Common::ComPointer<Hosting2::ComCodePackageActivationContext> const& comCodePackageActivationContextCPtr, TestCodePackageContext const& testCodePackageContext);
        void SetAsMultiPackageHost(TestMultiPackageHostContext const& testMultiPackageHostContext);

        void ActivateCodePackage(
            std::wstring const& codePackageId,
            Common::ComPointer<Hosting2::ComCodePackageActivationContext> const& activationContext, 
            Common::ComPointer<IFabricRuntime> const& fabricRuntime,
            TestCodePackageContext const& testCodePackageContext);

        void DeactivateCodePackage(std::wstring const& codePackageId);

        void SendPlacementData(std::map<std::wstring, TestServiceInfo> const& activeServices, std::wstring const& codePackageId);

        static void ValidateErrorCodes(IFabricCodePackageActivationContext& activationContext);

        static void SendPlacementData(
            std::map<std::wstring, TestServiceInfo> const& activeServices, 
            TestCodePackageContext const& testCodePackageContext);
        static void SendServiceManifestUpdate(
            ServiceModel::ServiceManifestDescription const& description, 
            TestCodePackageContext const& testCodePackageContext);
        static void SendCodePackageActivate(
            ServiceModel::ServiceManifestDescription const& description,
            TestCodePackageContext const& testCodePackageContext);
        static void SendClientCommandReply(
            std::wstring const& replyString, 
            Common::ErrorCodeValue::Enum errorCodeValue,
            Transport::MessageId const& messageId);

    private:

        class TestCodePackage
        {
        public:
            TestCodePackage()
                : CodePackageActivationContextCPtr(),
                FabricRuntime(),
                TestHostServiceFactory(),
                TestCodePackageContext(),
                CodeHandlerId(0),
                ConfigHandlerId(0),
                DataHandlerId(0)
            {
            }

            Common::ComPointer<Hosting2::ComCodePackageActivationContext> CodePackageActivationContextCPtr;
            Common::ComPointer<IFabricRuntime> FabricRuntime;
            TestHostServiceFactorySPtr TestHostServiceFactory;
            TestCodePackageContext TestCodePackageContext;
            LONGLONG CodeHandlerId;
            LONGLONG ConfigHandlerId;
            LONGLONG DataHandlerId;
        };

        Hosting2::ApplicationHostType::Enum hostType_;
        TestCodePackage singleHostCodePackage_;

        Common::ExclusiveLock codePackagesLock_;
        std::wstring hostId_;
        map<std::wstring, shared_ptr<TestCodePackage>> codePackages_;

        Transport::SecuritySettings clientSecuritySettings_;

        static std::wstring const ParamDelimiter;
        static std::wstring const StatePropertyDelimiter;

        void RegisterServiceTypes(
            std::wstring const& codePackageName, 
            Common::ComPointer<IFabricRuntime> const& fabricRuntime,
            Common::ComPointer<Hosting2::ComCodePackageActivationContext> const& activationContext, 
            TestHostServiceFactorySPtr const& testHostServiceFactory);

        bool TryFindStoreService(std::wstring const& serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr);

        bool GetPlacementData(Common::StringCollection const & params);
        bool KillCodePackage(Common::StringCollection const & params);
        bool PutData(Common::StringCollection const & params);
        bool GetData(Common::StringCollection const & params);

        void SendPeriodicPlacementData();

        void VerifyPackageSharingPolicy(
            std::wstring const& codePackageName, 
            bool isShared,
            Common::ComPointer<Hosting2::ComCodePackageActivationContext> const& activationContext);
    };

}
