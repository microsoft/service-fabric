// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.IO;
using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    public class EnvoyClusterSslContext
    {
        static string ca_cert_file_path = "/var/lib/sfreverseproxycerts/servicecacert.pem";
        public EnvoyClusterSslContext(string verify_certificate_hash, List<string> verify_subject_alt_name)
        {
            this.verify_certificate_hash = verify_certificate_hash;
            this.verify_subject_alt_name = verify_subject_alt_name;
            if (File.Exists(ca_cert_file_path)) // change to check for existance of file
            {
                ca_cert_file = ca_cert_file_path;
            }
        }

        [JsonProperty]
        public string cert_chain_file = "/var/lib/sfreverseproxycerts/reverseproxycert.pem";

        [JsonProperty]
        public string private_key_file = "/var/lib/sfreverseproxycerts/reverseproxykey.pem";

        [JsonProperty]
        public string verify_certificate_hash;
        public bool ShouldSerializeverify_certificate_hash()
        {
            return verify_certificate_hash != null;
        }

        [JsonProperty]
        public List<string> verify_subject_alt_name;
        public bool ShouldSerializeverify_subject_alt_name()
        {
            return verify_subject_alt_name != null && verify_subject_alt_name.Count != 0;
        }

        [JsonProperty]
        public string ca_cert_file;
        public bool ShouldSerializeca_cert_file()
        {
            return ca_cert_file != null;
        }
    }
}
