#!/usr/bin/perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

my $binary = "../../../src/cg-proc";
my $compiler = "../../../src/cg-comp";

`"$compiler" grammar.txt grammar.bin >/dev/null 2>&1`;
`"$binary" -d grammar.bin  input.txt output.txt >stdout.txt 2>stderr.txt`;
`diff -B expected.txt output.txt >diff.txt`;

if (-s "diff.txt") {
	print STDERR "Fail.\n";
} else {
	print STDERR "Success.\n";
}
