#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include <assert.h>

#include "kmer_utils.h"
#include "quikr.h"


/* getdelim.c --- Implementation of replacement getdelim function.
	 Copyright (C) 1994, 1996, 1997, 1998, 2001, 2003, 2005 Free
	 Software Foundation, Inc.

	 This program is free software; you can redistribute it and/or
	 modify it under the terms of the GNU General Public License as
	 published by the Free Software Foundation; either version 2, or (at
	 your option) any later version.

	 This program is distributed in the hope that it will be useful, but
	 WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	 General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with this program; if not, write to the Free Software
	 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
	 02110-1301, USA.  */

ssize_t getseq(char **lineptr, size_t *n, FILE *fp) {
	int result = 0;
	ssize_t cur_len = 0;

	if (lineptr == NULL || n == NULL || fp == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	flockfile (fp);

	if (*lineptr == NULL || *n == 0)
	{
		*n = 120;
		*lineptr = (char *) malloc (*n);
		if (*lineptr == NULL)
		{
			result = -1;
			goto unlock_return;
		}
	}

		int newline = 0;
	for (;;) {
		int i;

		i = getc (fp);
		if (i == EOF)
		{
			result = -1;
			break;
		}

		/* Make enough space for len+1 (for final NUL) bytes.  */
		if (cur_len + 2 >= *n)
		{
			size_t needed = 2 * (cur_len + 1) + 2;   /* Be generous. */
			char *new_lineptr;

			if (needed < cur_len)
			{
				result = -1;
				goto unlock_return;
			}

			new_lineptr = (char *) realloc (*lineptr, needed);
			if (new_lineptr == NULL){
				result = -1;
				goto unlock_return;
			}

			*lineptr = new_lineptr;
			*n = needed;
		}

		(*lineptr)[cur_len] = i;
		cur_len++;

		if (i == '\n') {
			newline = 1;
		}

		if(i == '>' && newline == 1) {
			break;
		}
	}
	(*lineptr)[cur_len] = '\0';
	result = cur_len ? cur_len : result;

	unlock_return:
		funlockfile (fp);
		return result;
}

void check_malloc(void *ptr, char *error) {
	if (ptr == NULL) {
		if(error != NULL)  {
			fprintf(stderr,"Error: %s\n", error);
		}
		else { fprintf(stderr, "Error: Could not allocate enough memory - %s\n", strerror(errno));
		}
		exit(EXIT_FAILURE);
	}
}
static int double_cmp (const void * a, const void * b) {
	return ( *(double*)a - *(double*)b );
}

void get_rare_value(double *count_matrix, unsigned long long width, double rare_percent, unsigned long long *ret_rare_value, unsigned long long  *ret_rare_width) { 
	size_t y, x;
	unsigned long long rare_width = 0;
	double rare_value = 0;

	// allocate * sort a temporary matrix
	double *sorted_count_matrix = malloc(width * sizeof(double));
	check_malloc(sorted_count_matrix, NULL);

	memcpy(sorted_count_matrix, count_matrix, width * sizeof(double));

	qsort(sorted_count_matrix, width, sizeof(double), double_cmp);

	// get our "rare" counts
	for(y = 0; y < width; y++) {
		double percentage = 0;

		rare_value = sorted_count_matrix[y];
		rare_width = 0;
		for(x = 0; x < width; x++) {
			if(count_matrix[x] <= rare_value) {
				rare_width++;
			}
		}
		percentage = (double)rare_width / (double)width;

		if(percentage >= rare_percent)
			break;
	}

	free(sorted_count_matrix);
	*ret_rare_width = rare_width;
	*ret_rare_value = rare_value;
}

void debug_arrays(double *count_matrix, struct matrix *sensing_matrix) {
	FILE *count_fh = fopen("count.mat", "w");
	FILE *sensing_fh = fopen("sensing.mat", "w");

	unsigned long long width = pow_four(sensing_matrix->kmer);
	unsigned long long i = 0;
	unsigned long long j = 0;

	for(i = 0; i < sensing_matrix->sequences; i++) {
		for(j = 0; j < width - 1; j++)
			fprintf(sensing_fh, "%lf\t", sensing_matrix->matrix[width*i + j]);
		fprintf(sensing_fh, "%lf\n", sensing_matrix->matrix[width*i + width-1]);
	}

	fclose(sensing_fh);

	for(j = 0; j < width - 1; j++)
		fprintf(count_fh, "%lf\t", count_matrix[j]);
	fprintf(count_fh, "%lf\n", count_matrix[width - 1]);

	fclose(count_fh);
}


void normalize_matrix(double *matrix, unsigned long long height, unsigned long long width) {
	unsigned long long x = 0;
	unsigned long long y = 0;

	for(x = 0; x < height; x++) {

		double row_sum = 0;

		for(y = 0; y < (width); y++)
			row_sum = row_sum + matrix[width * x + y];
		for(y = 0; y < (width); y++)
			matrix[width * x + y] = matrix[width * x + y] / row_sum;
	}
}

unsigned long long count_sequences(const char *filename) {
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	unsigned long long sequences = 0;

	FILE *fh = fopen(filename, "r");
	if(fh == NULL) {
		fprintf(stderr, "could not open \"%s\"\n", filename );
		return 0;
	}

	while ((read = getline(&line, &len, fh)) != -1) {
		if(line[0] == '>')
			sequences++;
	}

	free(line);
	fclose(fh);

	return sequences;
}


/*
 * * Sally - A Tool for Embedding Strings in Vector Spaces
 * * Copyright (C) 2010 Konrad Rieck (konrad@mlsec.org)
 * * --
 * * This program is free software; you can redistribute it and/or modify it
 * * under the terms of the GNU General Public License as published by the
 * * Free Software Foundation; either version 3 of the License, or (at your
 * * option) any later version. This program is distributed without any
 * * warranty. See the GNU General Public License for more details.
 * */
size_t gzgetline(char **s, size_t * n, gzFile f) {
	assert(f);
	int c = 0;
	*n = 0;

	if (gzeof(f))
		return -1;

	while (c != '\n') {
		if (!*s || *n % 256 == 0) {
			*s = realloc(*s, *n + 256 + 1);
			if (!*s)
				return -1;
		}

		c = gzgetc(f);
		if (c == -1)
			return -1;

		(*s)[(*n)++] = c;
	}

	(*s)[*n] = 0;
	return *n;
}

struct matrix *load_sensing_matrix(const char *filename, unsigned int target_kmer) {

	char *line = NULL;
	char **headers = NULL;

	double *matrix = NULL;

	unsigned int kmer = 0;
	
	unsigned long long i = 0;
	unsigned long long *row = NULL;
	unsigned long long sequences = 0;
	unsigned long long width = 0;

	struct matrix *ret = NULL;
	size_t lineno = 0;

	gzFile fh = NULL;

	fh = gzopen(filename, "r");
	if(fh == NULL) {
		fprintf(stderr, "could not open %s", filename);
		exit(EXIT_FAILURE);
	}

	line = malloc(1024 * sizeof(char));
	check_malloc(line, NULL);

	// Check for quikr
	line = gzgets(fh, line, 1024);
	lineno++;
	if(strcmp(line, "quikr\n") != 0) {
		fprintf(stderr, "This does not look like a quikr sensing matrix. Please check your path: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	// check version
	line = gzgets(fh, line, 1024);
	if(atoi(line) != MATRIX_REVISION) {
		fprintf(stderr, "Sensing Matrix uses an unsupported version, please retrain your matrix\n");
		exit(EXIT_FAILURE);
	}
	lineno++;

	// get number of sequences
	line = gzgets(fh, line, 1024);
	sequences = strtoull(line, NULL, 10);
	if(sequences == 0) {
		fprintf(stderr, "Error parsing sensing matrix, sequence count is zero\n");
		exit(EXIT_FAILURE);
	}
	lineno++;

	// get kmer
	gzgets(fh, line, 1024);
	kmer = atoi(line);
	if(kmer == 0) {
		fprintf(stderr, "Error parsing sensing matrix, kmer is zero\n");
		exit(EXIT_FAILURE);
	}
	lineno++;

	// if passed kmer in is zero, just use whatever the matrix is trained as.
	if(target_kmer != 0 && kmer != target_kmer ) {
			fprintf(stderr, "The sensing_matrix was trained with a different kmer than your requested kmer\n");
			exit(EXIT_FAILURE);
	}

	width = pow_four(kmer);

	// allocate a +1 size for the extra row
	matrix = malloc(sequences * (width) * sizeof(double));
	check_malloc(matrix, NULL);

	row = malloc((width) * sizeof(unsigned long long));
	check_malloc(row, NULL);
	
	headers = malloc(sequences * sizeof(char *));
	check_malloc(headers, NULL);

	char *buf = NULL;
	size_t len = 0;
	size_t read = 0;
	for(i = 0; i < sequences; i++) {
		unsigned long long j = 0;
		// get header and add it to headers array
		//
		read = gzgetline(&buf, &len, fh);
		if(read == 0)  {
			fprintf(stderr, "Error parsing sensing matrix, could not read header\n");
			fprintf(stderr, "%s\n", buf);
			exit(EXIT_FAILURE);
		}

		char *header = malloc(sizeof(char) * read + 1);
		check_malloc(header, NULL);	
		header = strncpy(header, buf, read - 1);
		if(header[0] != '>') {
			fprintf(stderr, "Error parsing sensing matrix, could not read header in line %llu\n", lineno);
			exit(EXIT_FAILURE);
		}
		lineno++;

		header[read - 1] = '\0';
		headers[i] = header+1;

		row = memset(row, 0, (width) * sizeof(unsigned long long));

		for(j = 0; j < width; j++) {
			line = gzgets(fh, line, 32);
			lineno++;
			if(line == NULL || line[0] == '>') {
				fprintf(stderr, "Error parsing sensing matrix, line %zu does not look like a value\n", lineno);
				exit(EXIT_FAILURE);
			}
			lineno++;

			row[j] = strtoull(line, NULL, 10);
			if(errno) {
				printf("could not parse '%s'\n into a number", line);
				exit(EXIT_FAILURE);
			}

		}
		for(j = 0; j < width; j++) {
			matrix[i*(width) + j] = ((double)row[j]);
		}
	}

	// load the matrix of counts
	gzclose(fh);

	free(line);
	free(row);
	ret = malloc(sizeof(struct matrix));
	(*ret).kmer = kmer;
	(*ret).sequences = sequences;
	(*ret).matrix = matrix;
	(*ret).headers = headers;

	return ret;
}
