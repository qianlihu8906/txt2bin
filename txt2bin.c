#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <ctype.h>
/*
	input   txt: bigendian  little 
	output  bin: bigendian  little
*/


#define BUFFER_LEN	2048
struct context{
	FILE *in_fp;
	char in_buffer[BUFFER_LEN];
	int in_buffer_pos;
	int in_buffer_len;

	FILE *out_fp;
};

static int load_in_buffer(struct context *c);
static int init(struct context *c,const char *in_path,const char *out_path)
{
	memset(c,0,sizeof(*c));
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
		if(t == EOF)
			return -1;
		next_token(c);
		buf[i++] = t;
		assert(i < 64);
	}
	fprintf(c->out_fp,"0x%8.8x\n",(unsigned)strtol(buf,NULL,0));
	return 0;
}

static int parse(struct context *c)
{
	parse_whitespace(c);
	while(1){
		int r = parse_number(c);	
		if(r < 0)
			break;
		parse_whitespace(c);
	}
	return 0;
}

int main()
{
	struct context c;
	init(&c,"dat.txt","dat.bin");
	parse(&c);
	finish(&c);
}


