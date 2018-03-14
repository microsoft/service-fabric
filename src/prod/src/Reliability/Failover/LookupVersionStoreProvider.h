// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class LookupVersionGlobalState : public Federation::SerializableWithActivation
    {
    public:
        LookupVersionGlobalState();

        LookupVersionGlobalState(LookupVersionGlobalState const & other);

        __declspec(property(get = get_FMGenerationNumber)) GenerationNumber FMGeneration;
        GenerationNumber get_FMGenerationNumber() const
        {
            return fmGeneration_;
        }

        __declspec(property(get = get_FMEndVersion)) int64 FMEndVersion;
        int64 get_FMEndVersion() const
        {
            return fmEndVersion_;
        }

        __declspec(property(get = get_FMMGenerationNumber)) GenerationNumber FMMGeneration;
        GenerationNumber get_FMMGenerationNumber() const
        {
            return fmmGeneration_;
        }

        __declspec(property(get = get_FMMEndVersion)) int64 FMMEndVersion;
        int64 get_FMMEndVersion() const
        {
            return fmmEndVersion_;
        }

        bool Update(LookupVersionGlobalState const & other);
        bool UpdateFMState(GenerationNumber const & generation, int64 endVersion);
        bool UpdateFMMState(GenerationNumber const & generation, int64 endVersion);

        FABRIC_FIELDS_04(fmGeneration_, fmEndVersion_, fmmGeneration_, fmmEndVersion_);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:
        GenerationNumber fmGeneration_;
        int64 fmEndVersion_;
        GenerationNumber fmmGeneration_;
        int64 fmmEndVersion_;
    };

    class LookupVersionStoreProvider : public Federation::GlobalStoreProvider, public Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>
    {
    public:
        LookupVersionStoreProvider(std::wstring const & traceId);

        virtual void AddOutputState(Federation::SerializableWithActivationList & output, Federation::NodeInstance const* target);
        virtual void ProcessInputState(Federation::SerializableWithActivation const & input, Federation::NodeInstance const & from);

        void UpdateFMState(GenerationNumber const & generation, int64 endVersion);
        void UpdateFMMState(GenerationNumber const & generation, int64 endVersion);

        LookupVersionGlobalState GetState();

    private:
        std::wstring traceId_;

        Common::RwLock lock_;
        LookupVersionGlobalState state_;
    };

    typedef std::shared_ptr<LookupVersionStoreProvider> LookupVersionStoreProviderSPtr;
}
