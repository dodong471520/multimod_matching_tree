
/*

multimod_matching_tree License
-----------

multimod_matching_tree is licensed under the terms of the MIT license reproduced below.
This means that Log4z is free software and can be used for both academic
and commercial purposes at absolutely no cost.


===============================================================================

Copyright (C) 2014-2015 YaweiZhang <yawei_zhang@foxmail.com>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

===============================================================================

(end of COPYRIGHT)


*/
#ifndef _MULTI_MOD_MATCHING_TREE_H_
#define _MULTI_MOD_MATCHING_TREE_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
*/
struct match_tree_node 
{
	unsigned char _is_valid_node;
	unsigned char _is_boundary;
	struct match_tree_node * _child_tree; //match_tree_node[256]
};

struct match_tree_head
{
	struct match_tree_node * _tree;
	unsigned int _tree_node_count;
	unsigned int _tree_node_valid_count;
	unsigned int _tree_node_maximum_len;
	unsigned int _tree_node_minimum_len;
};

static struct match_tree_head * match_tree_init();
static int match_tree_add_pattern(struct match_tree_head * head, const char * pattern, unsigned int pattern_len, unsigned char level);
static struct match_tree_head * match_tree_init_from_file(const char * file_name, const char* delimiter, unsigned int delimiter_len);

static unsigned int match_tree_matching(const struct match_tree_head *head, const char * text, unsigned int text_len, unsigned char is_greedy);
static void match_tree_translate(const struct match_tree_head *head, char * text, unsigned int text_len, unsigned char is_greedy, char escape);
static void match_tree_free(struct match_tree_head **head);

static struct match_tree_head * match_tree_init()
{
	struct match_tree_head * head = malloc(sizeof(struct match_tree_head));
	memset(head, 0, sizeof(struct match_tree_head));
	return head;
}

static int match_tree_add_pattern(struct match_tree_head * head, const char * pattern, unsigned int pattern_len, unsigned char level)
{
	struct match_tree_node **tree = NULL;
	unsigned int i = 0;
	unsigned char node = 0;

	if (head == NULL)
	{
		return -1;
	}
	tree = &head->_tree;
	if (pattern_len == 0)
	{
		return 0;
	}
	for (i = 0; i < pattern_len; i++)
	{
		//check  is current node had all childs. if not then malloc it.
		if (*tree == NULL)
		{
			*tree = malloc(sizeof(struct match_tree_node) * 256);
			memset(*tree, 0, sizeof(struct match_tree_node) * 256);
			head->_tree_node_count += 256;
		}
		
		node = (unsigned char)pattern[i];
		//set node valid, default is invalid
		(*tree)[node]._is_valid_node = 1;

		//check is the boundary, if true then set bound.
		if (i == pattern_len - 1 && !(*tree)[node]._is_boundary)
		{
			(*tree)[node]._is_boundary = 1;
			head->_tree_node_valid_count++;
			if (pattern_len > head->_tree_node_maximum_len)
			{
				head->_tree_node_maximum_len = pattern_len;
			}
			if (pattern_len < head->_tree_node_minimum_len)
			{
				head->_tree_node_minimum_len = pattern_len;
			}
		}
		tree = &(*tree)[node]._child_tree;
	}
	return 0;
}

static struct match_tree_head * match_tree_init_from_file(const char * file_name, const char * delimiter, unsigned int delimiter_len)
{
	FILE * fp = NULL;
	char * file_content = NULL;

	unsigned int file_content_len = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int is_delimiter = 0;
	char * pattern = NULL;
	unsigned int pattern_len = 0;

	struct match_tree_head *head = NULL;
	if (delimiter == NULL || delimiter_len == 0)
	{
		return NULL;
	}
	

#ifdef WIN32
	if (fopen_s(&fp, file_name, "rb") != 0)
	{
		return NULL;
	}
#else
	fp = fopen(file_name, "rb");
	if (!fp)
	{
		return NULL;
	}
#endif

	fseek(fp, 0, SEEK_END);
	file_content_len = ftell(fp);
	file_content = (char*)malloc(file_content_len + 1);
	file_content[file_content_len] = '\0';
	fseek(fp, 0, SEEK_SET);
	file_content_len = fread(file_content, 1, file_content_len, fp);
	fclose(fp);
	fp = NULL;
	head = match_tree_init();

	//delete file BOM head
	if (file_content_len >= 3 
		&& (unsigned char)file_content[0] == 0xef 
		&& (unsigned char)file_content[1] == 0xbb 
		&& (unsigned char)file_content[2] == 0xbf)
	{
		pattern = file_content+3;
		i = 3;
	}
	else
	{
		pattern = file_content;
		i = 0;
	}
	
	while ( i + delimiter_len < file_content_len)
	{
		is_delimiter = 1;
		for (j = 0; j < delimiter_len; j++)
		{
			if (file_content[i+j] != delimiter[j])
			{
				is_delimiter = 0;
				break;
			}
		}

		if (is_delimiter)
		{
			pattern_len = file_content + i - pattern;
			match_tree_add_pattern(head, pattern, pattern_len, 0);
			i = i + delimiter_len;
			pattern = file_content + i + delimiter_len;
		}
		else
		{
			i++;
		}
	}

	free(file_content);
	file_content = NULL;
	file_content_len = 0;
	return head;
}

static unsigned int match_tree_matching(const struct match_tree_head *head, const char * text, unsigned int text_len, unsigned char is_greedy)
{
	unsigned int ret_len = 0;
	unsigned int i = 0;
	unsigned char node = 0;
	const struct match_tree_node * tree = NULL;
	if (text == NULL || text_len == 0 || head == NULL)
	{
		return ret_len;
	}
	tree = head->_tree;
	for (i = 0; i < text_len; ++i)
	{
		node = (unsigned char)text[i];
		if (tree == NULL || !tree[node]._is_valid_node)
		{
			break;
		}
		
		if (tree[node]._is_boundary)
		{
			ret_len = i+1;
			if (!is_greedy)
			{
				break;
			}
		}
		tree = tree[node]._child_tree;
	}
	return ret_len;
}

static void match_tree_translate(const struct match_tree_head *head, char * text, unsigned int text_len, unsigned char is_greedy, char escape)
{
	unsigned int matching_count = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	for (i = 0; i < text_len;)
	{
		matching_count = match_tree_matching(head, text + i, text_len - i, is_greedy);
		if (matching_count > 0)
		{
			for (j = i; j < i + matching_count; j++)
			{
				text[j] = escape;
			}
			i += matching_count;
		}
		else
		{
			i++;
		}
	}
}

static void __match_tree_free(struct match_tree_node *tree)
{
	unsigned int i = 0;
	if (tree->_child_tree)
	{
		for (i = 0; i < 256; i++)
		{
			if (tree->_child_tree[i]._child_tree)
			{
				__match_tree_free(&tree->_child_tree[i]);
			}
		}
		free(tree->_child_tree);
	}
}
static void match_tree_free(struct match_tree_head **head)
{
	if (head == NULL)
	{
		return;
	}
	if (*head == NULL)
	{
		return;
	}
	if ((*head)->_tree)
	{
		__match_tree_free((*head)->_tree);
	}
	free((*head)->_tree);
	*head = NULL;
}





#endif




