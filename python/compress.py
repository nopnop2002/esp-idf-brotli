import brotli
import os
import sys

def usage(name):
	print("usage python3 {} path_to_compress path_to_output".format(name))

def main():
	argv = sys.argv
	args = len(argv)
	if args != 3:
		usage(argv[0])
		exit()

	if os.path.isfile(argv[1]) is False:
		print("{} not exist".format(argv[2]))
		exit()

	if os.path.isfile(argv[2]) is True:
		print("{} already exist".format(argv[2]))
		exit()

	file_path = argv[1]
	br_file_path = argv[2]
 
	with open(file_path, 'rb') as f:
		data = f.read()
 
	compressed_data = brotli.compress(data)
 
	with open(br_file_path, mode='wb') as f:
		f.write(compressed_data)

if __name__ == "__main__":
	main()

