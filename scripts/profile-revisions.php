#!/usr/local/bin/php
<?php

function profile_revision($rev) {
	$dir = 'rev-'.$rev;
	chdir('/tmp');
	shell_exec('rm -rf '.$dir.' 2>&1 >/dev/null');

	echo "Exporting revision $rev...\n";
	shell_exec('svn export -r '.$rev.' --ignore-externals http://beta.visl.sdu.dk/svn/visl/tools/vislcg3/trunk '.$dir.' >/dev/null 2>&1');
	chdir($dir);
	shell_exec('svn export -r 3617 --ignore-externals http://beta.visl.sdu.dk/svn/visl/trunk/parsers/dansk/etc/dancg dancg >/dev/null 2>&1');
	echo "Compiling...\n";
	/*
	shell_exec('g++ -DHAVE_BOOST -I/home/tino/tmp/boost_1_46_1 -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -ltcmalloc -licuio -licuuc $(ls -1 ./src/*.cpp | egrep -v "/test_" | egrep -v "/cg_" | egrep -v "/all_" | grep -v Apertium | grep -v Matxin | grep -v FormatConverter) -o vislcg3 >/dev/null 2>&1');
	/*/
	shell_exec('g++ -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc $(ls -1 ./src/*.cpp | egrep -v "/test_" | egrep -v "/cg_" | egrep -v "/all_" | grep -v Apertium | grep -v Matxin | grep -v FormatConverter) -o vislcg3 >/dev/null 2>&1');
	//*/
	
	if (!file_exists('vislcg3')) {
		echo "Revision $rev failed compilation!\n";
		return;
	}
	
	$times = array(
		'rev' => $rev
		);
	for ($i=0 ; $i<3 ; $i++) {
		echo "Parsing...\n";
		$start = microtime(true);
		$time = shell_exec('/usr/bin/time ./vislcg3 -C ISO-8859-1 -g dancg --grammar-only --grammar-bin dancg.cg3b 2>&1 | grep user | grep system');
		$times['parse'][$i]['microtime'] = microtime(true) - $start;
		$times['parse'][$i]['time'] = trim($time);
		
		if (!file_exists('dancg.cg3b')) {
			echo "Revision $rev failed grammar parsing!\n";
			return;
		}

		echo "Applying...\n";
		$start = microtime(true);
		$time = shell_exec('head -n 2000 /home/tino/vislcg3/trunk/comparison/arboretum_stripped.txt | u2i | /usr/bin/time ./vislcg3 -C ISO-8859-1 -g dancg.cg3b 2>&1 | grep user | grep system');
		$times['apply'][$i]['microtime'] = microtime(true) - $start;
		$times['apply'][$i]['time'] = trim($time);
	}
	
	echo "Parsing via valgrind...\n";
	$ticks = shell_exec('valgrind ./vislcg3 -C ISO-8859-1 -g dancg --grammar-only --grammar-bin dancg.cg3b 2>&1 | grep "total heap usage"');
	$times['parse']['memory'] = trim($ticks);

	echo "Applying via valgrind...\n";
	$ticks = shell_exec('head -n 2000 /home/tino/vislcg3/trunk/comparison/arboretum_stripped.txt | u2i | valgrind ./vislcg3 -C ISO-8859-1 -g dancg.cg3b 2>&1 | grep "total heap usage"');
	$times['apply']['memory'] = trim($ticks);

	echo "Parsing via callgrind...\n";
	$ticks = shell_exec('valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3 -C ISO-8859-1 -g dancg --grammar-only --grammar-bin dancg.cg3b 2>&1 | grep Collected');
	$times['parse']['ticks'] = trim($ticks);

	echo "Applying via callgrind...\n";
	$ticks = shell_exec('head -n 2000 /home/tino/vislcg3/trunk/comparison/arboretum_stripped.txt | u2i | valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3 -C ISO-8859-1 -g dancg.cg3b 2>&1 | grep Collected');
	$times['apply']['ticks'] = trim($ticks);

	file_put_contents('/tmp/cg3-times-'.$rev.'.txt', var_export($times, true));
	file_put_contents('/tmp/cg3-times-'.$rev.'.sphp', serialize($times));

	chdir('/tmp');
	shell_exec('rm -rf '.$dir.' 2>&1 >/dev/null');
}

$revs = array(7000, 6987, 6898, 6885, 6781, 6692, 6500, 6328, 6268, 6242, 6170, 5932, 5930, 5926, 5918, 5839, 5810, 5773, 5729, 5431, 5129, 5042, 4879, 4779, 4545, 4513, 4493, 4474, 4410, 4292, 4031, 3991, 3896, 3852, 3800, 3689, 3682, 3617);
$revs = array(7000, 6500);
foreach ($revs as $rev) {
	profile_revision($rev);
}
