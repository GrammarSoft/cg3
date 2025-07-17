from constraint_grammar import *

from collections import defaultdict
from dataclasses import dataclass, field
import struct
from typing import DefaultDict, Dict, List, Optional

@dataclass
class Reading:
	lemma: str = ''
	tags: List[str] = field(default_factory=list)
	subreading: Optional['Reading'] = None

@dataclass
class Cohort:
	static: Reading = field(default_factory=Reading)
	readings: List[Reading] = field(default_factory=list)
	dep_self: int = 0
	dep_parent: Optional[int] = None
	relations: DefaultDict[str, List[int]] = field(
		default_factory=lambda: defaultdict(list))
	text: str = ''
	wblank: str = ''

@dataclass
class Window:
	cohorts: List[Cohort] = field(default_factory=list)
	set_vars: Dict[str, Optional[str]] = field(default_factory=dict)
	rem_vars: List[str] = field(default_factory=list)
	text: str = ''
	text_post: str = ''
	flush_after: bool = False

def parse_binary_window(buf):
	'''Given a bytestring `buf` containing a single window
	(not including the length header), parse and return a Window()
	object. For most applications you probbaly want parse_binary_stream()
	instead.'''

	pos = 0
	def read_pat(pat):
		nonlocal pos, buf
		ret = struct.unpack_from('<'+pat, buf, pos)
		pos += struct.calcsize('<'+pat)
		return ret
	def read_u16():
		return read_pat('H')[0]
	def read_u32():
		return read_pat('I')[0]
	def read_str():
		l = read_u16()
		if l == 0:
			return ''
		return read_pat(f'{l}s')[0].decode('utf-8')
	window = Window()
	window_flags = read_u16()
	if window_flags & 1:
		window.flush_after = True
	tag_count = read_u16()
	tags = [read_str() for i in range(tag_count)]
	def read_tags():
		nonlocal tags
		ct = read_u16()
		if ct == 0:
			return []
		idx = read_pat(f'{ct}H')
		return [tags[t] for t in idx]
	var_count = read_u16()
	for i in range(var_count):
		mode = read_pat('B')[0]
		t1 = read_u16()
		t2 = read_u16()
		if mode == 1:
			window.set_vars[tags[t1]] = tags[t2]
		elif mode == 2:
			window.set_vars[tags[t1]] = None
		elif mode == 3:
			window.rem_vars.append(tags[t1])
	window.text = read_str()
	window.text_post = read_str()
	cohort_count = read_u16()
	for i in range(cohort_count):
		cohort = Cohort()
		cohort_flags = read_u16()
		cohort.static.lemma = tags[read_u16()]
		cohort.static.tags = read_tags()
		cohort.dep_self = read_u32()
		cohort.dep_parent = read_u32()
		if cohort.dep_parent == 0xffffffff:
			cohort.dep_parent = None
		rel_count = read_u16()
		for i in range(rel_count):
			tag = tags[read_u16()]
			head = read_u32()
			cohort.relations[tag].append(head)
		cohort.text = read_str()
		cohort.wblank = read_str()
		reading_count = read_u16()
		prev = None
		for i in range(reading_count):
			reading_flags = read_u16()
			reading = Reading()
			reading.lemma = tags[read_u16()]
			reading.tags = read_tags()
			if reading_flags & 1 and prev is not None:
				prev.subreading = reading
			else:
				cohort.readings.append(reading)
			prev = reading
		window.cohorts.append(cohort)
	return window

def parse_binary_stream(fin):
	'''Given a file `fin`, yield a series of Window() objects.
	raises ValueError if stream header is missing or invalid.'''

	header = fin.read(8)
	label, version = struct.unpack('<4sI', header)
	if label != b'CGBF':
		raise ValueError('Binary format header not found!')
	if version != 1:
		raise ValueError('Unknown binary format version!')
	while True:
		spec = fin.read(4)
		if len(spec) != 4:
			break;
		block_len = struct.unpack('<I', spec)[0]
		block = fin.read(block_len)
		if len(block) != block_len:
			break
		yield parse_binary_window(block)
