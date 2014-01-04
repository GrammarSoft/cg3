#!/usr/bin/perl
use strict;
use warnings;

print `svn up --ignore-externals`;
my $revision = `svnversion -n`;
$revision =~ s/^([0-9]+).*/$1/g;
$revision += 1;

`/usr/bin/perl -e 's/CG3_REVISION = [0-9]+;\$/CG3_REVISION = $revision;/;' -pi src/version.hpp`;
`/usr/bin/perl -e 's/\$revision = [0-9]+;\$/\$revision = $revision;/;' -pi scripts/cg3-autobin.pl`;
`svn2cl --group-by-day -i --reparagraph --limit 350`;

print "Set revision to $revision.\n";
