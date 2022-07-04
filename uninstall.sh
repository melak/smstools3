#!/bin/sh
#Do not run directly. This is a helper script for make.

echo "You are going to delete all files from the SMS Server Tools."
echo "This script deletes also the config file and stored messages."
echo "Are you sure to proceed? [yes/no]"
read answer
if [ "$answer" != "yes" ]; then
  exit 1
fi

echo "Deleting binary program files"
rm /usr/local/bin/smsd
rm /usr/local/bin/putsms
rm /usr/local/bin/getsms

echo "Deleting some scripts"
[ -f /usr/local/bin/pkill ] && echo "skipped /usr/local/bin/pkill, other programs might need it."
rm /usr/local/bin/sendsms
rm /usr/local/bin/sms2html
rm /usr/local/bin/sms2unicode
rm /usr/local/bin/unicode2sms

echo "Deleting config file"
rm /etc/smsd.conf

echo "Deleting start-script"
[ -d /etc/init.d ] && rm /etc/init.d/sms 
[ -d /sbin/init.d ] && rm /sbin/init.d/sms

echo "Deleting spool directories"
rm -R /var/spool/sms

