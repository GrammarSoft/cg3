#!/usr/bin/perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

# Search paths for the binary
my @binlist = (
	"../Debug/vislcg3",
	"../Release/vislcg3",
	"../Debug/cg3combined",
	"../Release/cg3combined",
	"../cg3combined",
	"../vislcg3",
);
my $binary = "vislcg3";

foreach (@binlist) {
	if (-x $_) {
		$binary = $_;
		last;
	}
	elsif (-x $_.".exe") {
		$binary = $_.".exe";
		last;
	}
}
$binary = realpath $binary;
print STDERR "Set binary to: $binary\n";

print STDERR "\nRunning tests...\n";

my @tests = grep { -x } glob('./T*/run.*');
foreach (@tests) {
	chdir $bindir or die("Error: Could not change directory to $bindir !");
	my ($test) = m/^.*(T[^\/]+).*$/;
	print STDERR "$test: ";
	`$_ "$binary"`;
}

print STDERR "\n";
