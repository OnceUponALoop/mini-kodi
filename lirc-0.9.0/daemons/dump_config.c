/*      $Id: dump_config.c,v 5.24 2009/09/19 06:41:56 lirc Exp $      */

/****************************************************************************
 ** dump_config.c ***********************************************************
 ****************************************************************************
 *
 * dump_config.c - dumps data structures into file
 *
 * Copyright (C) 1998 Pablo d'Angelo <pablo@ag-trek.allgaeu.org>
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "dump_config.h"
#include "config_file.h"

void fprint_comment(FILE * f, struct ir_remote *rem)
{
	time_t timet;
	struct tm *tmp;

	timet = time(NULL);
	tmp = localtime(&timet);
	fprintf(f,
		"#\n"
		"# this config file was automatically generated\n"
		"# using lirc-%s(%s) on %s" "#\n" "# contributed by \n" "#\n" "# brand:                       %s\n"
		"# model no. of remote control: \n" "# devices being controlled by this remote:\n" "#\n\n", VERSION,
		hw.name, asctime(tmp), rem->name);
}

void fprint_flags(FILE * f, int flags)
{
	int i;
	int begin = 0;

	for (i = 0; all_flags[i].flag; i++) {
		if (flags & all_flags[i].flag) {
			flags &= (~all_flags[i].flag);
			if (begin == 0)
				fprintf(f, "  flags ");
			else if (begin == 1)
				fprintf(f, "|");
			fprintf(f, "%s", all_flags[i].name);
			begin = 1;
		}
	}
	if (begin == 1)
		fprintf(f, "\n");
}

void fprint_remotes(FILE * f, struct ir_remote *all)
{

	while (all) {
		fprint_remote(f, all);
		fprintf(f, "\n\n");
		all = all->next;
	}
}

void fprint_remote_gap(FILE * f, struct ir_remote *rem)
{
	if (rem->gap2 != 0) {
		fprintf(f, "  gap          %u %u\n", (__u32) rem->gap, (__u32) rem->gap2);
	} else {
		fprintf(f, "  gap          %u\n", (__u32) rem->gap);
	}
}

void fprint_remote_head(FILE * f, struct ir_remote *rem)
{
	fprintf(f, "begin remote\n\n");
	fprintf(f, "  name  %s\n", rem->name);
	if (!is_raw(rem)) {
		fprintf(f, "  bits        %5d\n", rem->bits);
	}
	fprint_flags(f, rem->flags);
	fprintf(f, "  eps         %5d\n", rem->eps);
	fprintf(f, "  aeps        %5d\n\n", rem->aeps);
	if (!is_raw(rem)) {
		if (has_header(rem)) {
			fprintf(f, "  header      %5u %5u\n", (__u32) rem->phead, (__u32) rem->shead);
		}
		if (rem->pthree != 0 || rem->sthree != 0)
			fprintf(f, "  three       %5u %5u\n", (__u32) rem->pthree, (__u32) rem->sthree);
		if (rem->ptwo != 0 || rem->stwo != 0)
			fprintf(f, "  two         %5u %5u\n", (__u32) rem->ptwo, (__u32) rem->stwo);
		fprintf(f, "  one         %5u %5u\n", (__u32) rem->pone, (__u32) rem->sone);
		fprintf(f, "  zero        %5u %5u\n", (__u32) rem->pzero, (__u32) rem->szero);
	}
	if (rem->ptrail != 0) {
		fprintf(f, "  ptrail      %5u\n", (__u32) rem->ptrail);
	}
	if (!is_raw(rem)) {
		if (rem->plead != 0) {
			fprintf(f, "  plead       %5u\n", (__u32) rem->plead);
		}
		if (has_foot(rem)) {
			fprintf(f, "  foot        %5u %5u\n", (__u32) rem->pfoot, (__u32) rem->sfoot);
		}
	}
	if (has_repeat(rem)) {
		fprintf(f, "  repeat      %5u %5u\n", (__u32) rem->prepeat, (__u32) rem->srepeat);
	}
	if (!is_raw(rem)) {
		if (rem->pre_data_bits > 0) {
			fprintf(f, "  pre_data_bits   %d\n", rem->pre_data_bits);
			fprintf(f, "  pre_data       0x%llX\n", (unsigned long long)rem->pre_data);
		}
		if (rem->post_data_bits > 0) {
			fprintf(f, "  post_data_bits  %d\n", rem->post_data_bits);
			fprintf(f, "  post_data      0x%llX\n", (unsigned long long)rem->post_data);
		}
		if (rem->pre_p != 0 && rem->pre_s != 0) {
			fprintf(f, "  pre         %5u %5u\n", (__u32) rem->pre_p, (__u32) rem->pre_s);
		}
		if (rem->post_p != 0 && rem->post_s != 0) {
			fprintf(f, "  post        %5u %5u\n", (__u32) rem->post_p, (__u32) rem->post_s);
		}
	}
	fprint_remote_gap(f, rem);
	if (has_repeat_gap(rem)) {
		fprintf(f, "  repeat_gap   %u\n", (__u32) rem->repeat_gap);
	}
	if (rem->suppress_repeat > 0) {
		fprintf(f, "  suppress_repeat %d\n", rem->suppress_repeat);
	}
	if (rem->min_repeat > 0) {
		fprintf(f, "  min_repeat      %d\n", rem->min_repeat);
		if (rem->suppress_repeat == 0) {
			fprintf(f, "#  suppress_repeat %d\n", rem->min_repeat);
			fprintf(f, "#  uncomment to suppress unwanted repeats\n");
		}
	}
	if (!is_raw(rem)) {
		if (rem->min_code_repeat > 0) {
			fprintf(f, "  min_code_repeat %d\n", rem->min_code_repeat);
		}
		fprintf(f, "  toggle_bit_mask 0x%llX\n", (unsigned long long)rem->toggle_bit_mask);
		if (has_toggle_mask(rem)) {
			fprintf(f, "  toggle_mask    0x%llX\n", (unsigned long long)rem->toggle_mask);
		}
		if (rem->rc6_mask != 0) {
			fprintf(f, "  rc6_mask    0x%llX\n", (unsigned long long)rem->rc6_mask);
		}
		if (has_ignore_mask(rem)) {
			fprintf(f, "  ignore_mask 0x%llX\n", (unsigned long long)rem->ignore_mask);
		}
		if (is_serial(rem)) {
			fprintf(f, "  baud            %d\n", rem->baud);
			fprintf(f, "  serial_mode     %dN%d%s\n", rem->bits_in_byte, rem->stop_bits / 2,
				rem->stop_bits % 2 ? ".5" : "");
		}
	}
	if (rem->freq != 0) {
		fprintf(f, "  frequency    %u\n", rem->freq);
	}
	if (rem->duty_cycle != 0) {
		fprintf(f, "  duty_cycle   %u\n", rem->duty_cycle);
	}
	fprintf(f, "\n");
}

void fprint_remote_foot(FILE * f, struct ir_remote *rem)
{
	fprintf(f, "end remote\n");
}

void fprint_remote_signal_head(FILE * f, struct ir_remote *rem)
{
	if (!is_raw(rem))
		fprintf(f, "      begin codes\n");
	else
		fprintf(f, "      begin raw_codes\n\n");
}

void fprint_remote_signal_foot(FILE * f, struct ir_remote *rem)
{
	if (!is_raw(rem))
		fprintf(f, "      end codes\n\n");
	else
		fprintf(f, "      end raw_codes\n\n");
}

void fprint_remote_signal(FILE * f, struct ir_remote *rem, struct ir_ncode *codes)
{
	int i, j;

	if (!is_raw(rem)) {
		char format[30];
		struct ir_code_node *loop;

		sprintf(format, "          %%-24s 0x%%0%dllX", (rem->bits + 3) / 4);
		fprintf(f, format, codes->name, codes->code);
		sprintf(format, " 0x%%0%dlX", (rem->bits + 3) / 4);
		for (loop = codes->next; loop != NULL; loop = loop->next) {
			fprintf(f, format, loop->code);
		}

		fprintf(f, "\n");
	} else {
		fprintf(f, "          name %s\n", codes->name);
		j = 0;
		for (i = 0; i < codes->length; i++) {
			if (j == 0) {
				fprintf(f, "          %7u", (__u32) codes->signals[i]);
			} else if (j < 5) {
				fprintf(f, " %7u", (__u32) codes->signals[i]);
			} else {
				fprintf(f, " %7u\n", (__u32) codes->signals[i]);
				j = -1;
			}
			j++;
		}
		codes++;
		if (j == 0) {
			fprintf(f, "\n");
		} else {
			fprintf(f, "\n\n");
			j = 0;
		}
	}
}

void fprint_remote_signals(FILE * f, struct ir_remote *rem)
{
	struct ir_ncode *codes;

	fprint_remote_signal_head(f, rem);
	codes = rem->codes;
	while (codes->name != NULL) {
		fprint_remote_signal(f, rem, codes);
		codes++;
	}
	fprint_remote_signal_foot(f, rem);
}

void fprint_remote(FILE * f, struct ir_remote *rem)
{
	fprint_comment(f, rem);
	fprint_remote_head(f, rem);
	fprint_remote_signals(f, rem);
	fprint_remote_foot(f, rem);
}
