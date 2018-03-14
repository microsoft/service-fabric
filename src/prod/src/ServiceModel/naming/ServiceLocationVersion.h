// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceLocationVersion 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        ServiceLocationVersion()
          : fmVersion_(UnInitializedVersion)
          , storeVersion_(UnInitializedVersion)
          , generationNumber_(Reliability::GenerationNumber::Zero)
        {
        }

        ServiceLocationVersion(__int64 fmVersion, Reliability::GenerationNumber const & generationNumber, __int64 storeVersion)
          : fmVersion_(fmVersion)
          , storeVersion_(storeVersion)
          , generationNumber_(generationNumber)
        {
        }
        
        ServiceLocationVersion(
            ServiceLocationVersion const & other)
            : fmVersion_(other.fmVersion_)
            , storeVersion_(other.storeVersion_)
            , generationNumber_(other.generationNumber_)
        {
        }

        ServiceLocationVersion & ServiceLocationVersion::operator = (ServiceLocationVersion const & other)
        {
            if (this != &other)
            {
                fmVersion_ = other.fmVersion_;
                storeVersion_ = other.storeVersion_;
                generationNumber_ = other.generationNumber_;
            }

            return *this;
        }
        
        __declspec(property(get=get_FMVersion)) __int64 FMVersion;
        inline __int64 get_FMVersion() const { return fmVersion_; }
        
        __declspec(property(get=get_Generation)) Reliability::GenerationNumber const & Generation;
        inline Reliability::GenerationNumber const & get_Generation() const { return generationNumber_; }

        __declspec(property(get=get_StoreVersion)) __int64 StoreVersion;
        inline __int64 get_StoreVersion() const { return storeVersion_; }

        bool IsUnInitialized() const 
        { 
            return fmVersion_ == UnInitializedVersion && 
                storeVersion_ == UnInitializedVersion && 
                generationNumber_ == Reliability::GenerationNumber::Zero;
        }

        int64 CompareTo(ServiceLocationVersion const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;
        
        FABRIC_FIELDS_03(fmVersion_, generationNumber_, storeVersion_);
        
        static __int64 const UnInitializedVersion = -1;

        // No dynamic size overhead, just initialize the estimation
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        __int64 fmVersion_;
        __int64 storeVersion_;
        Reliability::GenerationNumber generationNumber_;
    };
}
