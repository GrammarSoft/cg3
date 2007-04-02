#!/usr/bin/perl
use strict;
use warnings;

print `svn up`;
my $revision = `svnversion -n`;
$revision =~ s/^([0-9]+).*/$1/g;
$revision += 1;

`/usr/bin/perl -e 's/CG3_REVISION [0-9]+\$/CG3_REVISION $revision/;' -pi src/version.h`;

print "Set revision to $revision.\n";
