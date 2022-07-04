#!/bin/bash
# This script is used by the author to make a tar.gz package

VERSION=`cat src/version.h | tr -d '"' | awk '/smsd_version/ {print $3}'`
PACKAGE=smstools-$VERSION.tar.gz

chmod -R a+rX *
tar -chzf $PACKAGE --exclude='*~' --exclude='*.bak' --exclude=".svn" --exclude='book' --exclude='*.tar.gz' .
echo "Package $PACKAGE created"
