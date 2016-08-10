#!/usr/local/bin/php
<?php

function profile_revision($rev) {
	$dir = 'rev-'.$rev;
	chdir('/tmp');
	shell_exec('rm -rf '.$dir.' 2>&1 >/dev/null');

	echo "Exporting revision $rev...\n";
	shell_exec('svn export -r '.$rev.' --ignore-externals svn+ssh://beta.visl.sdu.dk/usr/local/svn/repos/visl/tools/vislcg3/trunk '.$dir.' >/dev/null 2>&1');
	chdir($dir);
	shell_exec('svn export -r 10017 --ignore-externals svn+ssh://beta.visl.sdu.dk/usr/local/svn/repos/visl/trunk/parsers/dansk/etc/dancg.cg dancg >/dev/null 2>&1');
	echo "Compiling...\n";

	if (file_exists('./src/all_vislcg3.cpp')) {
		echo "Using all_vislcg3.cpp and Boost...\n";
		echo shell_exec('g++ -std=c++17 -DHAVE_BOOST -DNDEBUG -pthread -pipe -Wall -Wextra -Wno-deprecated -fPIC -flto -O3 -Iinclude -Iinclude/exec-stream -Iinclude/posix ./src/all_vislcg3.cpp -o vislcg3 -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc 2>&1');
		echo shell_exec('g++ -std=c++17 -DHAVE_BOOST -DNDEBUG -pthread -pipe -Wall -Wextra -Wno-deprecated -fPIC -flto -O3 -Iinclude -Iinclude/exec-stream -Iinclude/posix ./src/all_vislcg3.cpp -o vislcg3-tc -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc -ltcmalloc 2>&1');
	}
	else {
		echo "Using old-style without Boost...\n";
		echo shell_exec('g++ -std=c++17 -DHAVE_BOOST -pthread -pipe -Wall -Wextra -Wno-deprecated -fPIC -flto -O3 -Iinclude -Iinclude/exec-stream $(ls -1 ./src/*.cpp | egrep -v "/test_" | egrep -v "/cg_" | egrep -v "/all_" | grep -v Apertium | grep -v Matxin | grep -v FormatConverter) -o vislcg3 -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc -ltcmalloc 2>&1');
		echo shell_exec('g++ -std=c++17 -DHAVE_BOOST -pthread -pipe -Wall -Wextra -Wno-deprecated -fPIC -flto -O3 -ltcmalloc -Iinclude -Iinclude/exec-stream $(ls -1 ./src/*.cpp | egrep -v "/test_" | egrep -v "/cg_" | egrep -v "/all_" | grep -v Apertium | grep -v Matxin | grep -v FormatConverter) -o vislcg3-tc -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc -ltcmalloc 2>&1');
	}

	if (!file_exists('vislcg3') || !file_exists('vislcg3-tc')) {
		echo "Revision $rev failed compilation!\n";
		return;
	}

	$times = array(
		'rev' => $rev
		);
	for ($i=0 ; $i<3 ; $i++) {
		echo "Parsing...\n";
		$start = microtime(true);
		$time = shell_exec('/usr/bin/time ./vislcg3-tc -g dancg --grammar-only --grammar-bin dancg.cg3b 2>&1 | grep user | grep system');
		$times['parse'][$i]['microtime'] = microtime(true) - $start;
		$times['parse'][$i]['time'] = trim($time);

		if (!file_exists('dancg.cg3b')) {
			echo "Revision $rev failed grammar parsing!\n";
			return;
		}

		echo "Applying...\n";
		$start = microtime(true);
		$time = shell_exec('head -n 2000 /home/tino/vislcg3/trunk/comparison/arboretum_stripped.txt | /usr/bin/time ./vislcg3-tc -g dancg.cg3b 2>&1 | grep user | grep system');
		$times['apply'][$i]['microtime'] = microtime(true) - $start;
		$times['apply'][$i]['time'] = trim($time);
	}

	echo "Parsing via valgrind...\n";
	$ticks = shell_exec('valgrind ./vislcg3 -g dancg --grammar-only --grammar-bin dancg.cg3b 2>&1 | grep "total heap usage"');
	$times['parse']['memory'] = trim($ticks);

	echo "Applying via valgrind...\n";
	$ticks = shell_exec('head -n 2000 /home/tino/vislcg3/trunk/comparison/arboretum_stripped.txt | valgrind ./vislcg3 -g dancg.cg3b 2>&1 | grep "total heap usage"');
	$times['apply']['memory'] = trim($ticks);

	echo "Parsing via callgrind...\n";
	$ticks = shell_exec('valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3 -g dancg --grammar-only --grammar-bin dancg.cg3b 2>&1 | grep Collected');
	$times['parse']['ticks'] = trim($ticks);

	echo "Applying via callgrind...\n";
	$ticks = shell_exec('head -n 2000 /home/tino/vislcg3/trunk/comparison/arboretum_stripped.txt | valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3 -g dancg.cg3b 2>&1 | grep Collected');
	$times['apply']['ticks'] = trim($ticks);

	file_put_contents('/tmp/cg3-times-'.$rev.'.txt', var_export($times, true));
	file_put_contents('/tmp/cg3-times-'.$rev.'.sphp', serialize($times));

	chdir('/tmp');
	shell_exec('rm -rf '.$dir.' 2>&1 >/dev/null');
}

$revs = array(10824, 10809, 10800, 10373, 10044);
$revs = array(11682);
foreach ($revs as $rev) {
	profile_revision($rev);
}
