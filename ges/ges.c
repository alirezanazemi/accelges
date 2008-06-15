/*
 * Copyright (C) 2008 by OpenMoko, Inc.
 * Written by Paul-Valentin Borza <gestures@borza.ro>
 * All Rights Reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

//#include <float.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "class.h"
#include "gauss.h"
#include "ges.h"
#include "hmm.h"
#include "model.h"

/* parse one line in the configuration file */
static unsigned char parse_line(struct config_t *config, char *line);
/* recognize frames */
static void recognize(struct ges_3d_t *ges, struct accel_3d_t accel[], unsigned int accel_len);

/*
 * create feature vector
 */
unsigned char ges_fea_3d(struct seq_3d_t *seq, unsigned int seq_index, unsigned int seq_prev_index, struct sample_3d_t *sample)
{
	if ((!seq) || (!sample))
	{
		return 0;
	}
	
	if (seq_index > 0)
	{		
		/* magnitude at time indicated by index */
		sample->val[0] = sqrt(
			pow(seq->each[seq_index].val[0], 2) +
			pow(seq->each[seq_index].val[1], 2) +
			pow(seq->each[seq_index].val[2], 2));
		/* magnitude at time indicated by prev_index */
		sample->val[1] = sqrt(
			pow(seq->each[seq_prev_index].val[0], 2) +
			pow(seq->each[seq_prev_index].val[1], 2) +
			pow(seq->each[seq_prev_index].val[2], 2));
		/* delta magnitude between time t and t - 1 */
		sample->val[2] = sqrt(
			pow(seq->each[seq_index].val[0] - seq->each[seq_prev_index].val[0], 2) +
			pow(seq->each[seq_index].val[1] - seq->each[seq_prev_index].val[1], 2) +
			pow(seq->each[seq_index].val[2] - seq->each[seq_prev_index].val[2], 2));		
		
		return 1;
	}
	else
	{
		sample->val[0] = 0.0;
		sample->val[1] = 0.0;
		sample->val[2] = 0.0;
		
		return 0;
	}
}

/*
 * process accelerometer values
 */
void ges_process_3d(struct ges_3d_t *ges, struct accel_3d_t accel)
{
	/* increment and save current frame (uses a circular list) */
	unsigned int prev_index = ges->seq.index;
	ges->seq.index = (ges->seq.index + 1) % FRAME_LEN;
	ges->seq.each[ges->seq.index] = accel;
	
	/* extract feature vector */
	sample_3d_t feature;
	ges_fea_3d(&ges->seq, ges->seq.index, prev_index, &feature);
	
	//printf("%f\t%f\t%f\n", feature.val[0], feature.val[1], feature.val[2]);
	
	/* motion has index 1 and noise has index 0 */
	if (class_max_2c(&ges->endpoint, feature) == 1)
	{
		/* we want motion */
		if (!ges->detected)
		{
			ges->seq.begin = ges->seq.index;
			ges->detected = 1;
		}
	}
	else
	{
		/* noise detected */
		if (ges->detected)
		{
			ges->seq.end = ges->seq.index;
			ges->detected = 0;
			//printf("detected with size: %d\n", ges->seq.end - ges->seq.begin); 
			/* case when begin < end */
			if (ges->seq.begin < ges->seq.end)
			{
				if (ges->seq.end - ges->seq.begin > FRAME_DIF)
				{
					int before = 0;
					if (ges->seq.begin > FRAME_BEFORE)
					{
						before = FRAME_BEFORE;
					}
					
					unsigned int frame_len = ges->seq.end - ges->seq.begin + 1;
					struct accel_3d_t accels[before + frame_len];
					memcpy(&accels[before], &ges->seq.each[ges->seq.begin], frame_len * sizeof(sample_3d_t));
					
					if (before > 0)
					{
						memcpy(&accels[0], &ges->seq.each[ges->seq.begin - before], before * sizeof(sample_3d_t));
					}
					recognize(ges, accels, before + frame_len);
				}
			}
			else /* case when begin > end */
			{
				if (FRAME_LEN - ges->seq.begin + ges->seq.end > FRAME_DIF)
				{
					int before = 0;
					if (ges->seq.begin - ges->seq.end > FRAME_BEFORE)
					{
						before = FRAME_BEFORE;
					}
					unsigned int frame_len_end = FRAME_LEN - ges->seq.begin;
					unsigned int frame_len_begin = ges->seq.end + 1;
					unsigned int frame_len = frame_len_end + frame_len_begin;
					struct accel_3d_t accels[before + frame_len];
					memcpy(&accels[before], &ges->seq.each[ges->seq.begin], frame_len_end * sizeof(sample_3d_t));
					memcpy(&accels[before + frame_len_end], &ges->seq.each[0], frame_len_begin * sizeof(accel_3d_t));
					
					if (before > 0)
					{
						memcpy(&accels[0], &ges->seq.each[ges->seq.begin - before], before * sizeof(sample_3d_t));
					}
					
					recognize(ges, accels, before + frame_len); 
				}
			}
		}
	}
}

/* 
 * allocate memory for class mixtures 
 */
void ges_create_3d(struct ges_3d_t *ges)
{
	ges->seq.index = 0;
	ges->detected = 0;
	/* do not create here, will be created during reading from files */
	//gauss_mix_create_3d(&ges->endpoint.each[0], 1);
	//gauss_mix_create_3d(&ges->endpoint.each[1], 1);
	/* TO-DO: write these values to files */
	ges->endpoint.prior_prob[0] = 0.4; /* noise */
	ges->endpoint.prior_prob[1] = 0.6; /* motion */
}

/* 
 * de-allocate memory for class mixtures 
 */
void ges_delete_3d(struct ges_3d_t *ges)
{
	gauss_mix_delete_3d(&ges->endpoint.each[0]);
	gauss_mix_delete_3d(&ges->endpoint.each[1]);
}

/* 
 * read ges structure from files
 */
void ges_read_3d(struct ges_3d_t *ges, struct config_t *config)
{
	gauss_mix_read_3d(&ges->endpoint.each[0], config->noise_file_name);	
	//gauss_mix_print_3d(&ges->endpoint.each[0]);
	gauss_mix_read_3d(&ges->endpoint.each[1], config->motion_file_name);
	//gauss_mix_print_3d(&ges->endpoint.each[1]);
	
	ges->model_len = config->model_len;
	int i;
	for (i = 0; i < ges->model_len; i++)
	{
		hmm_read_3d(&ges->model[i], config->model_file_name[i]);
		strcpy(ges->model_cmd[i], config->model_cmd[i]);
		printf("%s\n", ges->model_cmd[i]); 
	}
}

/* 
 * write ges structure to file
 */
void ges_write_3d(struct ges_3d_t *ges, struct config_t *config)
{
	gauss_mix_write_3d(&ges->endpoint.each[0], config->noise_file_name);
	gauss_mix_write_3d(&ges->endpoint.each[1], config->motion_file_name);
}

/*
 * load configuration file
 */
unsigned char ges_load_config(struct config_t *config, char *file_name)
{
	if (!file_name)
	{
		return 0;
	}
	/* change working directory to be able to read files from same dir as config file */
	char path_copy[1024];
	strncpy(path_copy, file_name, sizeof(path_copy));
	path_copy[1023] = '\0';
	/* pass a copy because dirname will modify it */
	chdir(dirname(path_copy));
	
	FILE *file;
	char line[1024];
	unsigned int line_index;
	
	file = fopen(basename(file_name), "r");
	if (!file)
	{
		perror("fopen");
		return 0;
	}
	
	config->model_len = 0;
	
	line_index = 0;
	while (fgets(line, sizeof(line), file))
	{
		line_index++;
		if (!parse_line(config, line))
		{
			fprintf(stderr, "Unknown syntax in configuration file at line %d\n", line_index);
			return 0;
		}
	}
	
	fclose(file);
	
	return 1;
}

/* 
 * isolated recognition in continuous mode
 * only one gesture is detected in the classified frames
 */
static void recognize(struct ges_3d_t *ges, struct accel_3d_t accel[], unsigned int accel_len)
{
	//printf("Processing gesture... (TO-DO: gesture recognition)\n");
	int i;
	/*
	for (i = 0; i < accel_len; i++)
	{
		printf("%d:\t%+f\t%+f\t%+f\n", i, accel[i].val[0], accel[i].val[1], accel[i].val[2]);
	}
	// */
	//float vals[ges->model_len];
	unsigned char max_reached_final_state = 0;
	float max = hmm_viterbi(&ges->model[0], accel, accel_len, &max_reached_final_state);
	//float max = hmm_forward(&ges->model[0], accel, accel_len);
	//float prev_max = max;
	
	int argmax = 0;
	printf("Recognition results:\n");
	for (i = 0; i < ges->model_len; i++)
	{
		//float possib_max = hmm_forward(&ges->model[i], accel, accel_len);
		unsigned char possib_max_reached_final_state = 0; 
		float possib_max = hmm_viterbi(&ges->model[i], accel, accel_len, &possib_max_reached_final_state);

		//vals[i] = possib_max;
		if (!max_reached_final_state)
		{
			max = possib_max;
			max_reached_final_state = possib_max_reached_final_state;
			argmax = i;
		}
		printf("%d: %e\n", i, possib_max);
		if (possib_max_reached_final_state)
		{ 
			if (max < possib_max)
			{
				max = possib_max;
				max_reached_final_state = possib_max_reached_final_state;
				argmax = i;
			}
		}
	}
	
	/*prev_max = -1.0e+07;
	for (i = 0; i < ges->model_len; i++)
	{
		if (i != argmax)
		{
			if (prev_max < vals[i])
			{
				prev_max = vals[i];
			}
		}
	}*/
	
	//printf("prev_max: %e\n", prev_max);
	
	if (max_reached_final_state)
	{
		/* call handler once the recognition is done */
		ges->handle_reco(ges->model_cmd[argmax]);
	}
	else
	{
		printf("Sorry, didn't get that...\n");
	}
	
	
	//struct gauss_mix_3d_t gauss_mix_est;
	//gauss_mix_create_3d(&gauss_mix_est, 1);
	//gauss_mix_den_est_3d(&ges->endpoint.motion, &gauss_mix_est, accel, accel_len);
	//gauss_mix_print_3d(&gauss_mix_est);
	//memcpy(&ges->endpoint.motion, &gauss_mix_est, sizeof(struct gauss_mix_3d_t));
}

/* 
 * parse one line in the configuration file
 */
static unsigned char parse_line(struct config_t *config, char *line)
{
	if ((!config) || (!line))
	{
		return 0;
	}
		
	/* comment */
	if ((line[0] == '#') || (line[0] == ';'))
	{
		return 1;
	}
	else
	{
		char cmd_name[1024];
		unsigned int cmd_len = strcspn(line, "\t\n") / sizeof(char);
		strncpy(cmd_name, line, cmd_len * sizeof(char));
		cmd_name[cmd_len] = '\0';
		/* no command on this line */
		if (cmd_name[0] == '\0')
		{
			return 1; /* return success */
		}
		
		if (strcmp(cmd_name, "noise") == 0)
		{
			unsigned int no_param_len = strspn(&line[cmd_len], "\t\n") / sizeof(char);
			char *params = &line[cmd_len + no_param_len];
			char param_name[1024];
			unsigned int param_len = strcspn(params, "\t\n") / sizeof(char);
			strncpy(param_name, params, param_len);
			param_name[param_len] = '\0';
			
			if (param_name[0] == '\0')
			{
				return 0; /* must have param, so return failure */
			}
			
			strcpy(config->noise_file_name, param_name);
		}
		else if (strcmp(cmd_name, "motion") == 0)
		{
			unsigned int no_param_len = strspn(&line[cmd_len], "\t\n") / sizeof(char);
			char *params = &line[cmd_len + no_param_len];
			char param_name[1024];
			unsigned int param_len = strcspn(params, "\t\n") / sizeof(char);
			strncpy(param_name, params, param_len);
			param_name[param_len] = '\0';
			
			if (param_name[0] == '\0')
			{
				return 0; /* same as above */
			}
			
			strcpy(config->motion_file_name, param_name);	
		}
		else if (strncmp(line, "hmm", cmd_len) == 0)
		{
			// TODO: parse hmm line
			unsigned int no_param_len = strspn(&line[cmd_len], "\t\n") / sizeof(char);
			char *params = &line[cmd_len + no_param_len];
			char param_name_1[1024];
			unsigned int param_len_1 = strcspn(params, "\t\n") / sizeof(char);
			strncpy(param_name_1, params, param_len_1);
			// BUG HERE and also below
			param_name_1[param_len_1] = '\0';
			
			if (param_name_1[0] == '\0')
			{
				return 0; /* must have param, so return failure */
			}
			
			unsigned int no_param_len_2 = strspn(&line[cmd_len + no_param_len + param_len_1], " \t\n") / sizeof(char);
			char *params_2 = &line[cmd_len + no_param_len + param_len_1 + no_param_len_2];
			char param_name_2[1024];
			unsigned int param_len_2 = strcspn(params_2, "\t\n") / sizeof(char);
			// check length of param_len_2 with size of param_name_2!!!
			strncpy(param_name_2, params_2, param_len_2);
			param_name_2[param_len_2] = '\0';
			
			
			/* model command that is returned when detected */
			strcpy(config->model_cmd[config->model_len], param_name_1);
			/* model file name to read data from */
			strcpy(config->model_file_name[config->model_len], param_name_2);

			//printf("arg %s and %s.\n", param_name_1, param_name_2);
			config->model_len++;
		}
		else
		{
			return 0;
		}
	}
	// printf("%s", line);
	
	return 1;
}

/* 
 * populate ges structure with manual values
 */
void ges_populate_3d(struct ges_3d_t *ges)
{	
	/* noise */
	gauss_mix_create_3d(&ges->endpoint.each[0], 1);
	ges->endpoint.prior_prob[0] = 0.4;
	ges->endpoint.each[0].weight[0] = 1;
	ges->endpoint.each[0].each[0].mean[0] = 1.0;
	ges->endpoint.each[0].each[0].mean[1] = 1.0;
	ges->endpoint.each[0].each[0].mean[2] = 0.0;
	ges->endpoint.each[0].each[0].covar[0][0] = 0.07;
	ges->endpoint.each[0].each[0].covar[0][1] = 0.0;
	ges->endpoint.each[0].each[0].covar[0][2] = 0.0;
	ges->endpoint.each[0].each[0].covar[1][0] = 0.0;
	ges->endpoint.each[0].each[0].covar[1][1] = 0.07;
	ges->endpoint.each[0].each[0].covar[1][2] = 0.0;
	ges->endpoint.each[0].each[0].covar[2][0] = 0.0;
	ges->endpoint.each[0].each[0].covar[2][1] = 0.0;
	ges->endpoint.each[0].each[0].covar[2][2] = 0.07;
	
	/* motion */
	gauss_mix_create_3d(&ges->endpoint.each[1], 1);
	ges->endpoint.prior_prob[1] = 0.6;
	ges->endpoint.each[1].weight[0] = 1;
	ges->endpoint.each[1].each[0].mean[0] = 2.4;
	ges->endpoint.each[1].each[0].mean[1] = 2.0;
	ges->endpoint.each[1].each[0].mean[2] = 0.7;
	ges->endpoint.each[1].each[0].covar[0][0] = 0.4;
	ges->endpoint.each[1].each[0].covar[0][1] = 0.0;
	ges->endpoint.each[1].each[0].covar[0][2] = 0.0;
	ges->endpoint.each[1].each[0].covar[1][0] = 0.0;
	ges->endpoint.each[1].each[0].covar[1][1] = 0.4;
	ges->endpoint.each[1].each[0].covar[1][2] = 0.0;
	ges->endpoint.each[1].each[0].covar[2][0] = 0.0;
	ges->endpoint.each[1].each[0].covar[2][1] = 0.0;
	ges->endpoint.each[1].each[0].covar[2][2] = 0.4;
}