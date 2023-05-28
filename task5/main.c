#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
int main(){
    
    pid_t pid;
    // OPEN FILES
    int fd;
    fd = open("test.txt" , O_RDWR | O_CREAT | O_TRUNC);
    if (fd == -1)
    {
        fprintf(stderr, "fail on open %s\n", "test.txt");
        return -1;
        /* code */
    }
    //write 'hello fcntl!' to file
    write(fd, "hello fcntl!", 12);
    /* code */

    

    // DUPLICATE FD
    int fd1= fcntl(fd, F_DUPFD);
    /* code */
    

    pid = fork();

    if(pid < 0){
        // FAILS
        printf("error in fork");
        return 1;
    }
    
    struct flock fl;

    if(pid > 0){
        // PARENT PROCESS
        //set the lock
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        fcntl(fd, F_SETLK, &fl);
        //append 'b'
        write(fd, "b", 1);
        //unlock
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        sleep(3);

        // printf("%s", str); the feedback should be 'hello fcntl!ba'
        
        exit(0);

    } else {
        // CHILD PROCESS
        sleep(2);
        //get the lock
        fcntl(fd1, F_GETLK, &fl);
        //append 'a'
        write(fd1, "a", 1);
        exit(0);
    }
    close(fd);
    return 0;
}