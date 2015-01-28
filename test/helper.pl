#!/usr/bin/perl
use strict;
use warnings;

my $binary = $ARGV[0];
my $args = '';
if ($ARGV[2]) {
   $args = $ARGV[2];
}

`"$binary" $args $ARGV[1] -g grammar.cg3 -I input.txt -O output.txt >stdout.txt 2>stderr.txt`;
`diff -B expected.txt output.txt >diff.txt`;

if (-s "diff.txt") {
	print STDERR "Fail ";
} else {
	print STDERR "Success ";
}

`"$binary" $args $ARGV[1] -g grammar.cg3 --grammar-only --grammar-bin grammar.cg3b >stdout.bin.txt 2>stderr.bin.txt`;
my $gf = undef;
for my $g (glob('*.cg3b*')) {
   `"$binary" $args $ARGV[1] -g '$g' -I input.txt -O output.bin.txt >>stdout.bin.txt 2>>stderr.bin.txt`;
   `diff -B expected.txt output.bin.txt >diff.bin.txt`;
   if (-s "diff.bin.txt") {
      $gf = $g;
      last;
   }
}

if (-s "diff.bin.txt") {
	print STDERR "Fail ($gf).\n";
} else {
	print STDERR "Success.\n";
}
