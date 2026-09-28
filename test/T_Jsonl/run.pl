#!/usr/bin/env perl
use strict;
use warnings;
use Cwd qw(realpath);

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

my $binary = $ARGV[0];
my $binary_conv = $binary;
$binary_conv =~ s@/vislcg3([^/]*)$@/cg-conv$1@;
if (!$binary_conv || $binary_conv eq '' || !(-x $binary_conv)) {
	die("Error: $binary_conv is not executable!");
}

# Each case writes output.NAME.txt and stderr.NAME.txt, and is compared with expected.NAME.txt (and expected-stderr.NAME.txt if present)
my @cases = (
	['cg-to-jsonl', qq{"$binary_conv" -c -D --deleted -J < input.txt}],
	['jsonl-to-cg', qq{"$binary_conv" -j -D --deleted -C < expected.cg-to-jsonl.txt}],
	['jsonl-to-jsonl', qq{"$binary_conv" -j -D --deleted -J < expected.cg-to-jsonl.txt}],
	['no-deleted', qq{"$binary_conv" -j -D -C < expected.cg-to-jsonl.txt}],
	['rules', qq{"$binary" -g grammar.cg3 --in-jsonl --out-jsonl < input-rules.jsonl}],
	['rules-trace', qq{"$binary" -g grammar.cg3 --in-jsonl --out-jsonl --trace < input-rules.jsonl}],
	['rules-trace-cg', qq{"$binary" -g grammar.cg3 --in-jsonl --out-cg --trace < input-rules.jsonl}],
	['errors', qq{"$binary_conv" -j -J < input-errors.jsonl}],
);

my $bad = 0;
for my $case (@cases) {
	my ($name, $cmd) = @$case;
	`$cmd >output.$name.txt 2>stderr.$name.txt`;
	`diff expected.$name.txt output.$name.txt >diff.$name.txt`;
	if (-e "expected-stderr.$name.txt") {
		`diff expected-stderr.$name.txt stderr.$name.txt >>diff.$name.txt`;
	}
	if (-s "diff.$name.txt") {
		print STDERR "Fail($name) ";
		$bad = 1;
	}
	else {
		print STDERR "Success ";
	}
}
print STDERR "\n";

exit($bad);
