#!/usr/bin/perl

use strict;
use warnings;

use Test::More qw(no_plan);
use Socket;

require('test/PDBTest.pm');

## boot with no arguments
my $pid;
eval {
    $pid = PDBTest::startup_with_args("");

    PDBTest::shutdown();
};
like($@, qr/usage/, "pdb booted with no arguments") or die;

eval {
    $pid = PDBTest::startup_with_args("-c test/nonexistent.cfg");

    PDBTest::shutdown();
};
like($@, qr/error/, "pdb booted against nonexistent configuration file") or die;
