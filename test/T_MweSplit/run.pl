#!/usr/bin/env perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

my $binary_mwesplit = $ARGV[0];
$binary_mwesplit =~ s@/vislcg3([^/]*)$@/cg-mwesplit$1@;
if (!$binary_mwesplit || $binary_mwesplit eq '' || !(-x $binary_mwesplit)) {
	die("Error: $binary_mwesplit is not executable!");
}

`"$binary_mwesplit" < input.txt > output.txt 2>>stderr.txt`;
`diff -ZB expected.txt output.txt >diff.txt`;

if (-s "diff.txt") {
	print STDERR "Fail.\n";
} else {
	print STDERR "Success Success.\n";
}
