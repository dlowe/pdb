#!/usr/bin/perl

use strict;
use warnings;

use Test;
use Socket;

BEGIN { plan tests => 2 }

require('test/PDBTest.pm');

## boot with no arguments
my $pid;
eval {
    $pid = PDBTest::startup_with_args("");

    PDBTest::shutdown();
};
ok($@, qr/usage/, "pdb booted with no arguments") or die;

eval {
    $pid = PDBTest::startup_with_args("-c test/nonexistent.cfg");

    PDBTest::shutdown();
};
ok($@, qr/error/, "pdb booted against nonexistent configuration file") or die;
