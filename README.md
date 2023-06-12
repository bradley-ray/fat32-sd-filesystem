# FAT32 Filesystem for SD Card

* implements basic read/write functionality for a FAT32 filesystem for an SD Card.
* includes a basic interactive shell over the UART serial port to interact with the file system
	- `ls` : list files in the current directory
	- `cd` : change into the specified directory
	- `cat` : print out the contents of the specified file
	- `pwd` : print the current folder name
	- `ed` : a basic text editor

## TODO
* write still hardcoded, need to find next free space, and write to it
* implement mkdir function
* editor should be able to open a file, not just create new ones
* few fat32 functions need to be broken into multiple
* experiment with methods to keep track of whole path
