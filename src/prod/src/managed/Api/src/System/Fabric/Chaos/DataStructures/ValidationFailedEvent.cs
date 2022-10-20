// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Chaos.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// <para>Represents the Chaos event that is created when validation fails and encapsulates information about why validation failed.</para>
    /// </summary>
    [Serializable]
    public sealed class ValidationFailedEvent : ChaosEvent
    {
        private const string TraceType = "Chaos.ValidationFailedEvent";

        private string reason = string.Empty;

        internal ValidationFailedEvent()
        {
            base.Kind = "ValidationFailed";
        }

        internal ValidationFailedEvent(DateTime timeStampUtc, string reason)
            : base("ValidationFailed", timeStampUtc)
        {
            this.Reason = reason;
        }

        /// <summary>
        /// Gets the string representation of the reason for the validation failure
        /// </summary>
        [DataMember]
        public string Reason
        {
            get
            {
                return ChaosUtility.Decompress(this.reason);
            }

            private set
            {
                string temporary = ChaosUtility.Compress(value);
                if (temporary.Length > ChaosConstants.StringLengthLimit)
                {
                    this.reason = ChaosUtility.Compress(value.Substring(0, ChaosConstants.StringLengthLimit));
                }
                else
                {
                    this.reason = temporary;
                }
            }
        }

        /// <summary>
        /// Gets a string representation of the validation failed event.
        /// </summary>
        /// <returns>A string representation of the validation failed event.</returns>
        public override string ToString()
        {
            var description = new StringBuilder();
            description.AppendLine(base.ToString());
            description.AppendLine(string.Format(CultureInfo.InvariantCulture, "{0}", this.Reason));

            return description.ToString();
        }

        internal static unsafe ValidationFailedEvent FromNative(IntPtr nativeRaw)
        {
            if (nativeRaw == IntPtr.Zero)
            {
                return null;
            }

            var native = *(NativeTypes.FABRIC_VALIDATION_FAILED_EVENT*)nativeRaw;

            var timeStamp = NativeTypes.FromNativeFILETIME(native.TimeStampUtc);

            var reason = ChaosUtility.Decompress(NativeTypes.FromNativeString(native.Reason));

            return new ValidationFailedEvent(timeStamp, reason);
        }

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            string reasonToSendBack = null;

            if(ChaosUtility.DisableOptimizationForValidationFailedEvent)
            {
                reasonToSendBack = ChaosUtility.Decompress(this.reason);
                if(reasonToSendBack.Length > ChaosConstants.StringLengthLimit)
                {
                    reasonToSendBack = reasonToSendBack.Substring(0, ChaosConstants.StringLengthLimit);
                }

                reasonToSendBack = ChaosUtility.MakeLengthNotMultipleOfFour(reasonToSendBack);
            }
            else
            {
                reasonToSendBack = this.reason;
            }

            var nativeValidationFailedEvent = new NativeTypes.FABRIC_VALIDATION_FAILED_EVENT
                                                  {
                                                      TimeStampUtc = NativeTypes.ToNativeFILETIME(this.TimeStampUtc),
                                                      Reason = pinCollection.AddObject(reasonToSendBack)
                                                  };

            return pinCollection.AddBlittable(nativeValidationFailedEvent);
        }

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        /// <exception cref="EndOfStreamException">The end of the stream is reached. </exception>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Read(BinaryReader br)
        {
            this.ReadInheritedMembers(br);

            this.reason = br.ReadString();
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            this.WriteInheritedMembers(bw);

            bw.Write(this.reason);
        }
    }
}