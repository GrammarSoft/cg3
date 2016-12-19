#!/usr/bin/env perl
# -*- mode: cperl; indent-tabs-mode: nil; tab-width: 3; cperl-indent-level: 3; -*-
package CG3_External;
use strict;
use warnings;
use utf8;
#use feature 'unicode_strings';

our $VERSION = 7226;
our @EXPORT_OK;

BEGIN {
   $| = 1;
   use Exporter ();
   our (@ISA, @EXPORT_OK);

   @ISA = qw(Exporter);

   @EXPORT_OK = qw(&check_protocol &read_window &write_window &write_null_response);
}

sub read_uint16_t {
   my ($fh,$msg) = @_;

   my $n = 0;
   if (read($fh, $n, 2) != 2) {
      return undef;
   }
   $n = unpack('S', $n);
   if ($ENV{'DEBUG'}) { print STDERR "Read $msg uint16_t: $n\n"; }
   return $n;
}

sub write_uint16_t {
   my ($fh,$n,$msg) = @_;

   if ($ENV{'DEBUG'}) { print STDERR "Writing $msg uint16_t: $n\n"; }
   $n = pack('S', $n);
   print $fh $n;
}

sub read_uint32_t {
   my ($fh,$msg) = @_;

   my $n = 0;
   if (read($fh, $n, 4) != 4) {
      return undef;
   }
   $n = unpack('L', $n);
   if ($ENV{'DEBUG'}) { print STDERR "Read $msg uint32_t: $n\n"; }
   return $n;
}

sub write_uint32_t {
   my ($fh,$n,$msg) = @_;

   if ($ENV{'DEBUG'}) { print STDERR "Writing $msg uint32_t: $n\n"; }
   $n = pack('L', $n);
   print $fh $n;
}

sub read_utf8_string {
   my ($fh,$msg) = @_;

   my $n = read_uint16_t($fh, $msg.' length');
   my $s = '';
   if (read($fh, $s, $n) != $n) {
      return undef;
   }

   utf8::decode($s);

   if ($ENV{'DEBUG'}) { print STDERR "Read $msg UTF-8 string: $s\n"; }
   return $s;
}

sub write_utf8_string {
   my ($fh,$s,$msg) = @_;

   utf8::encode($s);

   my $n = length($s);
   write_uint16_t($fh, $n, $msg.' length');
   if ($ENV{'DEBUG'}) { utf8::decode($s); print STDERR "Writing $msg UTF-8 string: $s\n"; utf8::encode($s); }
   print $fh $s;
}

sub check_protocol {
   my ($fh) = @_;

   my $protocol = read_uint32_t($fh, 'protocol revision');
   if ($protocol == $VERSION) {
      return $VERSION;
   }

   return undef;
}

sub read_window {
   my ($fh) = @_;

   my $len = read_uint32_t($fh, 'window packet length');
   if (!$len) {
      return undef;
   }

   my %window = ();
   $window{'num'} = read_uint32_t($fh, 'window number');

   my @cohorts;
   my $clen = read_uint32_t($fh, 'num cohorts');
   for (my $c=0 ; $c<$clen ; $c++) {
      read_uint32_t($fh, 'cohort packet length');
      my %cohort = ();
      $cohort{'num'} = read_uint32_t($fh, 'cohort number');
      $cohort{'flags'} = read_uint32_t($fh, 'cohort flags');
      if ($cohort{'flags'} & (1 << 1)) {
         $cohort{'parent'} = read_uint32_t($fh, 'cohort parent');
      }
      $cohort{'wordform'} = read_utf8_string($fh, 'cohort wordform');

      my @readings;
      my $rlen = read_uint32_t($fh, 'num readings');
      for (my $r=0 ; $r<$rlen ; $r++) {
         read_uint32_t($fh, 'reading packet length');
         my %reading = ();
         $reading{'flags'} = read_uint32_t($fh, 'reading flags');

         if ($reading{'flags'} & (1 << 3)) {
            $reading{'baseform'} = read_utf8_string($fh, 'reading baseform');
         }

         my @tags;
         my $tlen = read_uint32_t($fh, 'num tags');
         for (my $t=0 ; $t<$tlen ; $t++) {
            my $tag = read_utf8_string($fh, 'tag');
            push @tags, $tag;
         }
         $reading{'tags'} = \@tags;

         push @readings, \%reading;
      }
      $cohort{'readings'} = \@readings;

      if ($cohort{'flags'} & (1 << 0)) {
         $cohort{'text'} = read_utf8_string($fh, 'cohort text');
      }

      push @cohorts, \%cohort;
   }
   $window{'cohorts'} = \@cohorts;

   return \%window;
}

sub write_window {
   my ($fh,$w) = @_;

   if ($ENV{'DEBUG'} && $ENV{'DEBUG'} > 1) { write_uint32_t($fh, $VERSION, 'debug version'); }

   open(my $fw, '>', \my $wo);
   write_uint32_t($fw, $w->{'num'}, 'window number');

   my $clen = @{$w->{'cohorts'}};
   write_uint32_t($fw, $clen, 'num cohorts');

   foreach my $c (@{$w->{'cohorts'}}) {
      open(my $fc, '>', \my $co);

      write_uint32_t($fc, $c->{'num'}, 'cohort number');
      if ($c->{'text'}) {
         $c->{'flags'} |= (1 << 0);
      }
      if (exists($c->{'parent'})) {
         $c->{'flags'} |= (1 << 1);
      }
      write_uint32_t($fc, $c->{'flags'}, 'cohort flags');
      if (exists($c->{'parent'})) {
         write_uint32_t($fc, $c->{'parent'}, 'cohort parent');
      }
      write_utf8_string($fc, $c->{'wordform'}, 'cohort wordform');

      my $rlen = @{$c->{'readings'}};
      write_uint32_t($fc, $rlen, 'num readings');

      foreach my $r (@{$c->{'readings'}}) {
         open(my $fr, '>', \my $ro);
         if ($r->{'baseform'}) {
            $r->{'flags'} |= (1 << 3);
         }
         write_uint32_t($fr, $r->{'flags'}, 'reading flags');
         if ($r->{'baseform'}) {
            write_utf8_string($fr, $r->{'baseform'}, 'reading baseform');
         }
         my $tlen = @{$r->{'tags'}};
         write_uint32_t($fr, $tlen, 'num tags');
         foreach my $t (@{$r->{'tags'}}) {
            write_utf8_string($fr, $t, 'tag');
         }

         write_uint32_t($fc, length($ro), 'reading packet length');
         print $fc $ro;
      }

      if ($c->{'text'}) {
         write_utf8_string($fc, $c->{'text'}, 'cohort text');
      }

      write_uint32_t($fw, length($co), 'cohort packet length');
      print $fw $co;
   }

   write_uint32_t($fh, length($wo), 'window packet length');
   print $fh $wo;

   if ($ENV{'DEBUG'} && $ENV{'DEBUG'} > 1) { write_uint32_t($fh, 0, 'debug null'); }
}

sub write_null_response {
   my ($fh) = @_;
   write_uint32_t($fh, 0, 'null response');
}

1;
