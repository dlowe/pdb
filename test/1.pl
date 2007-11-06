#!/usr/bin/perl

use strict;
use warnings;

use Test;

BEGIN { plan tests => 1 }

my $output = `../pdb 2>&1`;
ok($output, "", "pdb failed to boot: $output");

my $ps = `ps -ax | grep pdb | grep -v grep`;
