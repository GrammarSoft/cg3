#!/bin/bash

cd /tmp
rm -fv dist-debian-ubuntu.pl
wget http://visl.sdu.dk/svn/visl/tools/vislcg3/trunk/dist/dist-debian-ubuntu.pl -O dist-debian-ubuntu.pl
chmod +x *.pl
./dist-debian-ubuntu.pl -m 'Apertium Automaton <apertium-packaging@lists.sourceforge.net>' -e 'Apertium Automaton <apertium-packaging@lists.sourceforge.net>'

rm -fv /var/cache/pbuilder/result/*

cd /tmp/cg3-debian.*
for DISTRO in wheezy jessie sid precise saucy trusty utopic
do
	for ARCH in i386 amd64
	do
		echo "Updating $DISTRO for $ARCH"
		cowbuilder --update --basepath /var/cache/pbuilder/base-$DISTRO-$ARCH.cow/
		echo "Building $DISTRO for $ARCH"
		cowbuilder --build *$DISTRO*.dsc --basepath /var/cache/pbuilder/base-$DISTRO-$ARCH.cow/
	done
done

for DISTRO in wheezy jessie sid
do
	reprepro -b /home/apertium/public_html/apt/debian/ includedeb $DISTRO /var/cache/pbuilder/result/*$DISTRO*.deb
done

for DISTRO in precise saucy trusty utopic
do
	reprepro -b /home/apertium/public_html/apt/ubuntu/ includedeb $DISTRO /var/cache/pbuilder/result/*$DISTRO*.deb
done

chown -R apertium:apertium /home/apertium/public_html/apt
