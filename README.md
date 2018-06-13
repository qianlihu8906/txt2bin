# txt2bin

这是一个根据`编译原理`步骤实现的数据文本转二进制的工具

## Install

gcc txt2bin.c -o txt2bin

## Usage
	txt2bin -i input_file -o output_file -[hlb] 
	-h 按照本地大小端格式存储二进制文件
	-l 小端格式存储二进制文件
	-b 大端格式存储二进制文件
        默认本地格式存储

## example

	./txt2bin -i dat.txt -o dat.bin
	./txt2bin -i dat.txt -o dat.bin -h
	./txt2bin -i dat.txt -o dat.bin -l
	./txt2bin -i dat.txt -o dat.bin -b

## 格式要求

	1. 每个数据以逗号或空格分割，支持无符号32bit数据
	2. 16进制数据需要以0x开头

*转化不通过会有提示*
