#!/usr/bin/env perl
use strict;
use warnings;
use Cwd qw(realpath);
use FindBin qw($Bin);

my $bindir = realpath $Bin;
chdir $bindir or die("Error: Could not change directory to $bindir !");

# Search paths for the binary
my @binlist = (
	"../build/src/vislcg3",
	"../../build/VS17/src/Debug/vislcg3",
	"../../build/VS17/src/Release/vislcg3",
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
	'diff.bsf.txt',
	'expected.bsf.txt',
	'output.bsf.txt',
	'grammar.out.cg3',
	'diff.out.txt',
	'output.out.txt',
	'untraced.txt',
	'untraced.out.txt',
	);
my $binary = "vislcg3";

sub run_pl {
	my ($binary,$override,$args,$bsfargs) = @_;
	my $good = 1;

	# Normal run
	`"$binary" $args $override -g grammar.cg3 -I input.txt -O output.txt >stdout.txt 2>stderr.txt`;
	`diff -B expected.txt output.txt >diff.txt`;

	if (-s "diff.txt") {
		print STDERR "Fail ";
		$good = 0;
	} else {
		print STDERR "Success ";
	}

	# Write out the parsed grammar, run from the output
	`cat expected.txt | $bindir/../scripts/cg-untrace > untraced.txt`;
	`"$binary" $args $override -g grammar.cg3 --grammar-only --grammar-out grammar.out.cg3 >stdout.out.txt 2>stderr.out.txt`;
	`"$binary" $args $override -g grammar.out.cg3 -I input.txt -O output.out.txt >>stdout.out.txt 2>>stderr.out.txt`;
	`cat output.out.txt | $bindir/../scripts/cg-untrace > untraced.out.txt`;
	`diff -B untraced.txt untraced.out.txt >diff.out.txt`;

	if (-s "diff.out.txt") {
		print STDERR "Fail ";
		$good = 0;
	} else {
		print STDERR "Success ";
	}

	# Compile the grammar, run from the compiled form
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
		print STDERR "Fail ($gf) ";
		$good = 0;
	} else {
		print STDERR "Success ";
	}

	# Normal run, but with binary I/O
	my $conv = $binary;
	$conv =~ s@vislcg3(\.exe)?$@cg-conv@g;
	`cat input.txt | "$conv" --in-cg --out-binary $bsfargs 2>stderr.bsf.conv1.txt | "$binary" $args $override -g grammar.cg3 --in-binary --out-binary 2>stderr.bsf.vislcg3.txt | "$conv" --in-binary --out-cg 2>stderr.bsf.conv2.txt | "$bindir/../scripts/cg-sort" -m | grep -v '<STREAMCMD:FLUSH>' >output.bsf.txt`;
	`cat expected.txt | $bindir/../scripts/cg-untrace | "$bindir/../scripts/cg-sort" -m > expected.bsf.txt`;
	`diff -B expected.bsf.txt output.bsf.txt >diff.bsf.txt`;

	if (-s "diff.bsf.txt") {
		print STDERR "Fail";
		$good = 0;
	} else {
		print STDERR "Success";
	}

	print STDERR "\n";
	return $good;
}

foreach (@binlist) {
	if (-x $_ && int(`$_ --min-binary-revision 2>/dev/null`) >= 10373) {
		$binary = $_;
		last;
	}
	elsif (-x $_.".exe" && int(`$_.exe --min-binary-revision 2>/dev/null`) >= 10373) {
		$binary = $_.".exe";
		last;
	}
	elsif (-x "../".$_ && int(`../$_ --min-binary-revision 2>/dev/null`) >= 10373) {
		$binary = "../".$_;
		last;
	}
	elsif (-x "../".$_.".exe" && int(`../$_.exe --min-binary-revision 2>/dev/null`) >= 10373) {
		$binary = "../".$_.".exe";
		last;
	}
}
$binary = realpath $binary;
print STDERR "Binary found at: $binary\n";

print STDERR "\nRunning tests...\n";

my $bad = 0;

my $total = 0;
my $failed = 0;

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
	my $bsfargs = '';
	if (-s 'bsfargs.txt') {
		$bsfargs = `cat bsfargs.txt`;
	}
	if (-x 'run.pl') {
		`./run.pl "$binary" \Q$c\E $args`;
		if ($?) {
			$bad = 1;
			$failed += 1;
		}
	}
	else {
		if (!run_pl($binary, $c, $args, $bsfargs)) {
			$bad = 1;
			$failed += 1;
		}
	}
	$total += 1;
}

print STDERR "\n";

my $success = $total - $failed;
print STDERR "$success / $total tests passing\n";

exit($bad);
