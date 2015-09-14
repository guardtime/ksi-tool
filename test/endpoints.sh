if [[ ! $KSI_AGGREGATOR ]]; then
	export KSI_AGGREGATOR="url=http://ksigw.test.guardtime.com:3333/gt-signingservice";
	echo "KSI_AGGREGATOR set: " $KSI_AGGREGATOR
	else echo $KSI_AGGREGATOR;
fi

if [[ ! $KSI_EXTENDER ]]; then
	export KSI_EXTENDER="url=http://ksigw.test.guardtime.com:8010/gt-extendingservice";
	echo "KSI_EXTENDER set: " $KSI_EXTENDER
	else echo $KSI_EXTENDER;
fi

if [[ ! $KSI_PUBFILE ]]; then
	export KSI_PUBFILE="url=http://verify.guardtime.com/ksi-publications.bin 1.2.840.113549.1.9.1=publications@guardtime.com";
	echo "KSI_PUBFILE set: " $KSI_PUBFILE
	else echo $KSI_PUBFILE;
fi