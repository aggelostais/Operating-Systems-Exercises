#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

//fd: file descriptor arxeiou eksodou, buff: dedomena pros eggrafi, len: arithmos dedomenon pros eggrafi
void doWrite(int fd, const char * buff,size_t len) //len was suggested to be of type int
{
        size_t idx = 0; //index pou deixnei posa bytes exoume grapsei
        ssize_t wcount;

        do{
                wcount = write(fd, buff + idx, len - idx); 
                //Grapse apo tin arxiki thesi buff+index,len-idx aritmho bytes
                //wcount posa bytes graftikan
                if(wcount == -1)
                {
                        perror("write");
                        exit(1);
                }

                idx += wcount;
                //proxorame ton index toso oso ta byte pou graftikan
        } while(idx < len);
        //Oso o index einai mikroteros ton synolikon bytes sinexise tin eggrafi
}

//fd: file descriptor arxeiou output, infile:onoma arxeiou eisodou
void write_file(int fd, const char * infile)
{
        int read_fd; 
        read_fd = open(infile, O_RDONLY); //read_fd o file descriptor tou arxeiou 
                                          //pou tha diavastei
        if( read_fd == -1){
                perror("open");
                exit(1);
        }

        char buff[1024]; //buffer pou apothikeuei ta dedomena anagnosis
        ssize_t rcount;  //To arxiko itan size_t alla to size_t de pairnei arnitikes times

        rcount = read(read_fd, buff, 1023);
        //Diavase mexri 1023 bytes apo to arxeio me fd read_fd kai vale ta dedomena ston buff
        //rcount o arithmos dedomenon pou diavastikan
        if(rcount == -1){
                perror("read");
                exit(1);
        }

        while(rcount > 0){ //not End Of File
                doWrite(fd, buff, rcount); //grafei ta dedomena pou yparxoun ston buffer sto arxeio eksodou
                rcount = read(read_fd, buff, 1023);
                //Ksanadiavazontai alla 1023 bytes (an ginetai) kai bainoun ston buffer
        }

        if(rcount == -1) // check if exited while due to a read error 
        {
                perror("read");
                exit(1);
        }

        close(read_fd);
}

int main(int argc, char** argv)
{
        if(argc < 3 || argc > 4) 
        //An den exoun dothei 2 arxeia eisodou h exoun dothei parapano apo 3
        { //error
                printf("Impropper call.\n");
                printf("Usage: ./exec input_file_1 input_file_2 ");
                printf("output_file[default: fconc.out]\n");
                return 0;
        }

        int fd_out;

        if(argc == 3)
        { // use default output
                fd_out = open("fconc.out", O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
        else if(argc ==4)
        { // use argv[3] for output
                //An ena apo ta input files einai idio me to output
		 if(strcmp(argv[1], argv[3]) == 0 || strcmp(argv[2], argv[3]) == 0 )
	        {
                	printf("error: output file needs to be different from input files\n");
			return 0;
        	}
                fd_out = open(argv[3], O_WRONLY|O_TRUNC);
                //Anoixe to 3o arxeio mono gia eggrafi kai an yparxei idi svise ta periexomena 
        }

        write_file(fd_out, argv[1]); //Diavazetai kai grafontai ta dedomena tou protou arxeiou
        write_file(fd_out, argv[2]); 

        close(fd_out);

        return 0;
}
