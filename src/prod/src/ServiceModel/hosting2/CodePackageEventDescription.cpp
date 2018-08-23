// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceSource("CodePackageEventDescription");

namespace Hosting2
{
    namespace CodePackageEventType
    {
        void WriteToTextWriter(__in TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid: w << "Invalid"; return;
            case StartFailed: w << "StartFailed"; return;
            case Started: w << "Started"; return;
            case Ready: w << "Ready"; return;
            case Health: w << "Health"; return;
            case Stopped: w << "Stopped"; return;
            case Terminated: w << "Terminated"; return;
            default: w << "CodePackageEventType(" << static_cast<ULONG>(e) << ')'; return;
            }
        }

        FABRIC_CODE_PACKAGE_EVENT_TYPE ToPublicApi(__in Enum internalType)
        {
            switch (internalType)
            {
            case Invalid:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_INVALID;

            case StartFailed:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_START_FAILED;

            case Started:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_STARTED;

            case Ready:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_READY;

            case Health:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_HEALTH;

            case Stopped:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_STOPPED;

            case Terminated:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_TERMINATED;

            default:
                return FABRIC_CODE_PACKAGE_EVENT_TYPE_INVALID;
            }
        }

        ErrorCode FromPublicApi(__in FABRIC_CODE_PACKAGE_EVENT_TYPE publicType, __out Enum & internalType)
        {
            switch (publicType)
            {
            case FABRIC_CODE_PACKAGE_EVENT_TYPE_INVALID:
                internalType = Invalid;
                return ErrorCodeValue::Success;

            case FABRIC_CODE_PACKAGE_EVENT_TYPE_START_FAILED:
                internalType = StartFailed;
                return ErrorCodeValue::Success;

            case FABRIC_CODE_PACKAGE_EVENT_TYPE_STARTED:
                internalType = Started;
                return ErrorCodeValue::Success;

            case FABRIC_CODE_PACKAGE_EVENT_TYPE_READY:
                internalType = Ready;
                return ErrorCodeValue::Success;

            case FABRIC_CODE_PACKAGE_EVENT_TYPE_HEALTH:
                internalType = Health;
                return ErrorCodeValue::Success;

            case FABRIC_CODE_PACKAGE_EVENT_TYPE_STOPPED:
                internalType = Stopped;
                return ErrorCodeValue::Success;

            case FABRIC_CODE_PACKAGE_EVENT_TYPE_TERMINATED:
                internalType = Terminated;
                return ErrorCodeValue::Success;

            default:
                return ErrorCodeValue::InvalidArgument;
            }
        }

        ENUM_STRUCTURED_TRACE(CodePackageEventType, FirstValidEnum, LastValidEnum)
    }

    wstring const CodePackageEventProperties::ExitCode = L"ExitCode";
    wstring const CodePackageEventProperties::ErrorMessage = L"ErrorMessage";
    wstring const CodePackageEventProperties::FailureCount = L"FailureCount";
    wstring const CodePackageEventProperties::Version = L"Version";

    CodePackageEventDescription::CodePackageEventDescription()
        : codePackageName_()
        , isSetupEntryPoint_()
        , isContainerHost_()
        , eventType_()
        , timeStamp_()
        , squenceNumber_()
        , properties_()
    {
    }

    CodePackageEventDescription::CodePackageEventDescription(
        wstring const & codePackageName,
        bool isSetupEntryPoint,
        bool isContainerHost,
        CodePackageEventType::Enum eventType,
        Common::DateTime timeStamp,
        int64 squenceNumber,
        map<wstring, wstring> const & properties)
        : codePackageName_(codePackageName)
        , isSetupEntryPoint_(isSetupEntryPoint)
        , isContainerHost_(isContainerHost)
        , eventType_(eventType)
        , timeStamp_(timeStamp)
        , squenceNumber_(squenceNumber)
        , properties_(properties)
    {
    }

    void CodePackageEventDescription::WriteTo(TextWriter & w, FormatOptions const &) const
    {
        w.Write("CodePackageEventDescription { ");
        w.Write("CodePackageName={0}, ", CodePackageName);
        w.Write("IsSetupEntryPoint={0}, ", IsSetupEntryPoint);
        w.Write("IsContainerHost={0}, ", IsContainerHost);
        w.Write("EventType={0}, ", EventType);
        w.Write("TimeStamp={0}, ", TimeStamp);
        w.Write("SquenceNumber={0}, ", SquenceNumber);
        w.Write("Properties=[");

        for (auto const & kvPair : Properties)
        {
            w.Write("{0}={1}, ", kvPair.first, kvPair.second);
        }

        w.Write("]}");
    }

    ErrorCode CodePackageEventDescription::ToPublicApi(
        __in ScopedHeap & heap,
        __out FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION & eventDesc) const
    {
        eventDesc.CodePackageName = heap.AddString(this->CodePackageName);
        eventDesc.IsSetupEntryPoint = this->IsSetupEntryPoint;
        eventDesc.IsContainerHost = this->IsContainerHost;
        eventDesc.EventType = CodePackageEventType::ToPublicApi(this->EventType);
        eventDesc.TimeStampInTicks = this->TimeStamp.Ticks;
        eventDesc.SequenceNumber = this->SquenceNumber;

        auto properties = heap.AddItem<FABRIC_STRING_MAP>();
        auto error = PublicApiHelper::ToPublicApiStringMap(heap, this->Properties, *properties);
        if (!error.IsSuccess())
        {
            return error;
        }
        eventDesc.Properties = properties.GetRawPointer();

        return ErrorCode::Success();
    }

    ErrorCode CodePackageEventDescription::FromPublicApi(FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION const & eventDesc)
    {
        auto error = StringUtility::LpcwstrToWstring2(eventDesc.CodePackageName, true, codePackageName_);
        if (!error.IsSuccess())
        {
            return error;
        }

        isSetupEntryPoint_ = eventDesc.IsSetupEntryPoint;
        isContainerHost_ = eventDesc.IsContainerHost;
        
        error = CodePackageEventType::FromPublicApi(eventDesc.EventType, eventType_);
        if (!error.IsSuccess())
        {
            return error;
        }

        timeStamp_ = DateTime(eventDesc.TimeStampInTicks);
        squenceNumber_ = eventDesc.SequenceNumber;

        error = PublicApiHelper::FromPublicApiStringMap(*(eventDesc.Properties), properties_);

        return error;
    }
}
