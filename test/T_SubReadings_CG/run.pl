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

`"$binary" $ARGV[1] --trace -C UTF-8 --grammar grammar.cg3 -I input.txt -O output.txt >stdout.txt 2>stderr.txt`;
`diff -B -b expected.txt output.txt >diff.txt`;

if (-s "diff.txt") {
	print STDERR "Fail ";
} else {
	print STDERR "Success ";
}

`"$binary" $ARGV[1] -C UTF-8 --grammar grammar.cg3 --grammar-only --grammar-bin grammar.cg3b >stdout.bin.txt 2>stderr.bin.txt`;
`"$binary" $ARGV[1] --trace -C UTF-8 --grammar grammar.cg3b -I input.txt -O output.bin.txt >>stdout.bin.txt 2>>stderr.bin.txt`;
`diff -B -b expected.txt output.bin.txt >diff.bin.txt`;

if (-s "diff.bin.txt") {
	print STDERR "Fail.\n";
} else {
	print STDERR "Success.\n";
}
