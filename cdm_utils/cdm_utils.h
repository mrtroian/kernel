#ifndef _CDM_UTILS_H
#define _CDM_UTILS_H

#define DEVPATH "/dev/cdm."

void	defname(char *devpath, char *program_name);
int		device_read(char *devpath);
int		device_read_l(char *devpath);
int		device_write(char *devpath, char *msg);
int		get_free_space(char *devpath);
int		clear(char *devpath);
int		undo(char *devpath);

#endif