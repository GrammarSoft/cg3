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
	"../Common/vislcg3",
	"../Release/vislcg3",
	"../src/vislcg3",
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
	elsif (-x "../".$_) {
		$binary = "../".$_;
		last;
	}
	elsif (-x "../".$_.".exe") {
		$binary = "../".$_.".exe";
		last;
	}
}
$binary = realpath $binary;
$binary =~ s@/([^/]+)$@/@;
print STDERR "Binaries found at: $binary\n";

print STDERR "\nRunning tests...\n";

my @tests = grep { -x } glob('./T*/run.pl');
foreach (@tests) {
	if ($ARGV[0] && $ARGV[0] ne "" && !(/$ARGV[0]/i)) {
		next;
	}
	chdir $bindir or die("Error: Could not change directory to $bindir !");
	my ($test) = m/^.*?(T[^\/]+).*$/;
	print STDERR "$test: ";
	if (-s "./$test/byline.txt") {
		print STDERR "(".`cat "./$test/byline.txt"`.") ";
	}
	if (-e "./".$test."/diff.txt") {
	    unlink "./".$test."/diff.txt";
	}
	if (-e "./".$test."/output.txt") {
	    unlink "./".$test."/output.txt";
	}
	my $c = '""';
	if ($ARGV[1] && $ARGV[1] ne "") {
		$c = '"'.$ARGV[1].'"';
	}
	`$_ "$binary" \Q$c\E`;
}

print STDERR "\n";
