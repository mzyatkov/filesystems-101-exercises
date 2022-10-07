#include <solution.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>

struct process_info
{
    pid_t pid;
};

int is_process(struct dirent *proc_entry)
{
    if (proc_entry->d_type != DT_DIR)
    {
        return 0;
    }
    return atoi(proc_entry->d_name);
}


void lsof(void)
{
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

		char name[sizeof("/proc/%u/fd/0123456789") + sizeof(int)*3];
		char *fdlink;
		size_t baseofs;
		baseofs = sprintf(name, "/proc/%u/fd/", process_info->pid);
		DIR *d_fd = opendir(name);
		if (d_fd) {
			while ((proc_entry = readdir(d_fd)) != NULL) {
				/* Skip entries '.' and '..' (and any hidden file) */
				if (proc_entry->d_name[0] == '.')
					continue;

				strncpy(name + baseofs, proc_entry->d_name, 10);
				fdlink = calloc(PATH_MAX, sizeof(char));
				if ((readlink(name, fdlink, PATH_MAX)) != -1) {
					report_file(fdlink);
					free(fdlink);
				} else {
					report_error(name, errno);
					free(fdlink);
				}
			}
			closedir(d_fd);
		}

    };

    free(process_info);
    closedir(proc);
}
