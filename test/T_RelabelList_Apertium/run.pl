#!/usr/bin/env perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

my $binary_comp = $ARGV[0];
$binary_comp =~ s@/vislcg3([^/]*)$@/cg-comp$1@;
if (!$binary_comp || $binary_comp eq '' || !(-x $binary_comp)) {
	die("Error: $binary_comp is not executable!");
}

my $binary_relabel = $ARGV[0];
$binary_relabel =~ s@/vislcg3([^/]*)$@/cg-relabel$1@;
if (!$binary_relabel || $binary_relabel eq '' || !(-x $binary_relabel)) {
	die("Error: $binary_relabel is not executable!");
}

my $binary_proc = $ARGV[0];
$binary_proc =~ s@/vislcg3([^/]*)$@/cg-proc$1@;
if (!$binary_proc || $binary_proc eq '' || !(-x $binary_proc)) {
	die("Error: $binary_proc is not executable!");
}

my @unlinks = (
   'grammar.cg3b',
   'grammar-out.cg3b',
);
for my $u (@unlinks) {
        if (-e $u) {
                unlink $u;
        }
}

`"$binary_comp" grammar.cg3 grammar.cg3b >stdout.txt 2>stderr.txt`;
`"$binary_relabel" grammar.cg3b relabel.cg3r grammar-out.cg3b >>stdout.txt 2>>stderr.txt`;

if (-s "grammar.cg3b" && -s "grammar-out.cg3b") {
	print STDERR "Success ";
} else {
	print STDERR "Fail ";
}

`"$binary_proc" grammar-out.cg3b input.txt output.txt 2>>stderr.txt`;
`diff -B expected.txt output.txt >diff.txt`;

if (-s "diff.txt") {
	print STDERR "Fail.\n";
} else {
	print STDERR "Success.\n";
}
