#!/usr/bin/perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

my $binary = $ARGV[0];
if (!$binary || $binary eq '' || !(-x $binary)) {
	die("Error: $binary is not executable!");
}

`"$binary" --grammar grammar.txt -I input.txt -O output.txt >stdout.txt 2>stderr.txt`;
`diff -B expected.txt output.txt >diff.txt`;

if (-s "diff.txt") {
	print STDERR "Fail.\n";
} else {
	print STDERR "Success.\n";
}
