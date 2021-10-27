/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

#define perror_pthread(ret, msg) \
        do { errno = ret; perror(msg); } while (0)

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

struct thread_info_struct {
        pthread_t tid; /* POSIX thread id, as returned by the library */

        int ** arr; /* Pointer to array to manipulate */

        int thrid; /* Application-defined thread id */
        int thrcnt;
};

int safe_atoi(char *s, int *val)
{
        long l;
        char *endp;

        l = strtol(s, &endp, 10);
        if (s != endp && *endp == '\0') {
                *val = l;
                return 0;
        } else
                return -1;
}

void *safe_malloc(size_t size)
{
        void *p;

        if ((p = malloc(size)) == NULL) {
                fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
                        size);
                exit(1);
        }

        return p;
}

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;

	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void compute_and_output_mandel_line(int fd, int line)
{
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];

	compute_mandel_line(line, color_val);
	output_mandel_line(fd, color_val);
}

sem_t sema[50];

void* thread_start_fn(void* arg){ //se kathe nima dinetai mazi kai to struct me tis plirofories gia auto to nima
	struct thread_info_struct *thr = arg; 

	int row_index; //arxiki grammi pou tha ypologisei auto to nima
	for (row_index = thr->thrid; row_index < y_chars; row_index += thr->thrcnt) 
		compute_mandel_line(row_index, thr->arr[row_index]);
        //Ypologizei tis grammes pou tou antistoixoun prostetontas kathe fora n gia na brei tin epomeni grammi

    for (row_index = thr->thrid; row_index < y_chars; row_index += thr->thrcnt){
        sem_wait(&sema[row_index]); //Perimenei ton simaforo tis antistoixis grammis pou thelei na typosei
        output_mandel_line(1, thr->arr[row_index]);  //Typonei tin antistoixi grammi
        sem_post(&sema[row_index+1]);   //Kanei 1 ton epomeno simaforo oste na boresei i epomeni grammi na typothei
        sem_post(&sema[row_index]);     //Apodesmeuei to diko tou simaforo

    }

    return NULL;
}

int main(int argc, char ** argv)
{
	if(argc !=  2){
		printf("wrong usage: needs one int arguement:: the number of threads\n");
		return 1;
	}

	int nthreads;
	if (safe_atoi(argv[1], &nthreads) < 0 || nthreads <= 0) {
        fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
        exit(1);
    }

    // int color_table[y_chars][x_chars];

    int ** color_table = safe_malloc(y_chars * sizeof(int *));
    int i;
    for(i = 0; i < y_chars; i++)
    	color_table[i] = safe_malloc(x_chars * sizeof(int));

    xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

    sem_init(&sema[0], 0 ,1); // Orismos kai arxikopoisi simaforou protis grammis simaforou sto 1
    for(i = 1; i < y_chars; i++){ //Orismos kai arxikopoiisi ton ypoloipon 49 simaforon sto 0
        sem_init(&sema[i], 0 ,0);
    }

    struct thread_info_struct *thr;
    thr = safe_malloc(nthreads * sizeof(*thr));
	int ret;
    for (i = 0; i < nthreads; i++) {
            /* Initialize per-thread structure */
            thr[i].arr = color_table; //tous pernaei diekti pros to colour table
            thr[i].thrid = i;         //o arithmos tou nimatos pou orizei o xristis
            thr[i].thrcnt = nthreads;  //synolikos arithmos nimaton pou dothike apo to xristi

            /* Spawn new thread */
            ret = pthread_create(&(thr[i].tid), NULL, thread_start_fn, &thr[i]); //dimiourgei ena kainourgio nima to opoio ektelei ti thread_start
            if (ret) {
                    perror_pthread(ret, "pthread_create");
                    exit(1);
            }
    }

    /*
     * Wait for all threads to terminate
     */
    for (i = 0; i < nthreads; i++) {
            ret = pthread_join(thr[i].tid, NULL);
            if (ret) {
            		printf("ret on pthread_join kicks\n");
                    perror_pthread(ret, "pthread_join");
                    exit(1);
            }
    }

    for(i = 0; i < y_chars; i++){ //Katastrofi simaforon
        sem_destroy(&sema[i]);
    }

	reset_xterm_color(1);

	return 0;
}
