#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <endian.h>
#include <ctype.h>
#include <unistd.h>

struct context{
	FILE *in_fp;
#define BUFFER_LEN	2048
	char in_buffer[BUFFER_LEN];
	int in_buffer_pos;
	int in_buffer_len;
#define ENDIAN_HOST		 0
#define ENDIAN_LITTLE		 1
#define ENDIAN_BIG   		 2
	char endian;
	FILE *out_fp;
};

static int load_in_buffer(struct context *c);
static int init(struct context *c,const char *in_path,const char *out_path,char endian)
{
	memset(c,0,sizeof(*c));
	c->endian = endian;
	FILE *in_fp =  fopen(in_path,"r");
	if(in_fp == NULL)
		return -1;
	c->in_fp = in_fp;
	load_in_buffer(c);

	FILE *out_fp = fopen(out_path,"w+");
	if(out_fp == NULL){
		fclose(in_fp);
		return -1;
	}
	c->out_fp = out_fp;

	return 0;
}

static void finish(struct context *c)
{
	fclose(c->in_fp);
	fclose(c->out_fp);
}

static int load_in_buffer(struct context *c)
{
	FILE *in_fp = c->in_fp;
	char *buf =  c->in_buffer;

	if(feof(in_fp)){
		c->in_buffer_len = 0;
		c->in_buffer_pos = 0;
		return -1;
	}
	int r = fread(buf,1,BUFFER_LEN,in_fp);
	if( r != BUFFER_LEN){
		if(ferror(in_fp)){
			c->in_buffer_len = 0;
			c->in_buffer_pos = 0;
			return -1;
		}
	}
	c->in_buffer_pos = 0;
	c->in_buffer_len = r;
	return r;
}

static void next_token(struct context *c)
{
	if(c->in_buffer_pos+1 == c->in_buffer_len){
		load_in_buffer(c);
		return;
	}

	c->in_buffer_pos++;
}

static int get_token(struct context *c)
{
	if(c->in_buffer_pos == 0 && c->in_buffer_len == 0){
		return EOF;
	}
	const char *p = c->in_buffer+c->in_buffer_pos;
	return *p;
}

static void parse_whitespace(struct context *c)
{
	while(1){
		int t = get_token(c);
		if(!isspace(t))
			break;
		next_token(c);
			
	}
}


static int compile(struct context *c,const char *buf)
{
	uint32_t number = strtol(buf,NULL,0);
	switch(c->endian){
		case ENDIAN_HOST:
			break;
		case ENDIAN_LITTLE:
			number = htole32(number);
			break;
		case ENDIAN_BIG:
			number = htobe32(number);
			break;
		default:
			break;
	};

	fwrite(&number,sizeof(number),1,c->out_fp);
	return 0;
}

static int parse_number(struct context *c)
{
	char buf[64]= {};
	int i = 0;
	while(1){
		int t = get_token(c);
		if(t == ',' || isspace(t)){
			next_token(c);
			break;
		}
		if(t != 'x' && !isxdigit(t)){
			printf("unexpected character %c\n",t);
			return -1;
		}
		next_token(c);
		buf[i++] = t;
		assert(i < 64);
	}
	return compile(c,buf);
}

static int parse(struct context *c)
{
	parse_whitespace(c);
	while(1){
		int r = get_token(c);
		if(r == EOF)
			return 0;
		r = parse_number(c);	
		if(r < 0)
			return -1;
		parse_whitespace(c);
	}
}

static void print_usage(const char *name)
{
	printf("Usage: %s -i infile -o outfile -[hlb]\n",name);
	printf("       -h host_endian\n");
	printf("       -l little_endian\n");
	printf("       -b big_endian\n");
	exit(0);
}

int main(int argc,char **argv)
{
	const char *infile = NULL;
	const char *outfile = NULL;
	struct context c;
	char endian = ENDIAN_HOST;
	int opt;
	while((opt=getopt(argc,argv,"i:o:hlb")) != -1){
		switch(opt){
			case 'i':
				infile = optarg;
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				endian = ENDIAN_HOST;
				break;
			case 'l':
				endian = ENDIAN_LITTLE;
				break;
			case 'b':
				endian = ENDIAN_BIG;
				break;
		};
	}
	if(infile == NULL || outfile == NULL)
		print_usage(argv[0]);
	init(&c,infile,outfile,endian);
	parse(&c);
	finish(&c);
}


