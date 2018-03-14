// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#undef ENTER
#define ENTER TTestUtil::EnterTest(FUNCTION_TRACE); {
#undef LEAVE
#define LEAVE } TTestUtil::LeaveTest(FUNCTION_TRACE)

namespace Transport
{
    namespace ListenAddressType
    {
        enum Enum
        {
            LoopbackIpv4 = 0, // 127.0.0.1
            LoopbackIpv6 = 1, // [::]
            Localhost = 2, // localhost
            Fqdn = 3, // FQDN
        };
    }

    class TTestUtil
    {
    public:
        // Use Common::TestPortHelper to get static listen address
        static std::wstring GetListenAddress(ListenAddressType::Enum listenAddressType = ListenAddressType::LoopbackIpv4);
        static std::wstring GetListenAddress(std::wstring const & addressWithoutPort);

        // Use GUID action to further prevent crosstalk between different tests
        static std::wstring GetGuidAction();
        static void SetMessageHandler(
            IDatagramTransportSPtr transport,
            std::wstring const & action,
            IDatagramTransport::MessageHandler const & handler);

        static SecuritySettings CreateTestSecuritySettings(
            SecurityProvider::Enum provider,
            ProtectionLevel::Enum protectionLevel = ProtectionLevel::EncryptAndSign);

        static SecuritySettings CreateX509Settings(
            std::wstring const & subjectNameFindValue,
            std::wstring const & remoteCommonNames = L"",
            std::wstring const & certIssuerThumbprints = L"");

        static SecuritySettings CreateX509Settings(
            std::wstring const & subjectNameFindValue,
            std::wstring const & remoteCommonNames,
            Common::SecurityConfig::IssuerStoreKeyValueMap const & certIssuerStores);

        static SecuritySettings CreateX509Settings(
            std::wstring const & subjectNameFindValue,
            Common::SecurityConfig::X509NameMap const & remoteX509Names);

        static SecuritySettings CreateX509Settings(
            std::wstring const & subjectNameFindValue,
            Common::SecurityConfig::X509NameMap const & remoteX509Names,
            Common::SecurityConfig::IssuerStoreKeyValueMap const & issuerStores);

        static SecuritySettings CreateSvrX509Settings_ClaimsTest(
            std::wstring const & svrCertThumbprint,
            std::wstring const & clientCertThumbprints = L"");

        static SecuritySettings CreateX509Settings(
            std::wstring const & subjectNameFindValue,
            std::wstring const & subjectNameFindValueSecondary,
            std::wstring const & remoteCommonNames,
            std::wstring const & certIssuerThumbprints);

        static SecuritySettings CreateX509Settings2(
            std::wstring const & thumbprintFindValue,
            std::wstring const & remoteCommonNames = L"",
            std::wstring const & certIssuerThumbprints = L"");

        static SecuritySettings CreateX509Settings2(
            std::wstring const & thumbprintFindValue,
            Common::SecurityConfig::X509NameMap const & remoteX509Names);

        static SecuritySettings CreateX509Settings2(
            std::wstring const & thumbprintFindValue,
            std::wstring const & thumbprintFindValueSecondary,
            std::wstring const & remoteCommonNames,
            std::wstring const & certIssuerThumbprints);

        static SecuritySettings CreateX509Settings3(
            std::wstring const & findBySubjectAltName,
            std::wstring const & remoteCommonNames,
            std::wstring const & certIssuerThumbprints);

        static SecuritySettings CreateX509Settings_LoadByName_AuthByThumbprint(
            std::wstring const & localCertCommonName,
            std::wstring const & remoteCertThumbprint);

        static SecuritySettings CreateX509Settings_CertRefresh_IssuerStore(
            wstring const & localCertCommonName,
            Common::SecurityConfig::X509NameMap remoteNames,
            Common::SecurityConfig::IssuerStoreKeyValueMap issuerStores);

        static SecuritySettings CreateX509SettingsBySan(
            std::wstring const & storeName,
            std::wstring const & localDnsNameInSan,
            std::wstring const & remoteDnsNameInSan);

        static SecuritySettings CreateX509SettingsForRoundTripTest(
            std::wstring const & certIssuerThumbprints,
            std::wstring const & remoteCertThumbprints,
            std::wstring const & findValueSecondary,
            Common::X509FindType::Enum findType);

        static SecuritySettings CreateX509SettingsTp(
            std::wstring const & localCertThumbprint,
            std::wstring const & localCertThumbprint2,
            std::wstring const & remoteCertThumbprint,
            std::wstring const & remoteCertThumbprint2);

        static SecuritySettings CreateX509SettingsExpired(std::wstring const & remoteCommonNames = L"");

        static SecuritySettings CreateKerbSettings();

        static bool WaitUntil(
            std::function<bool(void)> const & becomeTrue,
            Common::TimeSpan timeout,
            Common::TimeSpan interval = Common::TimeSpan::FromMilliseconds(10));

        static void EnterTest(char const * testName);
        static void LeaveTest(char const * testName);

        static void CopyFile(std::wstring const & file, std::wstring const & srcDir, std::wstring & destDir);
        static void CopyFiles(std::vector<std::wstring> const & files, std::wstring const & srcDir, std::wstring & destDir);

        static void CreateRemoteProcess(
            std::wstring const & computer,
            std::wstring const & cmdline,
            std::wstring const & workDir,
            DWORD flags = 0);

        static void CreateRemoteProcessWithPsExec(
            std::wstring const & computer,
            std::wstring const & cmdline,
            std::wstring const & workDir,
            DWORD flags = 0);

        static void ReduceTracingInFreBuild();
        static void RecoverTracingInFreBuild();

        static void DisableSendThrottling();
    };

    struct TcpTestMessage : public Serialization::FabricSerializable
    {
        TcpTestMessage()
        {
        }

        TcpTestMessage(std::wstring const & message)
            : message_(message)
        {
        }

        FABRIC_FIELDS_01(message_);

        std::wstring message_;
    };

    struct TestMessageBody : public Serialization::FabricSerializable
    {
        TestMessageBody(size_t size = 0)
        {
            data_.reserve(size);
            for (ULONG i = 0; i < size; ++ i)
            {
                data_.push_back((byte)i);
            }
        }

        bool Verify() const
        {
            for (ULONG i = 0; i < data_.size(); ++ i)
            {
                if (data_[i] != (byte)i)
                {
                    return false;
                }
            }

            return true;
        }

        size_t size() const { return data_.size(); }

        FABRIC_FIELDS_01(data_);

    private:
        std::vector<byte> data_;
    };

    struct RequestReplyWrapper : public Common::ComponentRoot
    {
        static std::shared_ptr<RequestReplyWrapper> Create(IDatagramTransportSPtr const & t)
        {
            return make_shared<RequestReplyWrapper>(t);
        }

        RequestReplyWrapper(IDatagramTransportSPtr const & t) : transport(t)
        {
        }

        RequestReply* InitRequestReply()
        {
            requestReply = make_shared<RequestReply>(*this, transport);
            requestReply->Open();
            return requestReply.get();
        }

        IDatagramTransportSPtr transport;
        RequestReplySPtr requestReply;
    };

    struct DuplexRequestReplyWrapper : public Common::ComponentRoot
    {
        static std::shared_ptr<DuplexRequestReplyWrapper> Create(IDatagramTransportSPtr const & t)
        {
            return make_shared<DuplexRequestReplyWrapper>(t);
        }

        DuplexRequestReplyWrapper(IDatagramTransportSPtr const & t) : transport(t)
        {
        }

        DuplexRequestReply* InitRequestReply()
        {
            requestReply = make_shared<DuplexRequestReply>(*this, transport);
            requestReply->Open();
            return requestReply.get();
        }

        IDatagramTransportSPtr transport;
        DuplexRequestReplySPtr requestReply;
    };

    struct TestLeaseMessage : public LTMessageHeader
    {
        TestLeaseMessage(uint64 msgId)
        {
            MessageIdentifier.QuadPart = msgId;
            MessageHeaderSize = sizeof(TestLeaseMessage);

            for(uint i = 0; i < sizeof(data); ++i)
            {
                data[i] = (unsigned char)i;
            }
        }

        bool Verify() const
        {
            for(int i = 0; i < sizeof(data); ++i)
            {
                if (data[i] != (unsigned char)i) return false;
            }

            return true;
        }

        unsigned char data[256];
    };
}
