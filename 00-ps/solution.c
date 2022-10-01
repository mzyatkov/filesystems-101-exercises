#include "solution.h"
#include <sys/types.h>	
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ENV_SIZE 100
#define ENV_VAR_SIZE 256


struct process_info {
	pid_t pid;
	char *exe;
	char **argv;
	char **envp;
    size_t argv_size;
    size_t envp_size;
};


int read_cmdline(struct process_info *process_info) {
    char newpathname[PATH_MAX];
    sprintf(newpathname, "/proc/%d/cmdline", process_info->pid);
    FILE* fp = fopen(newpathname, "r");

    process_info->argv_size = ENV_SIZE;
    process_info->argv = calloc(process_info->argv_size, sizeof(char *));
    if(fp) {
        size_t len = 0;
        for (size_t i = 0; i < process_info->argv_size; i++) {
            if (i == process_info->argv_size - 1) {
                process_info->argv_size *= 2;
                process_info->argv = realloc(process_info->argv, process_info->argv_size*sizeof(char *));
            }
            if ((getdelim(&process_info->argv[i], &len, ' ', fp)) == -1) {
                // process_info->argv[i+1] = NULL;
                break;
            }
        }
        process_info->exe = strdup(process_info->argv[0]);
        fclose(fp);
    } else {
        report_error(newpathname, errno);
        return 1;
    }
    return 0;
}
int read_environ(struct process_info *process_info) {
    char newpathname[PATH_MAX];
    sprintf(newpathname, "/proc/%d/environ", process_info->pid);
    FILE* fp = fopen(newpathname, "r");

    process_info->envp_size = ENV_SIZE;
    process_info->envp = calloc(process_info->envp_size, sizeof(char *));
    if(fp) {
        size_t len = 0;
        for (size_t i = 0; i < process_info->envp_size; i++) {
            if (i == process_info->envp_size - 1) {
                process_info->envp_size *= 2;
                process_info->envp = realloc(process_info->envp, process_info->envp_size * sizeof(char *));
            }
            if (getdelim(&process_info->envp[i], &len, '\0', fp) == -1) {
                // process_info->envp[i] = NULL;
                break;
            }
        }
        fclose(fp);
    } else {
        report_error(newpathname, errno);
        return 1;
    }
    return 0;
}

int get_process_info(struct process_info *process_info) {
    int err;
    err = read_cmdline(process_info);   
    if (err) {return err;}
    err = read_environ(process_info);
    if (err) {
        return err;
    }
    return 0;
}

int is_process(struct dirent *proc_entry) {
	if (proc_entry->d_type != DT_DIR) {return 0;}
	return atoi(proc_entry->d_name);
}



void ps(void)
{
	/* implement me */
	
	
	DIR *proc;
    struct dirent *proc_entry;

    proc = opendir("/proc");
    if (!proc) {
        report_error("/proc", errno);
        exit(1);
    };

	struct process_info *process_info = (struct process_info *)malloc(sizeof(struct process_info));
    while ( (proc_entry = readdir(proc)) != NULL) {
		if(!(process_info->pid = is_process(proc_entry))) {continue;} // skip if dir is not process
        // if (process_info->pid != 351557) {continue;}
		get_process_info(process_info);
        report_process(process_info->pid, process_info->exe, process_info->argv, process_info->envp);
    };


    closedir(proc);

}
