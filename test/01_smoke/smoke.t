#!/usr/bin/perl

use strict;
use warnings;

use Test::More qw(no_plan);
use Socket;

require('test/PDBTest.pm');

ok(PDBTest::exists(), "can't find pdb") or die;

my $pid;
eval {
    $pid = PDBTest::startup();
};
ok($@ eq '', "pdb failed to boot: $@") or die;

like($pid, qr/^\d+$/, "can't figure out pdb process id");

eval {
    socket(SH, PF_INET, SOCK_STREAM, getprotobyname('tcp')) or die;
    connect(SH, sockaddr_in(5032, inet_aton("localhost"))) or die;
    close(SH) or die;
};
ok($@ eq '', "pdb not answering: $@");

eval {
    PDBTest::shutdown();
};
ok($@ eq '', "shutdown failed: $@");

ok(`grep ^2 test/pdb.log | grep -v 'connecting to a delegate'` eq '', "error logs found");
