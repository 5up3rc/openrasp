--TEST--
Check for callabe hook
--SKIPIF--
<?php 
if (!extension_loaded("openrasp")) print "skip";
if (!stristr(PHP_OS, "Linux")) die("skip this test is Linux platforms only");
?>
--FILE--
<?php
$a = array('ls', 'ls');
array_map("system", $a);
echo 'array_map OK';
?>
--EXPECT--