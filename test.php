<?php
$usage1 = memory_get_usage();
$a = range(0, 10000, 3);
$usage2 = memory_get_usage();
print "$usage1 -> $usage2\n";
