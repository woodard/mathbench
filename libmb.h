#ifndef __LIBMB_H__
#define __LIBMB_H_

double (*getFunc(char *optarg))(double);
void read_numbers(char *filename, bool nonnormals,
		  std::vector< double> &numbers);

#endif
