#!/usr/bin/perl

# For now, this just tests the addition and removal of 1000 VLAN interfaces on eth0

my $num_if = 4000;

`/usr/local/bin/vconfig set_name_type VLAN_PLUS_VID_NO_PAD`;

my $d = 5;
my $c = 5;

my $i;
print "Adding VLAN interfaces 1 through $num_if\n";
my $p = time();
for ($i = 1; $i<=$num_if; $i++) {
  `/usr/local/bin/vconfig add eth0 $i`;
  `ifconfig vlan$i 192.168.$c.$d`;
  `ifconfig -i vlan$i up`;

  $d++;
  if ($d > 250) {
    $d = 5;
    $c++;
  }
}
my $n = time();
my $diff = $n - $p;

print "Done adding $num_if VLAN interfaces in $diff seconds.\n";

sleep 2;

print "Doing ifconfig -a\n";
`time ifconfig -a > /tmp/vlan_test_ifconfig_a.txt`;

sleep 2;

print "Removing VLAN interfaces 1 through $num_if\n";
$d = 5;
$c = 5;
$p = time();
for ($i = 1; $i<=$num_if; $i++) {
  `/usr/local/bin/vconfig rem vlan$i`;

  $d++;
  if ($d > 250) {
    $d = 5;
    $c++;
  }
}
$n = time();
$diff = $n - $p;
print "Done deleting $num_if VLAN interfaces in $diff seconds.\n";

sleep 2;

my $tmp = $num_if / 4;
print "\nGoing to add and remove 2 interfaces $tmp times.\n";
$p = time();


for ($i = 1; $i<=$tmp; $i++) {
  `/usr/local/bin/vconfig add eth0 1`;
  `ifconfig vlan1 192.168.200.200`;
  `ifconfig -i vlan1 up`;

  `/usr/local/bin/vconfig add eth0 2`;
  `ifconfig vlan2 192.168.202.202`;
  `ifconfig -i vlan2 up`;

  `/usr/local/bin/vconfig rem vlan2`;
  `/usr/local/bin/vconfig rem vlan1`;
}
$n = time();
$diff = $n - $p;
print "Done adding/removing 2 VLAN interfaces $num_if times in $diff seconds.\n";
