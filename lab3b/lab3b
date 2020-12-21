#!/usr/local/cs/bin/python3

# NAME: Haley Kim,Shri Narayana
# EMAIL: haleykim@g.ucla.edu,shrinikethn@ucla.edu                                       
# ID: 405111152,505313060 

import csv
import sys

# Global Variables
is_consistent = True # global variable for exit code determination
superblock = None #the object of type SuperBlock
group = None #the object of type Group

# Data Structures
list_free_blocks = [] # filled with type: int
list_free_inodes = [] # filled with type: int
list_inodes = [] # filled with type: Inode
list_dirent = [] # filled with type: Dirent
list_indirent = [] # filled with type: Indirent

class SuperBlock:
	def __init__(self, line):
		self.total_num_blocks = int(line[1])
		self.total_num_inodes = int(line[2])
		self.block_size = int(line[3])
		self.inode_size = int(line[4])
		self.blocks_per_group = int(line[5])
		self.inodes_per_group = int(line[6])
		self.first_non_reserved_inode = int(line[7])

class Group:
	def __init__(self, line):
		self.group_num = int(line[1])
		self.total_num_blocks_in_group = int(line[2])
		self.total_num_inodes_in_group = int(line[3])
		self.num_free_blocks = int(line[4])
		self.num_free_inodes = int(line[5])
		self.block_bitmap_block_num = int(line[6])
		self.inode_bitmap_block_num = int(line[7])
		self.first_inode_block_num = int(line[8])

class Inode:
	def read_blocks(self, line):
		list_blocks = []
		if line[2] != "s":
			for i in range(12, 24):
				list_blocks.append(int(line[i]))
		return list_blocks
	def read_ptrs(self, line):
		list_ptrs = []
		if line[2] != "s":
			for i in range(24, 27):
				list_ptrs.append(int(line[i]))
		return list_ptrs
	def __init__(self, line):
		self.inode_num = int(line[1])
		self.file_type = line[2]
		self.mode = line[3]
		self.owner = int(line[4])
		self.group = int(line[5])
		self.link_count = int(line[6])
		self.change_time = line[7]
		self.mod_time = line[8]
		self.access_time = line[9]
		self.file_size = int(line[10])
		self.num_512_blocks = int(line[11]) # first 11 fields after "INODE"
		self.list_blocks = self.read_blocks(line) # 12 blocks
		self.list_ptrs = self.read_ptrs(line) # 3 indirect blocks

class Dirent:
	def __init__(self, line):
		self.parent_inode_num = int(line[1])
		self.offset_within_dir = int(line[2])
		self.referenced_file_inode_num = int(line[3])
		self.entry_length = int(line[4])
		self.name_length = int(line[5])
		self.name = line[6]

class Indirent:
	def __init__(self, line):
		self.owning_file_inode_num = int(line[1])
		self.indirection_level = int(line[2])
		self.offset_ref_block = int(line[3])
		self.indirect_block_num = int(line[4])
		self.ref_block_num = int(line[5])

class BlockData:
	def __init__(self, block_type, block_num, inode_num, offset):
		if block_type == "":
			self.block_type = block_type + "BLOCK"
		else:
			self.block_type = block_type + " BLOCK"
		self.block_num = block_num
		self.inode_num = inode_num
		self.offset = offset

# Helper Functions
def print_err_msg(msg): # change consistency state, print msg
	is_consistent = False
	print(msg, file=sys.stdout)

def parse_csv(filename): # parse .csv line by line, fill in structs
	global superblock
	global group
	try:
		with open(filename) as csv_file:
			csv_reader = csv.reader(csv_file)
			for line in csv_reader:
				if line[0] == "SUPERBLOCK":
					superblock = SuperBlock(line)
				if line[0] == "GROUP":
					group = Group(line)
				if line[0] == "BFREE":
					list_free_blocks.append(int(line[1]))
				if line[0] == "IFREE":
					list_free_inodes.append(int(line[1]))
				if line[0] == "INODE":
					list_inodes.append(Inode(line))
				if line[0] == "DIRENT":
					list_dirent.append(Dirent(line))
				if line[0] == "INDIRECT":
					list_indirent.append(Indirent(line))
	except IOError:
		print("Error: unable to read .csv file", file=sys.stderr)
		sys.exit(1)

def blocks_check(): # INVALID, RESERVED, ALLOCATED, UNREFERENCED, DUPLICATE
	indirection_level = {1: 'INDIRECT', 2: 'DOUBLE INDIRECT', 3: 'TRIPLE INDIRECT'}  # map of levels to text for output
	logical_offsets = {1: 12, 2: (12 + 256),
					   3: (12 + 256 + 256 ** 2)}  # dict of associated logical offsets for each block
	first_legal_block = group.first_inode_block_num + \
						superblock.inode_size * group.total_num_inodes_in_group / superblock.block_size
	block_ref_dict = {} # keeping track for ALLOCATED, UNREFERENCED, DUPLICATE
	for inode in list_inodes: # scan through all allocated inodes
		logical_offset = 0
		for block in inode.list_blocks: # scan through each direct block in each inode
			if block != 0:
				if (block < 0) or (block > (superblock.total_num_blocks-1)):
					print_err_msg("INVALID BLOCK {} IN INODE {} AT OFFSET {}"
								  .format(block, inode.inode_num, logical_offset))
				if block < first_legal_block:
					print_err_msg("RESERVED BLOCK {} IN INODE {} AT OFFSET {}"
								  .format(block, inode.inode_num, logical_offset))
				elif block in block_ref_dict and block != 0: # keeping track in dict for later
					block_ref_dict[block].append(BlockData("", block, inode.inode_num, logical_offset))
				else: # keeping track in dict for later
					block_ref_dict[block] = [BlockData("", block, inode.inode_num, logical_offset)]
			logical_offset += 1
		for i in range(len(inode.list_ptrs)): # scan through each indirect block in each inode
			level = indirection_level[i + 1]
			logical_offset = logical_offsets[i + 1]  # one-indexed
			if inode.list_ptrs[i] != 0:
				if (inode.list_ptrs[i] < 0) or (inode.list_ptrs[i] > (superblock.total_num_blocks-1)):
					print_err_msg("INVALID {} BLOCK {} IN INODE {} AT OFFSET {}"
								  .format(level, inode.list_ptrs[i], inode.inode_num, logical_offset))
				if inode.list_ptrs[i] < first_legal_block:
					print_err_msg("RESERVED {} BLOCK {} IN INODE {} AT OFFSET {}"
								  .format(level, inode.list_ptrs[i], inode.inode_num, logical_offset))
				elif inode.list_ptrs[i] in block_ref_dict:
					block_ref_dict[inode.list_ptrs[i]].append(BlockData(level,
																		inode.list_ptrs[i],
																		inode.inode_num,
																		logical_offset))
				else:
					block_ref_dict[inode.list_ptrs[i]] = [BlockData(level,
																	inode.list_ptrs[i],
																	inode.inode_num,
																	logical_offset)]
	for i in range(len(list_indirent)): # scan through the listed INDIRECT entries
		level = indirection_level[list_indirent[i].indirection_level]
		logical_offset = list_indirent[i].offset_ref_block
		if list_indirent[i].ref_block_num != 0:
			if (list_indirent[i].ref_block_num < 0) or (list_indirent[i].ref_block_num > (superblock.total_num_blocks-1)):
				print_err_msg("INVALID {} BLOCK {} IN INODE {} AT OFFSET {}".format(level,
																					list_indirent[i].ref_block_num,
																					list_indirent[i].owning_file_inode_num,
																					logical_offset))
			if list_indirent[i].ref_block_num < first_legal_block:
				print_err_msg("RESERVED {} BLOCK {} IN INODE {} AT OFFSET {}".format(level,
																					 list_indirent[i].ref_block_num,
																					 list_indirent[i].owning_file_inode_num,
																					 logical_offset))
			elif list_indirent[i].ref_block_num in block_ref_dict:
				block_ref_dict[list_indirent[i].ref_block_num].append(BlockData(level,
																				list_indirent[i].ref_block_num,
																				list_indirent[i].owning_file_inode_num,
																				logical_offset))
			else:
				block_ref_dict[list_indirent[i].ref_block_num] = [BlockData(level,
																			list_indirent[i].ref_block_num,
																			list_indirent[i].owning_file_inode_num,
																			logical_offset)]
	start = first_legal_block # temp var in order to shorten code
	end = superblock.total_num_blocks-1 # temp var in order to shorten code
	for block in range(0, end):
		if start <= block <= end:
			if block not in block_ref_dict and block not in list_free_blocks:
				print_err_msg("UNREFERENCED BLOCK {}".format(block))
	for block in range(0, end):
		if start <= block <= end:
			if block in block_ref_dict and block in list_free_blocks:
				print_err_msg("ALLOCATED BLOCK {} ON FREELIST".format(block))
	for block in range(0, end):
		if start <= block <= end:
			if block in block_ref_dict:
				block_data = block_ref_dict[block] # there could be multiple values for this key
				if len(block_data) > 1:
					for i in range(len(block_data)): # printing out each time BlockData was inserted
						data = block_data[i]
						print_err_msg("DUPLICATE {} {} IN INODE {} AT OFFSET {}"
									  .format(data.block_type, data.block_num, data.inode_num, data.offset))

def inodes_check():
	inode_accounted = [False] * superblock.total_num_inodes
	for inode in list_inodes:
		inode_accounted[inode.inode_num-1] = True
		if inode.mode != 0:
			for inode_num in list_free_inodes:
				inode_accounted[inode_num-1] = True
				if inode_num == inode.inode_num:
					print("ALLOCATED INODE " + str(inode_num) + " ON FREELIST")
					break
		else:
			print("This should only print with incorrect input.")
	for i in range(0,superblock.total_num_inodes):
		if not inode_accounted[i] and i >= 10:
			print("UNALLOCATED INODE " + str(i+1) + " NOT ON FREELIST")


def directory_check():
	inodes_child_count = [0] * (superblock.total_num_inodes+1)
	dir_parents = [0] * (superblock.total_num_inodes+1)
	dir_parents[2] = 2
	for dirent in list_dirent:
		if dirent.name != "'.'" and dirent.name != "'..'" and 1 <= dirent.referenced_file_inode_num <= superblock.total_num_inodes:
			dir_parents[dirent.referenced_file_inode_num] = dirent.parent_inode_num
	for dirent in list_dirent:
		# print(dirent.name)
		if dirent.referenced_file_inode_num < 1 or dirent.referenced_file_inode_num > superblock.total_num_inodes:
			print("DIRECTORY INODE " + str(dirent.parent_inode_num) + " NAME " + str(dirent.name) + " INVALID INODE " + str(dirent.referenced_file_inode_num))
			continue
		else:
			isAllocated = False
			for inode in list_inodes:
				if inode.inode_num == dirent.referenced_file_inode_num:
					isAllocated = True
					break
			if not isAllocated:
				print("DIRECTORY INODE " + str(dirent.parent_inode_num) + " NAME " + str(dirent.name) + " UNALLOCATED INODE " + str(dirent.referenced_file_inode_num))
				continue
		if dirent.name == "'.'":
			# print("in . " + str(dirent.referenced_file_inode_num) + "  " + str(dirent.parent_inode_num))
			if dirent.referenced_file_inode_num != dirent.parent_inode_num:
				print("DIRECTORY INODE " + str(dirent.parent_inode_num) + " NAME " + dirent.name + "LINK TO INODE " + str(dirent.referenced_file_inode_num) + " SHOULD BE " + str(dirent.parent_inode_num))
		elif dirent.name == "'..'":
			parent = dir_parents[dirent.parent_inode_num]
			# print(parent)
			if dirent.referenced_file_inode_num != parent:
				print("DIRECTORY INODE " + str(dirent.parent_inode_num) + " NAME " + dirent.name + " LINK TO INODE " + str(dirent.referenced_file_inode_num) + " SHOULD BE " + str(dirent.parent_inode_num))
		inodes_child_count[dirent.referenced_file_inode_num] += 1
	for inode in list_inodes:
		if inodes_child_count[inode.inode_num] != inode.link_count:
			print("INODE " + str(inode.inode_num) + " HAS " + str(inodes_child_count[inode.inode_num]) + " LINKS BUT LINKCOUNT IS " + str(inode.link_count))

# Main Routine
def main():
	if len(sys.argv) != 2: # check num of args, there must be one filename arg
		print("Error: wrong number of arguments", file=sys.stderr)
		sys.exit(1)
	parse_csv(sys.argv[1]) # parse and fill in data structures
	blocks_check()
	inodes_check()
	directory_check()
	if is_consistent:
		sys.exit(0) # print_err_msg was never called and file has no inconsistencies
	else:
		sys.exit(2) # print_err_msg was called at least once

if __name__ == "__main__":
	main()
