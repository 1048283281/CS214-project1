#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>

#define BUFFER_SIZE 1

typedef struct {
    size_t length;
    size_t used;
    char*data;
} strbuf_t;

int errorcheck = 0;
int sb_init(strbuf_t *L, size_t length)
{
    L->data = malloc(sizeof(char) * length);
    if (!L->data) return 1;

    L->length = length;
    L->used = 0;
    L->data[0] = '\0';
    return 0;
}

int sb_append(strbuf_t *L, char item)
{
    if (L->used == L->length) {
		size_t size = L->length * 2;
		char *p = realloc(L->data, sizeof(char) * (size+1));
		if (!p) return 1;

		L->data = p;
		L->length = size;

    }

    L->data[L->used] = item;
    ++L->used;
    L->data[L->used+1] = '\0';

    return 0;
}

int wrapping(int infd,int outfd,int size){
	char buf[BUFFER_SIZE];
    strbuf_t word;
    strbuf_t space;
    int currsize = 0, counter = 0;
    int ifParagraph = 0;
    sb_init(&word, size);
    sb_init(&space, size);
    while(read(infd, buf, BUFFER_SIZE)){
        if(isspace(buf[0]) == 0){//stop by the first non space char, skip everything before that
            sb_append(&word, buf[0]);
            break;
        }
    }
    while(read(infd, buf, BUFFER_SIZE)){
        if(isspace(buf[0]) == 0){//meet 
            for(int i = 0; i < space.used; i++){
                if(space.data[i] == '\n'){
                    counter +=1;
                }
            }
            if(counter >= 2){
                write(outfd, "\n\n", 2);
                ifParagraph=1;
            }
            sb_append(&word, buf[0]);
            free(space.data);
            sb_init(&space,size);
            counter = 0;
        }
        else {//meet whitespace
            sb_append(&space, buf[0]);
            if(word.used==0){
            	continue;
            }
            if(word.used > size){
                write(outfd, "\n", 1);
                write(outfd, word.data, word.used);
                write(outfd, "\n", 1);
                errorcheck = 1;
            }
            else{
                if((currsize + word.used) < size){//normal 
                	if(currsize==0||ifParagraph){
                		write(outfd, word.data, word.used);
                		currsize = word.used;
                		ifParagraph=0;
                	}
                	else{//most normal, 
                		write(outfd, " ", 1);
                		write(outfd, word.data, word.used);
                		currsize += word.used+1;
                	}
                }
                else if((currsize + word.used) == size){
                	if(currsize==0||ifParagraph){// if the length of a word ==width
                		write(outfd, word.data, word.used);
                		currsize = word.used;
                		ifParagraph=0;
                	}
                	else{//as the final word need to add another space, it needs newline
                		write(outfd, "\n", 1);
                		write(outfd, word.data, word.used);
                		currsize = word.used;
                	}
                }
                else{//put word to newline
                	if(ifParagraph){
                		write(outfd, word.data, word.used);                   	
                    	currsize = word.used;
                    	ifParagraph=0;
                	}
                	else{
                		write(outfd, "\n", 1);
                    	write(outfd, word.data, word.used);
                   	 	currsize = word.used;
                	}	
           
                }
            }
            free(word.data);
            sb_init(&word, size);
        }
    }
    //for the fianl word
    if(word.used>0){//final word is not space
    	if((currsize + word.used) < size){
    		if(ifParagraph){
            	write(outfd, word.data, word.used);                   	
            	ifParagraph=0;
        	}
        	else{
        		write(outfd, " ", 1);
            	write(outfd, word.data, word.used);
        	}
    	}
    	else{//(currsize + word.used) > size
    		if(ifParagraph){
            	write(outfd, word.data, word.used);                   	
            	ifParagraph=0;
        	}
        	else{
        		write(outfd, "\n", 1);
            	write(outfd, word.data, word.used);
        	}
    	}
    	write(outfd, "\n", 1);
    }
    else{//space.data!=NULL, so the final word is space
    	write(outfd, "\n", 1);
    }
    free(space.data);
    free(word.data);
    return 0;
}

int main(int argc, char** argv){
    DIR *dir;
    int size = atoi(argv[1]);
    assert(size > 0);
    if(argc < 3){
        wrapping(0,1,size);
    }
    else{
        if ((dir = opendir (argv[2])) == NULL) {//cannot open directory
            int fd = open(argv[2], O_RDONLY);   
            if(fd == -1){//cannot open file
                perror("Error");
                exit(EXIT_FAILURE);
            }
            else{
                wrapping(fd, 1, size);
                close(fd);
            }
        }
        else{
            struct dirent *de;
            de = readdir(dir);//skip . and .. directory
            de = readdir(dir);
            char prefix[10] = "wrap.";
            while ((de = readdir(dir)) != NULL) {//read every entry in the given directory
                char filename[256] = "\0";
                char path[256] = "\0";
                strcat(path, argv[2]);
                strcat(path, "/");
                strcat(path, de->d_name);
                strcat(filename, path);
                strtok(filename, "/");
                strcat(filename, "/");
                strcat(filename, prefix);
                strcat(filename, de->d_name);
                if(strcmp(strtok(de->d_name, "."), "wrap") == 0){//the file has prefix wrap. , ignore it
                    continue;
                }
                int infd = open(path, O_RDONLY);
                int outfd = open(filename, O_WRONLY|O_TRUNC|O_CREAT, 0600);
                wrapping(infd, outfd, size);
                close(infd);
                close(outfd);
            }
            closedir(dir);
        }
    }
    if(errorcheck == 1){
        exit(EXIT_FAILURE);
    }
    else{
        return EXIT_SUCCESS;
    }
}

