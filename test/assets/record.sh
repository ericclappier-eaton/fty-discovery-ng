MIBDIRS=../../server/mibs/

oid=`snmptranslate -On -m ALL -M $MIBDIRS $2`
echo $oid

snmprec --agent-udpv4-endpoint=$1 --protocol-version=1 --start-object=$oid --output-file=$1 --mib-source=$MIBDIRS
