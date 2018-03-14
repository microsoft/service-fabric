// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ContainerExecCommandJsonWrapper : public Common::IFabricJsonSerializable
    {
    public:
        ContainerExecCommandJsonWrapper() :
            detach_(false),
            tty_(false)
        {
        }

        ~ContainerExecCommandJsonWrapper() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerExecCommandJsonWrapper::DetachParameter, detach_)
            SERIALIZABLE_PROPERTY(ContainerExecCommandJsonWrapper::TtyParameter, tty_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        bool detach_;
        bool tty_;

        static WStringLiteral const DetachParameter;
        static WStringLiteral const TtyParameter;
    };

    class ContainerIdJsonWrapper : public Common::IFabricJsonSerializable
    {
    public:
        ContainerIdJsonWrapper(wstring containerId) :
            containerId_(containerId)
        {
        }

        ~ContainerIdJsonWrapper() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerIdJsonWrapper::ContainerIdParameter, containerId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring containerId_;
        static WStringLiteral const ContainerIdParameter;
    };

    class ContainerExecJsonWrapper : public Common::IFabricJsonSerializable
    {
    public:
        ContainerExecJsonWrapper(std::vector<std::wstring> cmd) :
            attachStdin_(true),
            attachStdout_(true),
            attachStderr_(true),
            privileged_(false),
            tty_(true),
            detach_(false),
            detachKeys_(L""),
            cmd_(cmd)
        {
        }

        ~ContainerExecJsonWrapper() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerExecJsonWrapper::AttachStdinParameter, attachStdin_)
            SERIALIZABLE_PROPERTY(ContainerExecJsonWrapper::AttachStdoutParameter, attachStdout_)
            SERIALIZABLE_PROPERTY(ContainerExecJsonWrapper::AttachStderrParameter, attachStderr_)
            SERIALIZABLE_PROPERTY_IF(ContainerExecJsonWrapper::CmdParameter, cmd_, !cmd_.empty())
            SERIALIZABLE_PROPERTY(ContainerExecJsonWrapper::DetachKeysParameter, detachKeys_)
            SERIALIZABLE_PROPERTY(ContainerExecJsonWrapper::DetachParameter, detach_)
            SERIALIZABLE_PROPERTY(ContainerExecJsonWrapper::PrivilegedParameter, privileged_)
            SERIALIZABLE_PROPERTY(ContainerExecJsonWrapper::TtyParameter, tty_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        bool attachStdin_;
        bool attachStdout_;
        bool attachStderr_;
        bool privileged_;
        bool tty_;
        bool detach_;
        std::vector<std::wstring> cmd_;
        wstring detachKeys_;

        static WStringLiteral const AttachStdinParameter;
        static WStringLiteral const AttachStdoutParameter;
        static WStringLiteral const AttachStderrParameter;
        static WStringLiteral const CmdParameter;
        static WStringLiteral const DetachKeysParameter;
        static WStringLiteral const DetachParameter;
        static WStringLiteral const PrivilegedParameter;
        static WStringLiteral const TtyParameter;
    };

    class SetupExecCommandResponse : public Common::IFabricJsonSerializable
    {
    public:
        SetupExecCommandResponse() {};
        ~SetupExecCommandResponse() {};

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(SetupExecCommandResponse::IdParameter, Id)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring Id;
        static Common::WStringLiteral const IdParameter;
    };
}
