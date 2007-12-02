#!/usr/bin/perl

use strict;
use warnings;

use Test;
use Socket;

BEGIN { plan tests => 14 }

require('test/PDBTest.pm');

## boot with log_level == NONE should not open a log file
eval {
    unlink('test/none.log');
    my $pid = PDBTest::startup_with_inline_configuration(<<'ENDCFG');
log_level = NONE
log_file = none.log
ENDCFG

    PDBTest::shutdown();
};
ok($@, '', "test failed: $@");
ok(not -e "test/none.log");
unlink('test/none.log');

## boot with empty configuration should use pdb.log
eval {
    unlink('pdb.log');
    my $pid = PDBTest::startup_with_inline_configuration('');
    PDBTest::shutdown();
};
ok($@, '', "test failed: $@");
ok(-e 'pdb.log');
unlink('pdb.log');

## boot with different log levels
eval {
    unlink('test/debug.log');
    my $pid = PDBTest::startup_with_inline_configuration(<<'ENDCFG');
log_level = DEBUG
log_file = test/debug.log
ENDCFG

    PDBTest::shutdown();
};
ok($@, '', "test failed: $@");
ok(-e 'test/debug.log');
ok(`cat test/debug.log`, qr/^0 /, "no debug logs found");
unlink('test/debug.log');

eval {
    unlink('test/info.log');
    my $pid = PDBTest::startup_with_inline_configuration(<<'ENDCFG');
log_level = INFO
log_file = test/info.log
ENDCFG

    PDBTest::shutdown();
};
ok($@, '', "test failed: $@");
ok(-e 'test/info.log');
ok(`cat test/info.log`, qr/^1 /, "no info logs found");
ok(`grep ^0 test/info.log`, '', "debug logs found");
unlink('test/info.log');

eval {
    unlink('test/error.log');
    my $pid = PDBTest::startup_with_inline_configuration(<<'ENDCFG');
log_level = ERROR
log_file = test/error.log
ENDCFG

    PDBTest::shutdown();
};
ok($@, '', "test failed: $@");
ok(-e 'test/error.log');
ok(-z 'test/error.log');
unlink('test/error.log');
