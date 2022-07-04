#!/bin/sh
#Do not run directly. This is a helper script for make.

# Remember to edit the config file if you change these default path's.
BINDIR=/usr/local/bin
SPOOLDIR=/var/spool/sms



copy()
{
  if [ -f $2 ]; then
    echo "  Skipped $2, file already exists"
  else  
    echo "  $2"
    cp $1 $2
  fi        
}

forcecopy()
{
  if [ -f $2 ]; then
    echo "  Overwriting $2"
    cp $1 $2
  else
    echo "  $2"
    cp $1 $2
  fi
}

delete()
{
  if [ -f $1 ]; then
    echo " Deleting $1"
    rm $1
  fi
}

makedir()
{
  if [ -d $1 ]; then
    echo "  Skipped $1, directory already exists"
  else
    echo "  Creating directory $1"
    mkdir $1
  fi
}

echo ""
if [ ! -f src/smsd ] && [ ! -f src/smsd.exe ]; then 
  echo 'Please run "make -s install" instead.'
  exit 1
fi

echo "Installing binary program files"
if [ -f src/smsd.exe ]; then
  forcecopy src/smsd.exe $BINDIR/smsd.exe
else
  forcecopy src/smsd $BINDIR/smsd
fi
delete $BINDIR/getsms
delete $BINDIR/putsms

echo "Installing some scripts"
copy scripts/sendsms $BINDIR/sendsms
copy scripts/sms2html $BINDIR/sms2html
copy scripts/sms2unicode $BINDIR/sms2unicode
copy scripts/unicode2sms $BINDIR/unicode2sms

echo "Installing config file"
copy examples/smsd.conf.easy /etc/smsd.conf

echo "Creating minimum spool directories"
makedir $SPOOLDIR
makedir $SPOOLDIR/incoming
makedir $SPOOLDIR/outgoing
makedir $SPOOLDIR/checked

echo "Installing start-script"
if [ -d /etc/init.d ]; then
  copy scripts/sms /etc/init.d/sms 
elif [ -d /sbin/init.d ]; then
   copy scripts/sms /sbin/init.d/sms
else
  echo "  I do not know where to copy scripts/sms. Please find out yourself."
fi

echo ""
echo "Example script files are not installed automatically."
echo 'Please dont forget to edit /etc/smsd.conf.'
