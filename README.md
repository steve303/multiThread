# multiThread


This program takes in as input: 1) sequence of multiple input files containing urls, 2) number of threads to utilize (user defined).  The inputs are issued from the command line.  An example of the input can be: `./multi-lookup names1.txt names2.txt 2`.  There is one restriction which is that the number of threads must be a multiple of the number of input files.  Once a thread has finished it will be written to a file called parserout.txt  



