#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define PARENTREAD 	c2p[0]
#define PARENTWRITE 	p2c[1]
#define CHILDREAD	p2c[0]
#define CHILDWRITE	c2p[1]

int main(int argc, char *argv[])
{
 	pid_t pid;
    	int p2c[2] = {-1, -1};
	int c2p[2] = {-1, -1};
    	FILE * sd;
	FILE * rd;
	char *audiofile;
	float gain;
	char trash[100];
	char error[20];
	int readyflag = 0, playflag = 0;
	int offset, status;
	char *dir = (char*)malloc(30*sizeof(char));
	char *originalpath = (char*)malloc(30*sizeof(char));
	char *saturatedpath = (char*)malloc(30*sizeof(char));
	char *fixedpath = (char*)malloc(30*sizeof(char));
	/// Inicializate variables
	memset(error, 0, sizeof(error));
	sprintf(dir, "/tmp/CSA%d/",getpid());
	mkdir(dir, 0777);

	sprintf(originalpath, "%saudio.raw",dir);
	sprintf(saturatedpath, "%ssaturated.raw",dir);
	sprintf(fixedpath, "%sfixed.raw",dir);
	
	/// Get arguments
	switch(argc){
		case 4:
			audiofile = argv[1];
			gain = atof(argv[2]);
			offset = (8000*atoi(argv[3]))/1000;
			break;

		case 5:
			audiofile = argv[1];
			gain = atof(argv[2]);
			offset = (8000*atoi(argv[3]))/1000;
			if( strcmp(argv[4], "p") == 0)
				playflag = 1;
			else {
				printf("Error in arg 4\n");
				exit(-1);
			} 
			break;
		
		default:
			printf("Error: %d arguments given\n",argc);
			exit(-1);
			break;
	}
       	FILE * test;
	if ( (test = fopen(audiofile, "rb")) <= 0) {
		printf("File not found\n");
		exit(-1);
	}
	fclose(test);
	printf("Starting ...\n");

     	/// Create the pipes.
    	if ((pipe(p2c) < 0) || (pipe(c2p) < 0)) {
        	perror("pipe");
        	exit(1);
    	}


	
     	// Create a child process with Octave
    	if ((pid = fork()) < 0) {
        	perror("fork");
        	exit(1);
    	}


	
     	// The child process executes "octave" with -iq option.
    	if (pid == 0) {
      		int junk;
      
        	/*
         	* Attach standard input of this child process to read from the pipe.
         	*/
        	dup2(CHILDREAD, 0);
       	 	close(PARENTREAD);
		close(PARENTWRITE);
        	junk = open("/dev/null", O_WRONLY);
        	/// Output to the parent trough a pipe 
		dup2(CHILDWRITE,1);
		/// throw away any error message 
		dup2(junk, 2);
          	execlp("octave", "octave", "-iq", (char *)0);
        	perror("exec");
        	exit(-1);
    	}
	
    	/// We won't be reading from the pipe.
  	close(CHILDWRITE);
	close(CHILDREAD);
	sd = fdopen(PARENTWRITE, "w");  /* to use fprintf instead of just write */
	/// Octave: Open Audio file
	fprintf(sd, "filename='%s';\n",audiofile);
    	fprintf(sd, "fid=fopen(filename,'rb');\n");
	fprintf(sd, "x=fread(fid,inf,'int16');\n");
	
	/// Octave: Operate Matrix
	fprintf(sd, "y=int16(x*%f);\n", gain);
	fprintf(sd, "z=int16(y/%f);\n",gain);
	fprintf(sd, "flag = 0;\n");
	fprintf(sd, "xx = [0 0 0 0];\n");
	fprintf(sd, "yy = [0 0 0 0];\n");
	
	/// Interpolation
	fprintf(sd, "for i = 1:length(y)\n");
	fprintf(sd, "  if ((y(i)>=intmax('int16') || y(i)<=intmin('int16'))&&flag==0)\n");
	fprintf(sd, "    xx(1) = i-2;\n");
	fprintf(sd, "    xx(2) = i-1;\n");
	fprintf(sd, "    yy(1) = z(i-2);\n");
	fprintf(sd, "    yy(2) = z(i-1);\n");
	fprintf(sd, "    flag = 1;\n");
	fprintf(sd, "  endif\n");
	fprintf(sd, "  if ((y(i)<intmax('int16') && y(i)>intmin('int16'))&&flag==1)\n");
	fprintf(sd, "    xx(3) = i;\n");
	fprintf(sd, "    xx(4) = i+1;\n");
	fprintf(sd, "    yy(3)=z(i);\n");
	fprintf(sd, "    yy(4)=z(i+1);\n");
	fprintf(sd, "    p = polyfit(xx, yy, 4);\n");
	fprintf(sd, "    for j = xx(2)+1:xx(3)-1\n");
	fprintf(sd, "      z(j) = polyval(p, j);\n");
	fprintf(sd, "    endfor\n");
	fprintf(sd, "    flag = 0;\n");
	fprintf(sd, "  endif\n");
	fprintf(sd, "endfor\n");
	/// Save audio in temp folder
	fprintf(sd, "saveaudio('%soriginal',x,'raw');\n",dir);
	fprintf(sd, "saveaudio('%ssaturated',y,'raw');\n",dir);
	fprintf(sd, "saveaudio('%sfixed',z,'raw');\n",dir);
	/// Octave: Plotting
	fprintf(sd, "subplot(3,1,1);\n");
	fprintf(sd, "plot(x(%d:%d));\n",offset,offset+320);
	fprintf(sd, "subplot(3,1,2);\n");
	fprintf(sd, "plot(y(%d:%d));\n",offset,offset+320);
	fprintf(sd, "subplot(3,1,3);\n");
	fprintf(sd, "plot(z(%d:%d));\n",offset,offset+320);

	/// Calculate the error. Also works as a signal that octave script has ended
	fprintf(sd, "error = sum(((z-x)).^2)\n");
	fprintf(sd, "printf('error=%%d', error);\n");
	fflush(sd);

	while (readyflag == 0 ){
		memset(trash, 0, sizeof(trash));
		read(PARENTREAD, trash, sizeof(trash));
		if( strcmp(trash, "error=")==0 ){
			read(PARENTREAD, error, sizeof(error));
			printf("Error: %s\n", error);
			readyflag = 1;
		}
	}
	
	/// If p arguments is set, play new audio with aplay (Not tested because no VM nor Server has SoundCard
	if( playflag ) {
		char command[50];
		FILE *pf;
		int i;
		for ( i=0; i< 3; i++){
			switch(i){
				case 0:
					sprintf(command, "aplay --format=S16_LE -t raw %s", originalpath);
					break;
				case 1:
					sprintf(command, "aplay --format=S16_LE -t raw %s", saturatedpath);
					break;
				case 2:	
					sprintf(command, "aplay --format=S16_LE -t raw %s", fixedpath);
					break;
				default:
					break;
			}
			if( (pf = popen(command, "r")) == NULL ) {
				printf("Error in popen aplay");
			}
			pclose(pf);
		}
	}
	printf("Press ENTER to exit");
	getchar();
    	fprintf(sd, "\n exit\n"); fflush(sd);

    	/*
     	* Wait for the child to exit and
     	* close de pipe.
     	*/
    	waitpid(pid, &status, 0);
    	fclose(sd);
	
	// Cleaning
	
	remove(originalpath);
	remove(saturatedpath);
	remove(fixedpath);
	rmdir(dir);
    	/*
     	* Exit with a status of 0, indicating that
     	* everything went fine.
     	*/
    	exit(0);
}

