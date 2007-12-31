#!/usr/bin/perl

package MySQLTest;

use strict;
use warnings;

our $basedir = '/usr/local/mysql';
our $socket_file = '/socket';
our $pid_file = '/pid';

our @servers = (
    { 'name' => 'master',
      'dir'  => '/tmp/test_master',
      'port' => 1234,
      'init' => 'test/mysql_master.sql' },
    { 'name' => 'partition_1',
      'dir'  => '/tmp/test_partition_1',
      'port' => 1235,
      'init' => 'test/mysql_partition_1.sql' },
    { 'name' => 'partition_2',
      'dir'  => '/tmp/test_partition_2',
      'port' => 1236,
      'init' => 'test/mysql_partition_2.sql' },
);

our $database_configuration = qq#
db_type = mysql
#;

foreach my $server (@servers) {
    $database_configuration .= qq#
delegate $server->{'name'}
{
    hostname = 127.0.0.1
    port = $server->{'port'}
    name = $server->{'name'}
}
#;
}

1;
