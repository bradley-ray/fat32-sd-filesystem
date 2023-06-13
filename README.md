# FAT32 Filesystem for SD Card

* implements basic read/write functionality for a FAT32 filesystem for an SD Card.
* includes a basic interactive shell over the UART serial port to interact with the file system
	- `ls` : list files in the current directory
	- `cd` : change into the specified directory
	- `cat` : print out the contents of the specified file
	- `pwd` : print the current folder name
	- `ed` : a basic text editor
	- `rm` : remove a file
	- `rmdir` : recursively delete all files in a directory and the directory itself
	- `mkdir` : create a directory
	- `touch` : create empty file

## TODO
* editor should be able to open and read/edit file, not just create new ones
* few fat32 functions need to be broken into multiple functions
* support longer names & larger files (> 512 Bytes) 
* experiment with methods to keep track of whole path
* most fat read/write stuff only deals in single sector/cluster, should be able
	to do more than that
* switch to DMA/Interrupt handling instead of just polling
