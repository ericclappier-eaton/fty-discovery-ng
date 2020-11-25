MIBDIRS=../../../server/mibs/

snmprec --agent-udpv4-endpoint=$1 --protocol-version=1 --output-file=$1 --mib-source=$MIBDIRS
