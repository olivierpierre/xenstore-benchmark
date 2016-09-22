#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <xenstore.h>

#define BENCH_PATH		"/test"

char *config_file;
sig_atomic_t child_stop = 0;
pid_t *pid_list;

/* Number of iteration per operation */
typedef struct config_s
{
	int xs_read;
	int xs_write;
	int xs_close;
	int xs_open;
	int xs_get_domain_path;
	int xs_set_permission;
	int repeat;
	int req_size;
    int connection_num;
} config;

int bench_write(int n, int size, double *result);
int bench_read(int n, int size, double *result);
int bench_open(int n, double *result);
int bench_close(int n, double *result);
int bench_get_domain_path(int n, double *result);
int bench_set_permission(int n, double *result);

void print_usage(int argc, char **argv);
void default_config(config *cfg);
int parse_config_file(char *path, config *cfg);
void print_config(const config *cfg);
void get_random_string(char *s, const int len);
void avg_stdev(const double *array, int size, double *mean, double *std);
int spawn_connections(int num);
void connection_loop(int child_id);
void child_exit_signal(int signum);

int main(int argc, char **argv)
{ 
	int ret = 0, i, child_status;
	config cfg;
	double *time_res;
	double avg, std;

	if(argc == 2)
	{
		if(!strcmp(argv[1], "-h"))
		{
			print_usage(argc, argv);
			return 0;
		}

		config_file = argv[1];
		default_config(&cfg);
		if(parse_config_file(argv[1], &cfg))
			return -2;
		print_config(&cfg);
	}
	else
	{
		print_usage(argc, argv);
		return -1;
	}
    
    pid_list = (pid_t *)malloc(cfg.connection_num * sizeof(pid_t));
	time_res = (double *)malloc(cfg.repeat * sizeof(double));

    spawn_connections(cfg.connection_num);

    for(i=0; i<cfg.connection_num; i++)
        usleep(100000);

	if(cfg.xs_open)
	{
		for(i=0; i<cfg.repeat; i++)
		{
			ret = bench_open(cfg.xs_open, &time_res[i]);
			if(ret)
			{
				fprintf(stderr, "Error running OPEN benchmark\n");
				break;
			}
		}
		avg_stdev(time_res, cfg.repeat, &avg, &std);
		printf("%s;%f;%f\n", "open", avg, std);
	}

	if(cfg.xs_close)
	{
		for(i=0; i<cfg.repeat; i++)
		{
			ret = bench_close(cfg.xs_close, &time_res[i]);
			if(ret)
			{
				fprintf(stderr, "Error running CLOSE benchmark\n");
				break;
			}
		}
		avg_stdev(time_res, cfg.repeat, &avg, &std);
		printf("%s;%f;%f\n", "close", avg, std);
	}

    if(cfg.xs_write)
    {
        for(i=0; i<cfg.repeat; i++)
        {
            ret = bench_write(cfg.xs_write, cfg.req_size, &time_res[i]);
            if(ret)
            {
                fprintf(stderr, "Error running WRITE benchmark\n");
                break;
            }
        }
        avg_stdev(time_res, cfg.repeat, &avg, &std);
        printf("%s;%f;%f\n", "write", avg, std);
    }

	if(cfg.xs_read)
	{
		for(i=0; i<cfg.repeat; i++)
		{
			ret = bench_read(cfg.xs_read, cfg.req_size, &time_res[i]);
			if(ret)
			{
				fprintf(stderr, "Error running READ benchmark\n");
				break;
			}
		}
		avg_stdev(time_res, cfg.repeat, &avg, &std);
		printf("%s;%f;%f\n", "read", avg, std);
	}

	if(cfg.xs_close)
	{
		for(i=0; i<cfg.repeat; i++)
		{
			ret = bench_write(cfg.xs_write, cfg.req_size, &time_res[i]);
			if(ret)
			{
				fprintf(stderr, "Error running WRITE benchmark\n");
				break;
			}
		}
		avg_stdev(time_res, cfg.repeat, &avg, &std);
		printf("%s;%f;%f\n", "write", avg, std);
	}

	if(cfg.xs_get_domain_path)
	{
		for(i=0; i<cfg.repeat; i++)
		{
			ret = bench_get_domain_path(cfg.xs_get_domain_path, &time_res[i]);
			if(ret)
			{
				fprintf(stderr, "Error running GET_DOMAIN_PATH benchmark\n");
				break;
			}
		}
		avg_stdev(time_res, cfg.repeat, &avg, &std);
		printf("%s;%f;%f\n", "get_domain_path", avg, std);
	}

	if(cfg.xs_set_permission)
	{
		for(i=0; i<cfg.repeat; i++)
		{
			ret = bench_set_permission(cfg.xs_set_permission, &time_res[i]);
			if(ret)
			{
				fprintf(stderr, "Error running SET_PERMISSION benchmark\n");
				break;
			}
		}
		avg_stdev(time_res, cfg.repeat, &avg, &std);
		printf("%s;%f;%f\n", "set_permission", avg, std);
	}

    for(i=0; i<cfg.connection_num; i++)
        kill(pid_list[i], SIGUSR1);

    for(i=0; i<cfg.connection_num; i++)
        waitpid(pid_list[i], &child_status, 0);

    free(pid_list);
	free(time_res);
	return ret;
}

void print_usage(int argc, char **argv)
{
	printf("Usage: %s <Config file>\n", argv[0]);
}

void default_config(config *cfg)
{
	cfg->connection_num = 0;
    cfg->xs_read = 0;
	cfg->xs_write = 0;
	cfg->xs_open = 0;
	cfg->xs_close = 0;
	cfg->xs_get_domain_path = 0;
	cfg->xs_set_permission = 0;

	cfg->repeat = 1;
	cfg->req_size = 32;
}

void print_config(const config *cfg)
{
	printf("Configuration:\n");
	printf("--------------\n");
	printf("connection_num: %d\n", cfg->connection_num);
    printf("xs_read: %d\n", cfg->xs_read);
	printf("xs_write: %d\n", cfg->xs_write);
	printf("xs_open: %d\n", cfg->xs_open);
	printf("xs_close: %d\n", cfg->xs_close);
	printf("xs_get_domain_path: %d\n", cfg->xs_get_domain_path);
	printf("xs_set_permission: %d\n", cfg->xs_set_permission);

	printf("repeat: %d\n", cfg->repeat);
	printf("req_size: %d\n", cfg->req_size);
	printf("--------------\n");
}

int parse_config_file(char *path, config *cfg)
{
	FILE *f;
	char *line;
	int read, ret = 0;
	size_t n = 256; /* max number of char to read */

	line = (char *)malloc(n * sizeof(char));

	f = fopen(path, "r");
	if(f == NULL)
	{
		perror("fopen");
		ret = -1;
		goto out_free;
	}

	/* yeah that's ugly */
    while ((read = getline(&line, &n, f)) != -1)
    {
    	if(!strcmp(line, "\n") || !strncmp(line, "#", 1))
    		continue;
		if(!strncmp(line, "xs_read ", strlen("xs_read ")))
			cfg->xs_read = atoi(line + strlen("xs_read "));
		else if(!strncmp(line, "xs_write ", strlen("xs_write ")))
			cfg->xs_write = atoi(line + strlen("xs_write "));
		else if(!strncmp(line, "xs_open ", strlen("xs_open ")))
			cfg->xs_open = atoi(line + strlen("xs_open "));
		else if(!strncmp(line, "xs_close ", strlen("xs_close ")))
			cfg->xs_close = atoi(line + strlen("xs_close "));
		else if(!strncmp(line, "xs_get_domain_path ", strlen("xs_get_domain_path ")))
			cfg->xs_get_domain_path = atoi(line + strlen("xs_get_domain_path "));
		else if(!strncmp(line, "xs_set_permission ", strlen("xs_set_permission ")))
			cfg->xs_set_permission = atoi(line + strlen("xs_set_permission "));
		else if(!strncmp(line, "repeat ", strlen("repeat ")))
			cfg->repeat = atoi(line + strlen("repeat "));
		else if(!strncmp(line, "req_size ", strlen("req_size ")))
			cfg->req_size = atoi(line + strlen("req_size "));
        else if(!strncmp(line, "connection_num ", strlen("connection_num ")))
            cfg->connection_num = atoi(line + strlen("connection_num "));
		else
		{
			fprintf(stderr, "Error: I cannot understand this line in the config file:\n");
			fprintf(stderr, "%s\n", line);
			ret = -2;
			goto out_close;
		}

    }

out_close:
	fclose(f);
out_free:
	free(line);
	return ret;
}

int bench_write(int n, int size, double *result)
{
	struct xs_handle *xs;
	int i, ret = 0, r;
	struct timeval start, stop, res;
	char *data;

	xs = xs_open(0);
	if(xs == NULL)
	{
		perror("xs_open");
		return -1;
	}

	data = (char *)malloc(size * sizeof(char) + 1);
	if(data == NULL)
	{
		perror("malloc");
		goto out_close;
	}
	get_random_string(data, size);

	r = gettimeofday(&start, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -3;
		goto out_free;

	}
	for(i=0; i<n; i++)
	{
		if (xs_write(xs, XBT_NULL, BENCH_PATH, data, strlen(data)) == false)
		{
			perror("xs_write");
			ret = -2;
			goto out_free;
		}
	}
	r = gettimeofday(&stop, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -3;
		goto out_free;

	}

	timersub(&stop, &start, &res);
	*result = 0 + res.tv_sec*1000000 + res.tv_usec;

out_free:
	free(data);
out_close:
	xs_close(xs);
	return ret;
}

int bench_read(int n, int size, double *result)
{
	struct xs_handle *xs;
	int i, ret = 0, r;
	unsigned int len;
	char *buffer, *data;
	struct timeval start, stop, res;

	xs = xs_open(0);
	if(xs == NULL)
	{
		perror("xs_open");
		return -1;
	}

	data = (char *)malloc(size * sizeof(char) + 1);
	if(data == NULL)
	{
		perror("malloc");
		goto out_close;
	}
	get_random_string(data, size);

	/* first write what we will read later */
	if (xs_write(xs, XBT_NULL, BENCH_PATH, data, strlen(data)) == false)
	{
		perror("xs_write");
		ret = -2;
		goto out_free;
	}

	r = gettimeofday(&start, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -3;
		goto out_free;

	}
	for(i=0; i<n; i++)
	{
		buffer = xs_read(xs, XBT_NULL, BENCH_PATH, &len);
		if(buffer == NULL)
		{
			perror("xs_read");
			ret = -4;
			goto out_free;
		}
		free(buffer);
	}
	r = gettimeofday(&stop, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -3;
		goto out_free;

	}

	timersub(&stop, &start, &res);
	*result = 0 + res.tv_sec*1000000 + res.tv_usec;

out_free:
	free(data);
out_close:
	xs_close(xs);
	return ret;
}

int bench_close(int n, double *result)
{
	struct xs_handle **xs;
	int r, actually_openned = 0, ret = 0, i;
	struct timeval start, stop, res;

	xs = (struct xs_handle **)malloc(n * sizeof(struct xs_handle *));

	for(i=0; i<n; i++)
	{
		xs[i] = xs_open(0);
		if(xs[i] == NULL)
		  {
			  perror("xs_open\n");
			  ret = -1;
			  goto out_close;
		  }
		actually_openned++;
	}

out_close:
	r = gettimeofday(&start, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -2;
		goto out_free;

	}

	for(i=0; i<actually_openned; i++)
		xs_close(xs[i]);

	r = gettimeofday(&stop, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -2;
		goto out_free;

	}

	timersub(&stop, &start, &res);
	*result = 0 + res.tv_sec*1000000 + res.tv_usec;

out_free:
	free(xs);

	return ret;
}

int bench_open(int n, double *result)
{
	int i, ret = 0, actually_openned = 0, r;
	struct xs_handle **xs;
	struct timeval start, stop, res;

	xs = (struct xs_handle **)malloc(n * sizeof(struct xs_handle *));

	r = gettimeofday(&start, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -1;
		goto out_free;

	}
	for(i=0; i<n; i++)
	{
		xs[i] = xs_open(0);
		if(xs[i] == NULL)
		  {
			  perror("xs_open\n");
			  ret = -2;
			  goto out_close;
		  }
		actually_openned++;
	}
	r = gettimeofday(&stop, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -1;
		goto out_free;

	}

	timersub(&stop, &start, &res);
	*result = 0 + res.tv_sec*1000000 + res.tv_usec;

out_close:
	for(i=0; i<actually_openned; i++)
		xs_close(xs[i]);

out_free:
	free(xs);
	return ret;
}

int bench_set_permission(int n, double *result)
{
	int ret = 0, i, r;
	struct timeval start, stop, res;
	struct xs_handle *xs;
	struct xs_permissions perms;
	bool perm_ret;

		xs = xs_open(0);
		if(xs == NULL)
		{
			perror("xs_open");
			return -1;
		}

		perms.id = 0;
		perms.perms = XS_PERM_READ;

		r = gettimeofday(&start, NULL);
		if(r == -1)
		{
			perror("gettimeofday");
			ret = -3;
			goto out_close;

		}

		for(i=0; i<n; i++)
		{
			perm_ret = xs_set_permissions(xs, XBT_NULL, BENCH_PATH, &perms, 1);
			if(perm_ret == false)
			{
				perror("xs_set_permissions");
				ret = -4;
				goto out_close;
			}
		}

		r = gettimeofday(&stop, NULL);
		if(r == -1)
		{
			perror("gettimeofday");
			ret = -3;
			goto out_close;

		}

		timersub(&stop, &start, &res);
		*result = 0 + res.tv_sec*1000000 + res.tv_usec;

	out_close:
		xs_close(xs);
		return ret;
}

int bench_get_domain_path(int n, double *result)
{
	int ret = 0, i, r;
	struct timeval start, stop, res;
	struct xs_handle *xs;
	char *path;

	xs = xs_open(0);
	if(xs == NULL)
	{
		perror("xs_open");
		return -1;
	}

	r = gettimeofday(&start, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -3;
		goto out_close;

	}

	for(i=0; i<n; i++)
	{
		path = xs_get_domain_path(xs, 0);
		if(path == NULL)
		{
			perror("xs_get_domain_path");
			ret = -4;
			goto out_close;
		}
		free(path);
	}

	r = gettimeofday(&stop, NULL);
	if(r == -1)
	{
		perror("gettimeofday");
		ret = -3;
		goto out_close;

	}

	timersub(&stop, &start, &res);
	*result = 0 + res.tv_sec*1000000 + res.tv_usec;

out_close:
	xs_close(xs);
	return ret;
}

void avg_stdev(const double *array, int size, double *mean, double *std)
{
	int i;
	double sum;

	sum = 0;
	for(i=0; i<size; i++)
		sum += array[i];

	*mean = sum/(double)size;

	sum = 0;
	for(i=0; i<size; i++)
		sum += (array[i] - *mean) * (array[i] - *mean);

	*std = sqrt(sum / (double)size);

}

/* Build a random string
 * Stolen from
 * http://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
 */
void get_random_string(char *s, const int len)
{
    int i;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (i = 0; i < len; ++i)
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

    s[len] = 0;
}

int spawn_connections(int num)
{
    int i, ret;
    pid_t pid;

    for(i=0; i<num; i++)
    {
        pid = fork();

        switch(pid)
        {
            case -1:
                perror("fork");
                ret = -1;
                goto out;
                break;

            case 0:
                connection_loop(i);
                break;

            default:
                pid_list[i] = pid;
                break;
        }
    }

    ret = 0;
out:
    return ret;
}

void connection_loop(int child_id)
{
    int should_stop;
    struct xs_handle *xs;
    
    signal(SIGUSR1, child_exit_signal);
    
    xs = xs_open(0);
    if(xs == NULL)
    {
        perror("xs_open");
        exit(EXIT_FAILURE);
    }
    
    while(1)
    {
        should_stop = child_stop;    
        if(should_stop)
            break;

        sleep(1);
    }

    xs_close(xs);

    exit(EXIT_SUCCESS);
}

void child_exit_signal(int signum)
{
    if(signum == SIGUSR1)
        child_stop = 1;
}
