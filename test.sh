#!/bin/bash

reset() 
{    
    rm -rf mail.store
}

logfile=log.$$.txt
if [ "$1" != "" ] ; then
    pattern=in.s.$1
else
    pattern=in.s.?
fi
for i in $pattern ; do
    echo Running test $i
    expfile=$(echo $i | sed -e 's/in\./exp./')
    outfile=$(echo $i | sed -e 's/in\./out./')
    expmailstore=$expfile.mail.store
    reset
    pkill mysmtpd
    ./mysmtpd 5005 >& $logfile &
    sleep 1
    nc localhost 5005 < $i | tr -d '\015' > $outfile
    sleep 1
    pkill mysmtpd
    if diff -c $expfile $outfile ; then
	rm -f $outfile
    fi
    rm -f $logfile
    if [ -d $expmailstore ] ; then
	if [ -d mail.store ] ; then
	    diff -r -c $expmailstore mail.store
	else
	    echo "Test expected mail to be created in mail.store, but none was created."
	fi
    fi
done
