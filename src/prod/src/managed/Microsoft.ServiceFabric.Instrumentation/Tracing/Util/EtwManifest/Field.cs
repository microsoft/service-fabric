// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    /// <summary>
    /// Encapsulation over one field inside an Event.
    /// </summary>
    public sealed class Field
    {
        internal Field(string name, string type, string mapId, int position)
        {
            this.Name = name;
            this.Type = type;
            this.MapId = mapId;
            this.Position = position;
        }

        /// <summary>
        /// The name of the field.
        /// </summary>
        public string Name { get; private set; }

        /// <summary>
        /// If it is an Enum, it holds the details. Otherwise, it's null.
        /// </summary>
        public Enumeration Enumeration { get; internal set; }

        /// <summary>
        /// Specified Type. In case it's an Enum, it's null.
        /// </summary>
        public string Type { get; private set; }

        /// <summary>
        /// Position of the field.
        /// </summary>
        public int Position { get; private set; }

        public string MapId { get; private set; }

        public override string ToString()
        {
            return string.Format("Name: {0}, Type: {1}, Position: {2}", this.Name, this.Type, this.Position);
        }
    }
}