#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define ARG_MAX 32
#define OUTOFRANGE 255
#define N_ARG 17

enum{
    TOO_MNY_PRC_ARGS,
    ERR_MISSING_CMD,
    NO_OUTPUT_FILE,
    CANNOT_OPEN_FILE,
    MISLOCATED_OUTPUT,
};

/* Swap characters*/
void char_switch(char *cmd){
    int cmd_len = strlen(cmd);
    for(int i = 0;i < cmd_len; i++){
        if(cmd[i] == '|' || cmd[i] == '&'){
            cmd[i] = ' ';
        }
    }
}

/*Parsing Error Handling*/
void error_message(int error){

    switch(error){
        case TOO_MNY_PRC_ARGS:
            fprintf(stderr, "Error: too many process arguments\n");
            break;
        case ERR_MISSING_CMD:
            fprintf(stderr, "Error: missing command\n");
            break;
        case NO_OUTPUT_FILE:
            fprintf(stderr, "Error: no output file\n");
            break;
        case CANNOT_OPEN_FILE:
            fprintf(stderr, "Error: cannot open output file\n");
            break;
        case MISLOCATED_OUTPUT:
            fprintf(stderr, "Error: mislocated output redirection\n");
            break;
    }

}
/* Split str by delimiter into dest*/
int splitStrtoArr(char *str, char *delimiter, char *dest[])
{
    int count = 0;
    char *arg = strtok(str, delimiter);
    dest[0] = arg;
    while (arg)
    {
        while (arg[0] == ' '){
            arg++;
        }
        dest[count] = arg;
        count++;
        arg = strtok(NULL, delimiter);
    }
    //printf("0:%s 1:%s\n", dest[0], dest[1]);
    return count;
}

/* Finds occurances in a str*/
int occur(char *s, char c)
{

    int count = 0;

    for (int i = 0; s[i]; i++)
    {
        if (s[i] == c)
        {
            count++;
        }
    }
    return count;
}

/* Built-in commands */
int pwd(char *argv[])
{
    if (argv[1])
    {
        return 255;
    }
    char *buffer = NULL;
    buffer = getcwd(buffer, 128);
    fprintf(stdout, "%s\n", buffer);
    return 0;
}

/* cd */
int cd(char *argv[])
{
    if (!argv[1])
    {
        return 255;
    }
    int e = chdir(argv[1]);
    return e;
}

/* exit */
void exit_cmd(char *cmd)
{
    fprintf(stderr, "Bye...\n");
    fprintf(stderr, "+ completed '%s' [0]\n", cmd);
    exit(0);
}

/* sls */
int sls()
{

    struct dirent *dp;
    struct stat sb;

    DIR *dirp;
    dirp = opendir(".");
    int retval = dirp ? 0 : 1;
    while ((dp = readdir(dirp)) != NULL)
    {
        if (dp->d_name[0] != '.')
        {
            stat(dp->d_name, &sb);
            fprintf(stdout, "%s (%lld bytes)\n", dp->d_name, (long long)sb.st_size);
        }
    }
    closedir(dirp);

    return retval;
}

int execute_process(char *cmd, char *filename, int output_redirect, int error_redirect)
{
    /* Parse command into array argv */
    char pcmd[strlen(cmd)];
    char *argv[N_ARG];
    int argc = 0;
    strcpy(pcmd, cmd); // copy cmd into another variable ocmd
    // put each arg into array, each arg is seperated by white spaces
    char *arg = strtok(pcmd, " ");
    // if an argument ends with '\ ', then this argument should be concatenated with the next argument together
    if (arg[strlen(arg) - 1] == '\\')
    {
        arg[strlen(arg) - 1] = ' ';
        strcat(arg, strtok(NULL, " "));
    }
    while (arg)
    {
        if (arg[strlen(arg) - 1] == '\\')
        {
            arg[strlen(arg) - 1] = ' ';
            strcat(arg, strtok(NULL, " "));
        }
        argv[argc] = arg;
        argc++; 
        arg = strtok(NULL, " ");
    }


    argv[argc] = NULL;
    /* System() -> fork + exec + wait */
    int retval;

    if (!strcmp(argv[0], "exit"))
        exit_cmd(cmd);
    else if (!strcmp(argv[0], "cd"))
        retval = cd(argv);
    else if (!strcmp(argv[0], "pwd"))
        retval = pwd(argv);
    else if (!strcmp(argv[0], "sls"))
        retval = sls();
    else
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // this is the child
            if (filename)
            {
                int fd = open(filename, O_WRONLY | O_CREAT, 0644);
                if (output_redirect == 1) dup2(fd, STDOUT_FILENO);
                if (error_redirect == 1) dup2(fd, STDERR_FILENO);
                close(fd);
            }
            retval = execvp(argv[0], argv);
            fclose(stdout);
            fclose(stderr);

            exit(retval);
        }
        else if (pid > 0)
        {
            // this is the parent
            waitpid(pid, &retval, 0);
            retval = WEXITSTATUS(retval);

            //Will be moved to the bottom of the code; after pipeline
            //fprintf(stderr, "+ completed '%s' [%d]\n", ocmd, WEXITSTATUS(retval));
        }
        else
        {
            exit(1);
            // error
        }
    }
    return retval;
}

int main(void)
{
    char cmd[CMDLINE_MAX], ocmd[CMDLINE_MAX], temp_cmd[CMDLINE_MAX];
    char *temp_cmds[17];

    while (1)
    {
        char *nl;
        int output_redirect = 0;
        int error_redirect = 0;

        /* Print prompt */
        printf("sshell$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);


        if (!strcmp(cmd, "\n"))
        {
            continue;
        }
        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO))
        {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        strcpy(ocmd, cmd);

        /* if there is an output direction '>' or ">&", 
        remove it and the filename after it and store it */
        char *filename = NULL;
        char *delim = NULL;
        char *outputPos = NULL;

        if (strstr(cmd, ">&") != NULL) {
            output_redirect = 1;
            error_redirect = 1;
            delim = ">&";
            strcpy(cmd, ocmd);
        }
        if (strstr(cmd, ">") != NULL){
            output_redirect = 1;
            delim = ">";
            strcpy(cmd, ocmd);
        }
        if (delim != NULL){
            outputPos = strstr(cmd, delim);
        }

        if (outputPos != NULL)
        {
            // user input an input file
            
            filename = outputPos + 1;
            if(filename[0] == '&') filename++;
            while (filename[0] == ' ')
                filename++;
            cmd[outputPos - cmd] = '\0';
        }
        // Error: no command 
        if (cmd == NULL || cmd[0] == '\n' || cmd[0] == '\0'){
            error_message(ERR_MISSING_CMD);
            continue;
        }

// check the total arg count. first replace all | and & with space, second count how many arg we have
// if arg count is > 16 we break
        strcpy(temp_cmd, cmd);
        char_switch(temp_cmd);
        int argc = splitStrtoArr(temp_cmd, " ", temp_cmds);
        if (argc > N_ARG - 1){
            error_message(TOO_MNY_PRC_ARGS);
            continue;
        }

        /* split command into piping */
        int cmdc = occur(cmd, '|') + 1;
        char *cmds[3];
        char *p_delim = "|";

        splitStrtoArr(cmd, p_delim, cmds);

        // FIXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        // printf("0:%s\n1:%d\n", cmds[0], *cmds[1]);
        // Error: no command 
        // int missing_cmd_flag = 0;
        // for (int i = 0; i < cmdc; i++){
        //     // printf("%s\n",cmds[i]);
        //     if (cmds[i] == NULL || cmds[i][0] == '\n' || cmds[i][0] == '\0'){
        //         error_message(ERR_MISSING_CMD);
        //         missing_cmd_flag = 1;
        //         break;
        //     }
        // }

        // if (missing_cmd_flag) continue;

        int fd1[2];
        int fd2[2];
        int fd3[2];
        int retarr[3];
        int outSave = dup(STDOUT_FILENO);
        int inSave = dup(STDIN_FILENO);
        pipe(fd1);
        pipe(fd2);
        pipe(fd3);
        



        if (fork() != 0)
        { /* Parent */
 //process 1
            /* No need for read access */
            close(fd1[0]);
            /* Process[0] will write output onto pipe*/
            if (cmdc > 1)
                dup2(fd1[1], STDOUT_FILENO);
            /* Close now unused FD */
            close(fd1[1]);
            /* Parent becomes process[0] */
            //FIX::
            retarr[0] = execute_process(cmds[0], filename, output_redirect, error_redirect);
        }
        else
        { /* Child */
            if (cmdc < 2)
            {
                exit(0);
            }
            if (fork() != 0)
            {
 // process 2                
 /* No need for write access to pipe 1 */
                close(fd1[1]);
                /* Process[1] will read from pipe */
                dup2(fd1[0], STDIN_FILENO);

                /*Process[1] must write onto next pipe */
                if (cmdc > 2)
                    dup2(fd2[1], STDOUT_FILENO);

                /* Close now unused FD */
                close(fd1[0]);
                close(fd2[1]);
                /* Child becomes process[1] */

                //FIX::
                retarr[1] = execute_process(cmds[1], filename, output_redirect, error_redirect);
            }
            else //process 3
            {
                if (cmdc < 3)
                    exit(0);

                if (fork() != 0)
                {
                    /*No need for write access to pipe 2 */
                    close(fd2[1]);

                    /*Process[2] will read from pipe 2 */
                    dup2(fd2[0], STDIN_FILENO);

                    close(fd2[0]);
                    close(fd3[1]);
                    /*This will be process[2] */
                    //FIX::

                    retarr[2] = execute_process(cmds[2], filename, output_redirect, error_redirect);
                }
                else
                {
                    if (cmdc < 4)
                        exit(0);

                    /*No need for write access to pipe 2 */
                    close(fd3[1]);

                    /*Process[2] will read from pipe 2 */
                    dup2(fd3[0], STDIN_FILENO);

                    close(fd3[0]);

                    /*This will be process[2] */

                    retarr[3] = execute_process(cmds[3], filename, output_redirect, error_redirect);
                    
                }
            }
        }

        //restoring original macros
        dup2(outSave,STDOUT_FILENO);
        dup2(inSave,STDIN_FILENO);
        // printf("This is to test stdout_fileno \n");
        
        if (cmdc == 1)
        {
            fprintf(stderr, "+ completed '%s' [%d]\n", ocmd, retarr[0]);
        }
        else if (cmdc == 2)
        {
            fprintf(stderr, "+ completed '%s' [%d][%d]\n",
                    ocmd, retarr[0], retarr[1]);
        }
        else if (cmdc == 3)
        {
            fprintf(stderr, "+ completed '%s' [%d][%d][%d]\n",
                    ocmd, retarr[0], retarr[1], retarr[2]);
        }
        else if (cmdc == 4)
        {
            fprintf(stderr, "+ completed '%s' [%d][%d][%d][%d]\n",
                    ocmd, retarr[0], retarr[1], retarr[2], retarr[3]);
        }

            //return EXIT_SUCCESS;
        }
    }


    /*Phase 2: https://stackoverflow.com/questions/11198604/c-split-string-into-an-array-of-strings */