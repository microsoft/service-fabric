// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.ComponentModel;
using System.Diagnostics.Tracing;
using System.Runtime.InteropServices;

namespace System.Fabric.Common.Tracing
{
    /// <summary>
    ///     Contains the metadata that defines an event.
    /// </summary>
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    internal struct GenericEventDescriptor
    {
        # region private
        [FieldOffset(0)]
        private int m_traceloggingId;
        [FieldOffset(0)]
        private ushort m_id;
        [FieldOffset(2)]
        private byte m_version;
        [FieldOffset(3)]
        private byte m_channel;
        [FieldOffset(4)]
        private byte m_level;
        [FieldOffset(5)]
        private byte m_opcode;
        [FieldOffset(6)]
        private ushort m_task;
        [FieldOffset(8)]
        private long m_keywords;
        #endregion

        //
        // Summary:
        //     Initializes a new instance of the GenericEventDescriptor
        //     class.
        //
        // Parameters:
        //   id:
        //     The event identifier.
        //
        //   version:
        //     Version of the event. The version indicates a revision to the event definition.
        //     You can use this member and the Id member to identify a unique event.
        //
        //   channel:
        //     Defines a potential target for the event.
        //
        //   level:
        //     Specifies the level of detail included in the event.
        //
        //   opcode:
        //     Operation being performed at the time the event is written.
        //
        //   task:
        //     Identifies a logical component of the application that is writing the event.
        //
        //   keywords:
        //     Bit mask that specifies the event category. The keyword can contain one or more
        //     provider-defined keywords, standard keywords, or both.
        public GenericEventDescriptor(
                int id,
                byte version,
                byte channel,
                byte level,
                byte opcode,
                int task,
                long keywords
                )
        {
            m_traceloggingId = 0;
            m_id = (ushort)id;
            m_version = version;
            m_channel = channel;
            m_level = level;
            m_opcode = opcode;
            m_keywords = keywords;
            m_task = (ushort)task;
        }

        public int EventId
        {
            get
            {
                return m_id;
            }
        }
        public byte Version
        {
            get
            {
                return m_version;
            }
        }
        public byte Channel
        {
            get
            {
                return m_channel;
            }
        }
        public byte Level
        {
            get
            {
                return m_level;
            }
        }
        public byte Opcode
        {
            get
            {
                return m_opcode;
            }
        }
        public int Task
        {
            get
            {
                return m_task;
            }
        }
        public long Keywords
        {
            get
            {
                return m_keywords;
            }
        }
    }
}