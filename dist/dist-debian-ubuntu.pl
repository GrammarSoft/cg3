#!/usr/bin/env perl
# -*- mode: cperl; indent-tabs-mode: nil; tab-width: 3; cperl-indent-level: 3; -*-
use utf8;
use strict;
use warnings;
BEGIN {
	$| = 1;
	binmode(STDIN, ':encoding(UTF-8)');
	binmode(STDOUT, ':encoding(UTF-8)');
}
use open qw( :encoding(UTF-8) :std );

use Getopt::Long;
my %opts = (
	'm' => 'Tino Didriksen <mail@tinodidriksen.com>',
	'e' => 'Tino Didriksen <mail@tinodidriksen.com>',
);
GetOptions(
	'm=s' => \$opts{'m'},
	'e=s' => \$opts{'e'},
);

my %distros = (
	'wheezy' => 'debian',
	'jessie' => 'debian',
	'sid' => 'debian',
	'precise' => 'ubuntu',
	'saucy' => 'ubuntu',
	'trusty' => 'ubuntu',
	'utopic' => 'ubuntu',
);

print `rm -rf /tmp/cg3-debian.*`;
print `mkdir -pv /tmp/cg3-debian.$$`;
chdir "/tmp/cg3-debian.$$" or die "Could not change folder: $!\n";

print `svn export http://visl.sdu.dk/svn/visl/tools/vislcg3/trunk/src/version.hpp`;
my $major = 0;
my $minor = 0;
my $patch = 0;
my $revision = 0;
{
	local $/ = undef;
	open FILE, 'version.hpp' or die "Could not open version.hpp: $!\n";
	my $data = <FILE>;
	($major,$minor,$patch,$revision) = ($data =~ m@CG3_VERSION_MAJOR = (\d+);.*?CG3_VERSION_MINOR = (\d+);.*?CG3_VERSION_PATCH = (\d+);.*?CG3_REVISION = (\d+);@s);
	close FILE;
}

my $version = "$major.$minor.$patch.$revision";
my $date = `date -u -R`;

print `svn export http://visl.sdu.dk/svn/visl/tools/vislcg3/trunk/ 'cg3-$version'`;
print `tar -zcvf 'cg3_$version.orig.tar.gz' 'cg3-$version'`;

foreach my $distro (keys %distros) {
	my $chver = $version;
	if ($distros{$distro} eq 'ubuntu') {
		$chver .= "-ubuntu1~".$distro."1";
	}
	else {
		$chver .= "-1~".$distro."1";
	}
	my $chlog = <<CHLOG;
cg3 ($chver) $distro; urgency=low

  * Automatic build - see changelog at http://visl.sdu.dk/svn/visl/tools/vislcg3/trunk/ChangeLog

 -- $opts{e}  $date
CHLOG

	`cp -al 'cg3-$version' 'cg3-$chver'`;
	open FILE, ">cg3-$chver/debian/changelog" or die "Could not write to debian/changelog: $!\n";
	print FILE $chlog;
	close FILE;
	print `dpkg-source '-DMaintainer=$opts{m}' '-DUploaders=$opts{e}' -b 'cg3-$chver'`;
	#print `debsign 'cg3_$chver.dsc'`;
	chdir "cg3-$chver";
	print `dpkg-genchanges -S -sa '-m$opts{m}' '-e$opts{e}' > '../cg3_$chver\_source.changes'`;
	chdir '..';
	print `debsign 'cg3_$chver\_source.changes'`;
}

chdir "/tmp";
#print `rm -rf /tmp/cg3-debian.$$`;