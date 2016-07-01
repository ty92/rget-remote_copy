#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>


int delete_dir(char *filename) {
        DIR *ptr;
        struct dirent *direntp;
        struct stat sbuf;
        char current_dir[256]={0};

        if(getcwd(current_dir,255) ==NULL) {
                perror("getcwd");
                exit(EXIT_FAILURE);
	}	
	current_dir[255] = '\0';	

	printf("stat filename %s\n",filename);
        if(stat(filename,&sbuf) == -1) {
                perror("stat dir");
                exit(EXIT_FAILURE);
	}

        if(S_ISDIR(sbuf.st_mode)) {
                if((ptr = opendir(filename)) != NULL) {
			chdir(filename);
                        while((direntp = readdir(ptr)) != NULL)
				if(strncmp(direntp->d_name,".",1) != 0 && strncmp(direntp->d_name,"..",2) != 0)
                                	if(delete_dir(direntp->d_name) != 0)
						printf("delete failed %m\n"),exit(-1);
			chdir(current_dir);
                        if(rmdir(filename) == -1) {
                                fprintf(stderr,"not permission rmdir %s\n",filename);
                                exit(EXIT_FAILURE);
                        }
		}
        }else if(S_ISREG(sbuf.st_mode)){
                if(unlink(filename) == -1) {
                        fprintf(stderr,"not permission unlink file %s\n",filename);
                                exit(EXIT_FAILURE);
                }
        } else {
                fprintf(stderr,"%s is not a dir or regularfile\n",filename);
                exit(EXIT_FAILURE);
        }
	return 0;
}


int main(int argc, char **argv){
	delete_dir(argv[1]);
	return 0;
}

