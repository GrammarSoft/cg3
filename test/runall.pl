#!/usr/bin/env perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

# Search paths for the binary
my @binlist = (
	"../../build/VS15/src/Debug/vislcg3",
	"../../build/VS15/src/Release/vislcg3",
	"../../build/VS14/src/Debug/vislcg3",
	"../../build/VS14/src/Release/vislcg3",
	"../src/Debug/vislcg3",
	"../src/Release/vislcg3",
	"../Debug/vislcg3",
	"../Release/vislcg3",
	"../src/vislcg3",
	"../vislcg3",
	);
my @unlinks = (
   'diff.txt',
   'output.txt',
   'grammar.cg3b',
   'diff.bin.txt',
   'output.bin.txt',
   );
my $binary = "vislcg3";

sub run_pl {
   my ($binary,$override,$args) = @_;

   `"$binary" $args $override -g grammar.cg3 -I input.txt -O output.txt >stdout.txt 2>stderr.txt`;
   `diff -B expected.txt output.txt >diff.txt`;

   if (-s "diff.txt") {
      print STDERR "Fail ";
   } else {
      print STDERR "Success ";
   }

   `"$binary" $args $override -g grammar.cg3 --grammar-only --grammar-bin grammar.cg3b >stdout.bin.txt 2>stderr.bin.txt`;
   my $gf = undef;
   for my $g (glob('*.cg3b*')) {
      `"$binary" $args $override -g '$g' -I input.txt -O output.bin.txt >>stdout.bin.txt 2>>stderr.bin.txt`;
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
}

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
print STDERR "Binary found at: $binary\n";

print STDERR "\nRunning tests...\n";

my @tests = grep { -x } glob('./T_*');
foreach (@tests) {
	if ($ARGV[0] && $ARGV[0] ne "" && !(/$ARGV[0]/i)) {
		next;
	}
	chdir $bindir or die("Error: Could not change directory to $bindir !");
	my ($test) = m/^.*?(T[^\/]+).*$/;
	chdir $test or die("Error: Could not change directory to $test !");
	print STDERR "$test: ";
	for (my $i=length $test;$i<30;$i++) {
		print STDERR " ";
	}
	if (-s "./$test/byline.txt") {
		print STDERR "(".`cat "./$test/byline.txt"`.") ";
	}
	for my $u (@unlinks) {
	   if (-e $u) {
	      unlink $u;
	   }
	}

	my $c = '""';
	if ($ARGV[1] && $ARGV[1] ne "") {
		$c = '"'.$ARGV[1].'"';
	}
	my $args = '';
	if (-s 'args.txt') {
	   $args = `cat args.txt`;
	}
	if (-x 'run.pl') {
	   `./run.pl "$binary" \Q$c\E $args`;
	}
	else {
	   run_pl($binary, $c, $args);
	}
}

print STDERR "\n";
