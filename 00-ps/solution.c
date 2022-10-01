#include "solution.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ENV_SIZE 128

struct process_info
{
    pid_t pid;
    char *exe;
    char **argv;
    char **envp;
    size_t argv_len;
    size_t envp_len;
};

int read_cmdline(struct process_info *process_info)
{
    char newpathname[PATH_MAX];
    sprintf(newpathname, "/proc/%d/cmdline", process_info->pid);
    FILE *fp = fopen(newpathname, "r");

    process_info->argv_len = ENV_SIZE;
    process_info->argv = (char **)calloc(process_info->argv_len, sizeof(char *));

    if (fp)
    {
        size_t len = 0;
        for (size_t i = 0; i < process_info->argv_len; i++)
        {
            if (i == process_info->argv_len - 1)
            {
                process_info->argv_len *= 2;
                process_info->argv = realloc(process_info->argv, process_info->argv_len * sizeof(char *));
            }
            process_info->argv[i] = NULL;
            if ((getdelim(&process_info->argv[i], &len, '\0', fp)) == -1)
            {
                free(process_info->argv[i]);
                process_info->argv[i] = NULL;
                break;
            }
            if (process_info->argv[i][0] == '\0') {
                free(process_info->argv[i]);
                process_info->argv[i] = NULL;
                break;
            }
        }
        fclose(fp);
    }
    else
    {
        report_error(newpathname, errno);
        return 1;
    }
    return 0;
}
int read_environ(struct process_info *process_info)
{
    char newpathname[PATH_MAX];
    sprintf(newpathname, "/proc/%d/environ", process_info->pid);
    FILE *fp = fopen(newpathname, "r");

    process_info->envp_len = ENV_SIZE;
    process_info->envp = (char **)calloc(process_info->envp_len, sizeof(char *));
    if (fp)
    {
        size_t len = 0;
        for (size_t i = 0; i < process_info->envp_len; i++)
        {
            if (i == process_info->envp_len - 1)
            {
                process_info->envp_len *= 2;
                process_info->envp = realloc(process_info->envp, process_info->envp_len * sizeof(char *));
            }
            process_info->envp[i] = NULL;
            if (getdelim(&process_info->envp[i], &len, '\0', fp) == -1)
            {
                free(process_info->envp[i]);
                process_info->envp[i] = NULL;
                break;
            }
            if (process_info->envp[i][0] == '\0') {
                free(process_info->envp[i]);
                process_info->envp[i] = NULL;
                break;
            }
        }
        fclose(fp);
    }
    else
    {
        report_error(newpathname, errno);
        return 1;
    }
    return 0;
}
int read_exe(struct process_info *process_info)
{
    char newpathname[PATH_MAX];
    sprintf(newpathname, "/proc/%d/exe", process_info->pid);
    process_info->exe = calloc(PATH_MAX , sizeof(char));
    return (readlink(newpathname, process_info->exe, PATH_MAX) == -1);
}
int get_process_info(struct process_info *process_info)
{
    int err;
    err = read_cmdline(process_info);

    err |= read_environ(process_info);

    err |= read_exe(process_info);
    if (err)
    {
        return err;
    }
    return 0;
}
int clean_process_info(struct process_info *process_info)
{
    for (size_t i = 0; i < process_info->argv_len; i++)
    {
        if (process_info->argv[i] == NULL)
        {
            break;
        }
        free(process_info->argv[i]);
    }
    for (size_t i = 0; i < process_info->envp_len; i++)
    {
        if (process_info->envp[i] == NULL)
        {
            break;
        }
        free(process_info->envp[i]);
    }
    if (process_info->exe)
    {
        free(process_info->exe);
    }
    if (process_info->argv)
    {
        free(process_info->argv);
    }
    if (process_info->envp)
    {
        free(process_info->envp);
    }
    return 0;
}
int is_process(struct dirent *proc_entry)
{
    if (proc_entry->d_type != DT_DIR)
    {
        return 0;
    }
    return atoi(proc_entry->d_name);
}

void ps(void)
{
    /* implement me */

    DIR *proc;
    struct dirent *proc_entry;

    proc = opendir("/proc");
    if (!proc)
    {
        report_error("/proc", errno);
        exit(1);
    };

    struct process_info *process_info = (struct process_info *)calloc(1, sizeof(struct process_info));
    while ((proc_entry = readdir(proc)) != NULL)
    {
        if (!(process_info->pid = is_process(proc_entry)))
        {
            continue;
        } // skip if dir is not process

        int err = get_process_info(process_info);
        if (!err) {
        report_process(process_info->pid, process_info->exe, process_info->argv, process_info->envp);
        }
        clean_process_info(process_info);
    };

    free(process_info);
    closedir(proc);
}
