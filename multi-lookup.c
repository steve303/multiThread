#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include "util.h"
#include<time.h>
#define MAX 1024    	//max buffer size
char buffer[MAX];
int bIndex = 0; 	 //this is the current buffer index !!!
int sizeS = 64;		 //used to define size of various char array	
char *parserFile = "parserOut.txt";
char *converterFile = "converterOut.txt";
int maxstrlen = 256; 	//this defines the url char array size -- inputstr[]
int inpfileCount = 0;	//counter for parser input file
int numthreads;		//number of threads -- must be entered on cmd line
char** argvx;	

					//declare the mutex
pthread_mutex_t mutxB;			  //protect buffer
pthread_mutex_t mutxCfile;		  //protect Converter output file	
pthread_mutex_t mutxPfile;		  //protect Parser output file
					
pthread_cond_t cvConvert;		  //declare the conditional variables
pthread_cond_t cvParser;


//function prototypes - see below for descriptions
void Createfile(char *filename);
int Appendfile(char inpstring[maxstrlen], char *filenom);
void Insert(char* inputstr, int maxChar);
int Sizefile(char *filenom);
void* Parser(char *filenom);
int Converter(void);
void* LoopConverter(int count_j);
void* LoopParser(int count_i);

//argv[]: file names,...,numofthreads
//reminder: argc = number of args passed in command line
//reminder: argv[0] = the executable's file name 
int main(int argc, char* argv[]){
	argvx = argv;
	time_t start = time(NULL);

	inpfileCount = argc-2; //subtract 2 for non file args, i.e. argv[0]
	numthreads = atoi(argv[argc-1]);
	if (inpfileCount % numthreads != 0 || numthreads > inpfileCount){
		printf("Number of thread pairs must be a multiple of input file count. Try again.\n");
		exit(-1);
	}

	pthread_t th_parser[numthreads];	//declare the threads
	pthread_t th_convert[numthreads];
	
	pthread_mutex_init (&mutxB, NULL);
	pthread_mutex_init (&mutxCfile, NULL);
	pthread_mutex_init (&mutxPfile, NULL);

	pthread_cond_init (&cvConvert, NULL);	//initialize cond variables;
	pthread_cond_init (&cvParser, NULL);
				
	for(int i=1; i<argc-1; i++){		//print names of input files	
		printf("file %d = %s\n", i, argv[i]);
	}
	Createfile(parserFile);//set up new file perform once
	Createfile(converterFile);//set up new file perform once
	
	for(int i=0; i < numthreads; i++){	//create threads to run the Parser() func
		pthread_create(&th_parser[i], NULL, LoopParser, i);
	}
	
	for(int j=0; j < numthreads; j++){	//create threads to run Converter() func
		pthread_create(&th_convert[j], NULL, LoopConverter, j);
	}	

	for(int i = 0; i < numthreads; i++){
		pthread_join(th_parser[i], NULL);	//join the threads: th_parser[] 
		pthread_join(th_convert[i], NULL);	//join the threads: th_convert[] 
	}	

	pthread_mutex_destroy(&mutxB);
	pthread_mutex_destroy(&mutxCfile);
	pthread_mutex_destroy(&mutxPfile);

	pthread_cond_destroy(&cvParser);	//free the cond variables
	pthread_cond_destroy(&cvConvert);
	printf("exit: time taken %.4f seconds\n", (double)(time(NULL) - start));
	return 0;

}

//function to create starter files
void Createfile(char *filename){
	FILE *fp;
	fp = fopen(filename, "w");
	fclose(fp);
	return;
}
//function to append char string to file: 
int Appendfile(char inpstring[maxstrlen], char *filenom){
	printf("In Appendfile() function\n");
	FILE *fp;
	int i = 0;
	fp = fopen(filenom, "a"); //a option means you can append to file - careful with option settings! 
	if(inpstring[0] == '\n' || inpstring[0] == '\0'){
		printf("inpstring is empty!\n");
		return (-1);
	}
	printf("inpstring = %s\n", inpstring);
	while(inpstring[i]){
		printf("inpstring[%d] = %d\n", i, inpstring[i]);
		fputc(inpstring[i], fp);
		i++;
	}
	
	fclose(fp);
	return 0;
}
//function to write to shared buffer by the Parser()	
void Insert(char* inputstr, int maxChar){
	printf("inside Insert() function\n");
	//printf("%s\n", inputstr);
	size_t strlength0 = strlen(inputstr);
	//printf("I: strlength of inputstr = %ld\n", strlength0);
	int strlength = maxChar;
	int j = 0;	
	int availbytes;
	pthread_mutex_lock(&mutxB);
	printf("I: string size = %d bIndex(before) = %d\n", strlength, bIndex);
	availbytes = MAX - bIndex;  //MAX and bIndex are global
	if (availbytes < strlength)  //wait if not enough space
		pthread_cond_wait(&cvParser, &mutxB); 

	for (int i=bIndex; i<bIndex + strlength; i++){
		buffer[i] = inputstr[j];  //work being done here filling the buffer
		j++;
	}
	//bIndex is the number of chars in buffer array including the last \n char	
	//bIndex is also the index of the next avail spot in the buffer 
	bIndex = bIndex + strlength;  //changing global var
	
	pthread_cond_signal(&cvConvert);  //cond variable: wakeup converter, data in the buffer

	printf("I: buffer =\n%s\n", buffer);
	printf("I: availbytes = %d bIndex(after) = %d\n", availbytes, bIndex);
	pthread_mutex_unlock(&mutxB);
	return;

}



//function to determine number of chars(bytes) in a file
int Sizefile(char *filenom){
	FILE *fp;
	char c;
	int count = 0;
	fp = fopen(filenom,"r");	
	if (fp == 0) {printf("could not open file\n"); return(-1);}
	c = fgetc(fp);
	//test the first character for empty file
	if(c=='\n' || c=='\0' || c==EOF) {printf("empty file\n"); return(-1);}
  
	while(c != EOF){	//is there a builtin C function to do this?
		count++;	
		c = fgetc(fp);
	}
	
	return count ; // add one so EOF char is included in total count???
}


//function reads a file and stores its Url names as char array called inpstr; it then 
//calls Insert() which takes the inputstr and fills the buffer 
void* Parser(char *filenom){
	char thrdID[sizeS];  
	char message[sizeS];
	strcpy(message, filenom);
	int n;
	FILE *fp;
	char *mode = {"r"}; //we will only read the file
	int maxChar = Sizefile(filenom); 
	printf("P: filename = %s filesize char = %d \n", filenom, maxChar);
	fp = fopen(filenom, mode);
	if (fp == 0) {printf("could not open file\n");}
	int a;
	int c; //this is one char in the string
	char *inputstr = malloc(sizeof(char)*(maxChar+1));
	if (inputstr == NULL){ printf("memory not allocated\n");} //return -1 here!!!

	for( a=0; a < maxChar; a++){
		c=fgetc(fp);
		inputstr[a] = c;
		//printf("inputstr[%d] = %d\n", a, inputstr[a]);
	}
	inputstr[a] = '\0'; //terminate string with \0	
	printf("P: inputstr[%d] = %d\n", a, inputstr[a]);
	//printf("%s\n", inputstr);
	Insert(inputstr, maxChar);		//call Insert() to write to buffer
	fclose(fp); 				//close file
	free(inputstr);				//free memory


	sprintf(thrdID, " thread %ld finished\n", pthread_self());	
	strcat(message, thrdID);

	pthread_mutex_lock(&mutxPfile);
 	Appendfile(message, parserFile);	//write to status file -- mutex
	pthread_mutex_unlock(&mutxPfile);	

	return (void*) 0; 
	//pthread_exit(0);  
} 

//Converter() takes the last url from buffer and stores as char* urlstring;  urlstring is processed by dnslookup()
// and the string is appended to consumerOut.txt; it returns -1 on error, 0 on sucess
int Converter(void){
	printf("inside consumer() function: bIndex = %d\n", bIndex);
	char ipnumASAstr[sizeS]; //url as a string
	char urlstring[maxstrlen];
	int j;
	int offsetn = 0;
	
//	if(bIndex != 0){

	pthread_mutex_lock(&mutxB);
	if(bIndex == 0)			//wait if nothing in buffer
		pthread_cond_wait(&cvConvert, &mutxB);

		for(int i=bIndex-2; i>=0; i--){  //what about the case when only one url?
			if(buffer[i] != '\n'){
				offsetn = offsetn + 1;
			}
			else{
				printf("offsetn = %d\n", offsetn);
				break;
			}
		}
		printf("offsetn = %d bIndex(before) = %d\n", offsetn, bIndex);
		//subtract by one to maintain bIndex defined properties 
		bIndex = bIndex - offsetn - 1;  //change in global variable!!!
		printf("bIndex(after) = %d\n", bIndex);
		int i=0;
		
		j = bIndex;

		//while(buffer[j]){
		for(j; j< bIndex + offsetn + 1; j++){ 
			//fputc(buffer[j], fp); //write to file
			urlstring[i]=buffer[j];
			printf("urlstring[%d] = buffer[%d] = %c\n", i, j, buffer[j]);
			i++;
		}

		urlstring[i-1] = '\0';  //make sure to terminate the string \0 and overwriting \n
					//newline char must be removed before using dnslookup() 
		dnslookup(urlstring, ipnumASAstr, sizeS);

		printf("%s\n", ipnumASAstr);
		strcat(urlstring, ", ");
		strcat(urlstring, ipnumASAstr);
		strcat(urlstring, "\n");
		pthread_cond_signal(&cvParser);
		pthread_mutex_unlock(&mutxB);

		pthread_mutex_lock(&mutxCfile);
		Appendfile(urlstring, converterFile);  //!!!mutex here
		pthread_mutex_unlock(&mutxCfile);
		
		//check urlstring length for max chars requirement
		if (i>maxstrlen){printf("exceeded urlstring length\n"); return (-1);}
	
	
	return 0;
}

//function which loops through the Parser() function according to 
//number of input files and threads  note: number of files should be >1 and a multiple of 
//the number of threads; number of threads should be greater than 1 
void* LoopParser(int count_i){
	int i;
	i=count_i;
	printf("count_i = %d\n",i);
	int remainder = inpfileCount % numthreads;
	int quotient = inpfileCount/numthreads;
	if (remainder == 0){
		//for(int i=0; i < numthreads; i++)
			for(int j = 0; j < quotient; j++)
				Parser(argvx[1 + j + quotient*i]);
	}

}


//function which loops through the Converter() according to the number of 
//urls in an input file for each converter thread
void* LoopConverter(int count_j){
	int i;
	i = count_j;
	int countNL = 0;
	FILE *fp;
	char c;
	int remainder = inpfileCount % numthreads;
	int quotient = inpfileCount/numthreads;
	if (remainder == 0){
		//for(int i=0; i < numthreads; i++)
			for(int j = 0; j < quotient; j++){
				fp = fopen(argvx[1 + j + quotient*i], "r");
				c = fgetc(fp);	

				while(c != EOF){
					if( c == '\n')
						countNL++;
						c = fgetc(fp);
				}
	
				printf("numloops = %d\n", countNL);
				for (int i=0; i < countNL; i++)
					Converter();
				countNL = 0;
			}		
	}	

	return (void*) 0;
}




