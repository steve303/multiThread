## Multi-threaded Program in C  

### Description  
This program reads several files containing URLs.  Each URL in the file is parsed and then converted into its IP address.  


This program takes in as input: 1) sequence of input files containing urls, 2) number of threads (user defined).  An example of the command to be input in the terminal can be: `./multi-lookup names1.txt names2.txt 2`.  There is one restriction which is that the number of threads must be a multiple of the number of input files.  Once a thread has finished it's status will be written to a file called parserout.txt  

Details of the problem statment can be found [here](https://steve303.github.io/multiThread/problem_stmt.pdf)



