#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define ARG_MAX 32
#define CMDLINE_MAX 512
#define OUTOFRANGE 255
#define N_ARG 17

// For ease of use in errorMessage()
enum
{
    TOO_MNY_PRC_ARGS,
    ERR_MISSING_CMD,
    NO_OUTPUT_FILE,
    CANNOT_OPEN_FILE,
    MISLOCATED_OUTPUT,
    CANNOT_CD_INTO_DIR,
    COMMAND_NOT_FOUND,
    CANNOT_OPEN_DIR,
};

/* Swap characters*/
void charSwitch(char *cmd)
{
    int cmdLen = strlen(cmd);
    for (int i = 0; i < cmdLen; i++)
    {
        if (cmd[i] == '|' || cmd[i] == '&')
        {
            cmd[i] = ' ';
        }
    }
}

/* Completed Status Handling*/
void completedProcess(int argNum, char *cmd, int retarr[])
{
    switch (argNum)
    {
    case 1:
        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, retarr[0]);
        break;
    case 2:
        fprintf(stderr, "+ completed '%s' [%d][%d]\n", cmd, retarr[0], retarr[1]);
        break;
    case 3:
        fprintf(stderr, "+ completed '%s' [%d][%d][%d]\n", cmd, retarr[0], retarr[1], retarr[2]);
        break;
    case 4:
        fprintf(stderr, "+ completed '%s' [%d][%d][%d][%d]\n", cmd, retarr[0], retarr[1], retarr[2], retarr[3]);
        break;
    }
}

/*Parsing Error Handling*/
void errorMessage(int error)
{
    switch (error)
    {
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
    case CANNOT_CD_INTO_DIR:
        fprintf(stderr, "Error: cannot cd into directory\n");
        break;
    case COMMAND_NOT_FOUND:
        fprintf(stderr, "Error: command not found\n");
        break;
    case CANNOT_OPEN_DIR:
        fprintf(stderr, "Error: cannot open directory\n");
        break;
    }
}

/* Trims leading and trailing spaces of a string */
char *trimSpaces(char *str)
{
    char *end;

    // Trim leading space
    while (*str == ' ')
    {
        str++;
    }

    // Str is at the end, return
    if (*str == '\0')
    {
        return str;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (*end == ' ')
    {
        end--;
    }

    // Write \0 at the end
    *(end + 1) = '\0';

    return str;
}

/* Split str by delimiter into dest, return the count*/
int splitStrToArr(char *str, char *delimiter, char *dest[])
{
    int argc = 0;
    char *token;
    while ((token = strsep(&str, delimiter)) != NULL)
    {
        token = trimSpaces(token);
        if (!strcmp(token, ""))
        {
            continue;
        }
        dest[argc] = token;
        argc++;
    }
    return argc;
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
// pwd
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
    int e = chdir(argv[1]) ? 1 : 0;
    return e;
}

/* exit */
void exitCMD(char *cmd)
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

// check if the process is built-in, otherwise fork and call execvp(), returns exit code
int executeProcess(char *argv[])
{
    /* System() -> fork + exec + wait */
    int retval;

    if (!strcmp(argv[0], "pwd"))
        retval = pwd(argv);
    else if (!strcmp(argv[0], "sls"))
    {
        retval = sls();
        if (retval == 1)
        {
            errorMessage(CANNOT_OPEN_DIR);
        }
    }
    else
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            retval = execvp(argv[0], argv);
            exit(retval);
        }
        else if (pid > 0)
        {
            // this is the parent
            waitpid(pid, &retval, 0);
            retval = WEXITSTATUS(retval);
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
    // Initialize some variables
    char cmd[CMDLINE_MAX], ocmd[CMDLINE_MAX], tempCmd[CMDLINE_MAX];
    char *tempCmds[17];

    while (1)
    {
        char *nl;
        int stdoutRedirect = 0;
        int stderrRedirect = 0;

        /* Print prompt */
        printf("sshell$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        // If nothing is entered, do nothing
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
        char *splitCmdFile[1];

        if (strstr(cmd, ">&") != NULL)
        {
            stdoutRedirect = 1;
            stderrRedirect = 1;
            delim = ">";
            strcpy(cmd, ocmd);
        }
        else if (strstr(cmd, ">") != NULL)
        {
            stdoutRedirect = 1;
            delim = ">";
            strcpy(cmd, ocmd);
        }

        // Get output file name
        if (delim != NULL)
        {
            splitStrToArr(cmd, delim, splitCmdFile);
            strcpy(cmd, splitCmdFile[0]);
            filename = splitCmdFile[1];
            if (stderrRedirect)
            {
                while (filename[0] == '&' || filename[0] == ' ')
                {
                    filename++;
                }
            }

            // Error: no command // We need to check after we split out the filename whether there is a command
            if (!strcmp(cmd, ""))
            {
                errorMessage(ERR_MISSING_CMD);
                continue;
            }

            // Check if there is a filename
            if (!strcmp(filename, ""))
            {
                errorMessage(NO_OUTPUT_FILE);
                continue;
            }

            // Check if there is a | or |& in filename
            if (strstr(filename, "|") != NULL || strstr(filename, "|&") != NULL)
            {
                errorMessage(MISLOCATED_OUTPUT);
                continue;
            }

            // Check if we can open the file
            int testfile = open(filename, O_WRONLY | O_CREAT, 0644);
            if (testfile == -1)
            {
                errorMessage(CANNOT_OPEN_FILE);
                continue;
            }
            close(testfile);
        }

        // check the total arg count. first replace all | and & with space, second count how many arg we have
        // if arg count is > 16 we break
        strcpy(tempCmd, cmd);
        charSwitch(tempCmd); // this replaces all | and & with spaces
        int argc = splitStrToArr(tempCmd, " ", tempCmds);
        if (argc > N_ARG - 1)
        {
            errorMessage(TOO_MNY_PRC_ARGS);
            continue;
        }

        /* split command into piping */
        int cmdc = occur(cmd, '|') + 1;
        char *cmds[3];

        splitStrToArr(cmd, "|", cmds);

        // Error piping, if errorPiping[i] == 1 then that process's error needs to be piped 
        int errorPiping[3];
        for (int i = 1; i < cmdc; i++)
        {
            // strcpy(tempCmd, cmds[i]);
            if (cmds[i][0] == '&')
            {
                errorPiping[i - 1] = 1;
                cmds[i]++;
                while (cmds[i][0] == ' ')
                {
                    cmds[i]++;
                }
            }
            else
            {
                errorPiping[i - 1] = 0;
            }
        }
        errorPiping[cmdc - 1] = 0;

        // Error: no command
        int missingCmdFlag = 0;
        for (int i = 0; i < cmdc; i++)
        {
            if (!strcmp(cmds[i], ""))
            {
                errorMessage(ERR_MISSING_CMD);
                missingCmdFlag = 1;
                break;
            }
        }
        if (missingCmdFlag)
            continue;

        // Now we're ready to execute, but first create some pipes and file descriptors
        pid_t pid;
        int fd[2];
        int retarr[3];
        int OrigStdout = dup(STDOUT_FILENO);
        int OrigStdin = dup(STDIN_FILENO);
        int OrigStderr = dup(STDERR_FILENO);
        int TempStdin = dup(STDIN_FILENO);

        for (int i = 0; i < cmdc; i++)
        {
            char *argv[N_ARG];
            int retval;
            int argc = splitStrToArr(cmds[i], " ", argv);
            argv[argc] = NULL;

            pipe(fd);

            /* Builtin command is detected, these can not be piped */

            if (!strcmp(argv[0], "exit"))
                exitCMD(cmd);
            else if (!strcmp(argv[0], "cd"))
            {
                retval = cd(argv);
                if (retval == 1)
                {
                    errorMessage(CANNOT_CD_INTO_DIR);
                }
                continue;
            }

            pid = fork();
            if (pid != 0)
            {
                // this is the parent
                waitpid(pid, &retval, 0);
                close(fd[1]);
                dup2(fd[0], TempStdin);
                close(fd[0]);
                // check if the command exist. if the command doesn't exist, retval will be 65280
                if (retval == 65280)
                {
                    retval = 1;
                    errorMessage(COMMAND_NOT_FOUND);
                }
                else
                {
                    retval = WEXITSTATUS(retval);
                }
                retarr[i] = retval;
            }
            else if (pid == 0)
            {
                // this is the child

                /* checking input logic */
                dup2(TempStdin, STDIN_FILENO);
                close(TempStdin);
                close(fd[0]);

                /* checking output logic */
                // output to pipe
                if (cmdc > 1 && i != (cmdc - 1))
                {
                    if (errorPiping[i] == 1)
                    {
                        dup2(fd[1], STDERR_FILENO);
                    }
                    dup2(fd[1], STDOUT_FILENO);

                    close(fd[1]);
                }
                // output to stdout and stderr, no piping
                else if ((stdoutRedirect == 0) && (!(cmdc > 1) || (i == (cmdc - 1))))
                {
                    dup2(OrigStdout, STDOUT_FILENO);
                    dup2(OrigStderr, STDERR_FILENO);
                    close(fd[1]);
                }
                // if there is a file, redirect stdout and/or stderr, and if last command
                else
                {
                    int outputFile = open(filename, O_WRONLY | O_CREAT, 0644);
                    dup2(outputFile, STDOUT_FILENO);
                    if (stderrRedirect == 1)
                        dup2(outputFile, STDERR_FILENO);
                    close(outputFile);
                    close(fd[1]);
                }
                //if only one pipe reset Stdout
                retval = executeProcess(argv);
                exit(retval);
            }

        } // end for loop

        // restore original file descriptors
        dup2(OrigStdin, STDIN_FILENO);
        dup2(OrigStdout, STDOUT_FILENO);
        dup2(OrigStderr, STDERR_FILENO);
        close(fd[1]);
        close(fd[0]);

        completedProcess(cmdc, ocmd, retarr);
    } // end while
}
