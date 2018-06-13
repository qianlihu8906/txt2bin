#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <endian.h>
#include <ctype.h>
#include <unistd.h>

struct context{
	char in_path[256];
	FILE *in_fp;
#define BUFFER_LEN	2048
	char in_buffer[BUFFER_LEN];
	int in_buffer_pos;
	int in_buffer_len;
	int line_no;

#define ENDIAN_HOST		 0
#define ENDIAN_LITTLE		 1
#define ENDIAN_BIG   		 2
	char endian;

	char out_path[256];
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
	strncpy(c->in_path,in_path,sizeof(c->in_path));
	strncpy(c->out_path,out_path,sizeof(c->out_path));
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

	const char *p = fgets(buf,BUFFER_LEN,in_fp);
	if(p == NULL){
		c->in_buffer_len = 0;
		c->in_buffer_pos = 0;
		if(ferror(in_fp)){
			fprintf(stderr,"file error it should not be happened!");
			return -1;
		}
		return 0;
	}
	c->in_buffer_len = strlen(buf);
	c->in_buffer_pos = 0;
	c->line_no++;

	return 0;
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

static void print_errmsg(struct context *c,const char *msg)
{
	fprintf(stderr,"%s:%d:%d:Error:%s\n",c->in_path,c->line_no,c->in_buffer_pos+1,msg);
	fprintf(stderr,"%s",c->in_buffer);
	char buffer[BUFFER_LEN] = {};
	memcpy(buffer,c->in_buffer,c->in_buffer_len);
	int i;
	for(i=0;i<c->in_buffer_len;i++){
		if(!isspace(buffer[i]))
			buffer[i] = ' ';
	}
	buffer[c->in_buffer_pos] = '^';

	fprintf(stderr,"%s",buffer);
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

static int compile(struct context *c,uint32_t number)
{
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

static int parse_hex(struct context *c)
{
	char buf[64] = {};
	int i = 0;
	while(1){
		int t = get_token(c);
		if(t== ',' || isspace(t)){
			next_token(c);
			break;
		}
		if(!isxdigit(t)){
			print_errmsg(c,"unexpected character");
			return -1;
		}
		next_token(c);
		buf[i++] = t;
	}
	uint32_t number = strtol(buf,NULL,16);
	compile(c,number);
	return 0;
}

static int parse_int(struct context *c)
{
	char buf[64] = {};
	int i = 0;
	while(1){
		int t = get_token(c);
		if(t== ',' || isspace(t)){
			next_token(c);
			break;
		}
		if(!isdigit(t)){
			print_errmsg(c,"unexpected character");
			return -1;
		}
		next_token(c);
		buf[i++] = t;
	}
	uint32_t number = strtol(buf,NULL,10);
	compile(c,number);
	return 0;
}

static int parse_number(struct context *c)
{
	int t = get_token(c);
	if(t == '0'){
		next_token(c);
		t = get_token(c);
		if(t != 'x'){
			print_errmsg(c,"unexpected value");
			return -1;
		}
		next_token(c);
		return parse_hex(c);
	}
	return parse_int(c);
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
