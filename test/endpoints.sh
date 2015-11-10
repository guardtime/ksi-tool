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

if [[ ! $KSI_TCP_AGGREGATOR ]]; then
	export KSI_TCP_AGGREGATOR="ksi+tcp://ksigw.test.guardtime.com:3332";
	echo "KSI_TCP_AGGREGATOR set: " $KSI_TCP_AGGREGATOR
	else echo $KSI_TCP_AGGREGATOR;
fi

if [[ ! $KSI_TCP_LOGIN ]]; then
	export KSI_TCP_LOGIN="--user anon --pass anon";
	echo "KSI_TCP_LOGIN set: " $KSI_TCP_LOGIN
	else echo $KSI_TCP_LOGIN;
fi