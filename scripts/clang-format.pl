#!/usr/bin/env perl
# -*- mode: cperl; indent-tabs-mode: nil; tab-width: 3; cperl-indent-level: 3; -*-
use warnings;
use strict;
use utf8;

sub file_read {
   my ($name) = @_;
   local $/ = undef;
   open FILE, '<'.$name or die "Could not open $name: $!\n";
   my $data = <FILE>;
   close FILE;
   return $data;
}

sub file_write {
   my ($name,$data) = @_;
   open FILE, '>'.$name or die "Could not open $name: $!\n";
   print FILE $data;
   close FILE;
}

chdir('src/');
my @files = glob('*.c *.h *.cpp *.hpp');

foreach my $file (@files) {
   # Protect preprocessor directives due to https://llvm.org/bugs/show_bug.cgi?id=17362
   my $data = file_read($file);
   $data =~ s@#pragma once\n#ifndef@PRAGMA_ONCE_IFNDEF@g;
   $data =~ s@([ \t]*#if.+?#endif\n)@// clang-format off\n$1// clang-format on\n@sg;
   $data =~ s@\n// clang-format on\n// clang-format off\n@\n@g;
   $data =~ s@PRAGMA_ONCE_IFNDEF@#pragma once\n#ifndef@g;
   file_write($file, $data);

   `clang-format-6.0 -style=file -i '$file'`;

   my $data = file_read($file);
   $data =~ s@\n[^\n]*//[^\n]+clang-format (off|on)\n@\n@g; # Remove preprocessor protection
   # Things clang-format gets wrong:
   $data =~ s@([ \t]+)(BOOST_FOREACH|boost_foreach|reverse_foreach|foreach)([^{\n]*) \{[ \t]+([^}\n]*)[ \t]+\}@$1$2$3 {\n$1\t$4\n$1}@g; # Don't allow single-line foreach blocks
   $data =~ s@(operator .+?) \(@$1(@g; # No space before ( in operators
   $data =~ s@ \*([,>)])@*$1@g; # No space before statement-final *
   $data =~ s@^([ \t]*)(  [:,] [^\n]+) \{@$1$2\n$1\{@mg; # { after ctor-init should go on its own line
   $data =~ s@template <@template<@g; # No space in template<

   # clang-format horribly mangles enums if AlignConsecutiveAssignments is off, so fix that
   my @enums = ($data =~ m@enum [^{]*\{(.+?)\n\s*\}[^;\n]*;@sg);
   foreach my $enum (@enums) {
      my @lines = split /\n/, $enum;
      my $len = 0;
      foreach my $line (@lines) {
         if ($line =~ m@^(\s*\S+) = @) {
            if (length($1) > $len) {
               $len = length($1);
            }
         }
      }

      my @comb = ();
      foreach my $line (@lines) {
         if ($line =~ m@^(\s*\S+) = @) {
            my $sps = ' ' x (1 + $len - length($1));
            $line =~ s@ = @$sps= @;
         }
         $line =~ s@\(1 << (\d)\)@(1 <<  $1)@;
         push @comb, $line;
      }
      my $comb = join "\n", @comb;
      $data =~ s@\Q$enum\E@$comb@g;
   }

   # clang-format has no idea what I want with UOptions[], so fix that as well
   my @enums = ($data =~ m@UOption options\[\] = \{(.+?)\n\s*\}[^;\n]*;@sg);
   foreach my $enum (@enums) {
      my @lines = split /\n/, $enum;
      my $len = 0;
      foreach my $line (@lines) {
         if ($line =~ m@^([^"]+"[^"]+",)@) {
            if (length($1) > $len) {
               $len = length($1);
            }
         }
      }

      my @comb = ();
      foreach my $line (@lines) {
         if ($line =~ m@^([^"]+"[^"]+",)@) {
            my $txt = $1;
            my $sps = ' ' x (1 + $len - length($txt));
            my ($is) = ($txt =~ m@^(\s+)@);
            $line =~ s@\s+@ @g;
            $line =~ s@, 0, @,   0, @g;
            $line =~ s@UOPT_NO_ARG, @UOPT_NO_ARG,       @g;
            $line =~ s@^\s+@$is@;
            $line =~ s@(\Q$txt\E) @$1$sps@;
         }
         push @comb, $line;
      }
      my $comb = join "\n", @comb;
      $data =~ s@\Q$enum\E@$comb@g;
   }

   file_write($file, $data);
}
