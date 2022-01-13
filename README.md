## Multi-threaded Program in C  

### Description  
This program reads multiple files concurrently parsing the contents and writing to a shared buffer file.  Multiple threads will complete this task but only a single thread will handle one individual file.  Once the thread is finished with the file it will write its status to a file called parserOut.txt.  The contents of the shared buffer file contain URLs and we want these converted to IPv4 addresses and written to a file called converterOut.txt.  Multiple threads will perform this task until buffer is empty.  

This program takes in as input: 1) sequence of input files containing urls, 2) number of threads (user defined).  An example of the command to be input in the terminal can be: `./multi-lookup names1.txt names2.txt 2`.  There is one restriction which is that the number of threads must be a multiple of the number of input files.  

Details of the problem statment can be found [here](https://steve303.github.io/multiThread/problem_stmt.pdf)



