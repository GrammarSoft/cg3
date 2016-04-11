#!/usr/bin/perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

my $bpath = $ARGV[0];
my $binary = $bpath."cg-proc";
my $compiler = $bpath."cg-comp";

`"$compiler" grammar.cg3 grammar.bin  >stdout.txt 2>stderr.txt`;
`"$binary" -d grammar.bin  input.txt output.txt >>stdout.txt 2>>stderr.txt`;
`diff -B expected.txt output.txt >diff.txt`;

if (-s "diff.txt") {
	print STDERR "Fail.\n";
} else {
	print STDERR "Success.\n";
}
