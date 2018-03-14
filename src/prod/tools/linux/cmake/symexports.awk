#!/usr/bin/awk -f
BEGIN {
    print "V1.0 {";
    print "    global:";
}
{
    gsub(/\r/,"", $0);

    if (NF && !match($0, /^[:space:]*;/) && !match($0, /^LIBRARY.*/) && !match($0, /^EXPORTS.*/))
    {
        print "         "  $0 ";";
    }
}
END {
    print "    local: *;"
    print "};";
}

