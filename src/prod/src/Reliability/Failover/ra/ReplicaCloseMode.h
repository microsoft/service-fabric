// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReplicaCloseMode
        {
        public:
            static const ReplicaCloseMode None;
            static const ReplicaCloseMode Close;
            static const ReplicaCloseMode Drop;
            static const ReplicaCloseMode DeactivateNode;
            static const ReplicaCloseMode Abort;
            static const ReplicaCloseMode Restart;
            static const ReplicaCloseMode Delete;
            static const ReplicaCloseMode Deactivate;
            static const ReplicaCloseMode ForceAbort;
            static const ReplicaCloseMode ForceDelete;
            static const ReplicaCloseMode QueuedDelete;
            static const ReplicaCloseMode AppHostDown;
            static const ReplicaCloseMode Obliterate;

            explicit ReplicaCloseMode(ReplicaCloseModeName::Enum name) :
                name_(name)
            {
            }

            ReplicaCloseMode() :
                name_(ReplicaCloseModeName::None)
            {
            }

            __declspec(property(get = get_Name)) ReplicaCloseModeName::Enum Name;
            ReplicaCloseModeName::Enum get_Name() const
            {
                return name_;
            }

            __declspec(property(get = get_IsDropImplied)) bool IsDropImplied;
            bool get_IsDropImplied() const
            {
                switch (name_)
                {
                case ReplicaCloseModeName::Close:
                case ReplicaCloseModeName::DeactivateNode:
                case ReplicaCloseModeName::Restart:
                case ReplicaCloseModeName::AppHostDown:
                    return false;

                case ReplicaCloseModeName::Obliterate:
                case ReplicaCloseModeName::Drop:
                case ReplicaCloseModeName::Abort:
                case ReplicaCloseModeName::Deactivate:
                case ReplicaCloseModeName::Delete:
                case ReplicaCloseModeName::ForceAbort:
                case ReplicaCloseModeName::ForceDelete:
                case ReplicaCloseModeName::QueuedDelete:
                    return true;

                default:
                    Common::Assert::CodingError("Unexpected close mode {0}", static_cast<int>(name_));
                }
            }

            __declspec(property(get = get_IsDeleteImplied)) bool IsDeleteImplied;
            bool get_IsDeleteImplied() const
            {
                switch (name_)
                {
                case ReplicaCloseModeName::Close:
                case ReplicaCloseModeName::DeactivateNode:
                case ReplicaCloseModeName::Restart:
                case ReplicaCloseModeName::Drop:
                case ReplicaCloseModeName::Abort:
                case ReplicaCloseModeName::Deactivate:
                case ReplicaCloseModeName::ForceAbort:
                case ReplicaCloseModeName::AppHostDown:
                case ReplicaCloseModeName::Obliterate:
                    return false;

                case ReplicaCloseModeName::Delete:
                case ReplicaCloseModeName::ForceDelete:
                case ReplicaCloseModeName::QueuedDelete:
                    return true;

                default:
                    Common::Assert::CodingError("Unexpected close mode {0}", static_cast<int>(name_));
                }
            }

            __declspec(property(get = get_IsForceDropImplied)) bool IsForceDropImplied;
            bool get_IsForceDropImplied() const
            {
                switch (name_)
                {
                case ReplicaCloseModeName::Close:
                case ReplicaCloseModeName::DeactivateNode:
                case ReplicaCloseModeName::Restart:
                case ReplicaCloseModeName::Drop:
                case ReplicaCloseModeName::Abort:
                case ReplicaCloseModeName::Deactivate:
                case ReplicaCloseModeName::Delete:
                case ReplicaCloseModeName::AppHostDown:
                case ReplicaCloseModeName::QueuedDelete:
                    return false;

                case ReplicaCloseModeName::ForceAbort:
                case ReplicaCloseModeName::ForceDelete:
                case ReplicaCloseModeName::Obliterate:
                    return true;

                default:
                    Common::Assert::CodingError("Unexpected close mode {0}", static_cast<int>(name_));
                }
            }

            __declspec(property(get = get_IsAppHostDownTerminalTransition)) bool IsAppHostDownTerminalTransition;
            bool get_IsAppHostDownTerminalTransition() const
            {
                switch (name_)
                {
                case ReplicaCloseModeName::Close:
                case ReplicaCloseModeName::DeactivateNode:
                case ReplicaCloseModeName::Restart:
                case ReplicaCloseModeName::Obliterate:
                case ReplicaCloseModeName::AppHostDown:
                    return true;

                
                case ReplicaCloseModeName::Drop:
                case ReplicaCloseModeName::ForceAbort:
                case ReplicaCloseModeName::Abort:
                case ReplicaCloseModeName::Deactivate:
                case ReplicaCloseModeName::Delete:
				case ReplicaCloseModeName::ForceDelete:
				case ReplicaCloseModeName::QueuedDelete:
                    return false;

                default:
                    Common::Assert::CodingError("Unexpected close mode {0}", static_cast<int>(name_));
                }
            }

            __declspec(property(get = get_MessageFlags)) ProxyMessageFlags::Enum MessageFlags;
            ProxyMessageFlags::Enum get_MessageFlags() const
            {
                switch (name_)
                {
                case ReplicaCloseModeName::ForceAbort:
                case ReplicaCloseModeName::ForceDelete:
                case ReplicaCloseModeName::Abort:
                    return ProxyMessageFlags::Abort;

                case ReplicaCloseModeName::Drop:
                case ReplicaCloseModeName::Delete:
                case ReplicaCloseModeName::Deactivate:
                    return ProxyMessageFlags::Drop;

                case ReplicaCloseModeName::Close:
                case ReplicaCloseModeName::DeactivateNode:
                case ReplicaCloseModeName::Restart:
                    return ProxyMessageFlags::None;

                default:
                    Common::Assert::CodingError("Unknown close mode {0}", static_cast<int>(name_));
                };
            }

            bool operator==(ReplicaCloseMode const & other) const
            {
                return name_ == other.Name;
            }

            bool operator!=(ReplicaCloseMode const & other) const
            {
                return !(*this == other);
            }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

        private:
            ReplicaCloseModeName::Enum name_;

        };
    }
}


