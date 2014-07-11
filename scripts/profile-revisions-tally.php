#!/usr/local/bin/php
<?php

$times = array();

$files = glob('/tmp/cg3-times-*.sphp');
foreach ($files as $file) {
	$time = unserialize(file_get_contents($file));
	$rev = intval($time['rev']);

	$times['parse_secs'][$rev] = 99999999.9;
	foreach ($time['parse'] as $k => $v) {
		if (!is_array($v) || empty($v['time'])) {
			continue;
		}
		preg_match('@([0-9.]+)user@', $v['time'], $m);
		$times['parse_secs'][$rev] = min($times['parse_secs'][$rev], floatval($m[1]));
	}
	preg_match('@Collected : ([0-9]+)@', $time['parse']['ticks'], $m);
	$times['parse_ticks'][$rev] = intval($m[1]);

	preg_match('@([0-9,]+) allocs.+ ([0-9,]+) bytes allocated@', $time['parse']['memory'], $m);
	$times['parse_allocs'][$rev] = intval(str_replace(',', '', $m[1]));
	$times['parse_bytes'][$rev] = intval(str_replace(',', '', $m[2]));

	$times['apply_secs'][$rev] = 99999999.9;
	foreach ($time['apply'] as $k => $v) {
		if (!is_array($v) || empty($v['time'])) {
			continue;
		}
		preg_match('@([0-9.]+)user@', $v['time'], $m);
		$times['apply_secs'][$rev] = min($times['apply_secs'][$rev], floatval($m[1]));
	}
	preg_match('@Collected : ([0-9]+)@', $time['apply']['ticks'], $m);
	$times['apply_ticks'][$rev] = intval($m[1]);

	preg_match('@([0-9,]+) allocs.+ ([0-9,]+) bytes allocated@', $time['apply']['memory'], $m);
	$times['apply_allocs'][$rev] = intval(str_replace(',', '', $m[1]));
	$times['apply_bytes'][$rev] = intval(str_replace(',', '', $m[2]));
}

foreach ($times as $s => $t) {
	echo "$s\n";
	ksort($t);
	foreach ($t as $r => $v) {
		echo "[$r, $v],\n";
	}
	echo "\n";
}
