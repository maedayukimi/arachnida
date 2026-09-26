#include <curl/curl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static bool	flag = 0;
static char*	root_url = 0;

typedef struct
{
	char*	data;
	size_t	size;
}	t_buffer;

// u need to check is it "Absolute Path(https://url/cat.jpg)", "Protocol Relative(//url/cat.jpg)", "Root Relative(/cat.img)" or "Relative Path(cat.jpg || ../cat.jpg)"
//char*	solve_url(char* dst)// now dst = cat.jpg
//{
//	char*	buf;
//	size_t	len;
//
//	if (strncmp("https://", dst, 8) == 0 || strncmp("http://", dst, 7) == 0)//Abs path
//		return (dst);
//
//	if (strncmp("//", dst, 2) == 0)//Protocol relative path
//	{
//		if (flag)
//		{
//			len = strlen(dst) + 6;
//			buf = malloc(len + 1);
//			if (!buf)
//				exit(1);
//
//			memcpy(buf, "https:");
//			memcpy(buf + 6, dst);
//			buf[len] = 0;
//			free(dst);
//
//			return buf;
//		}
//		else
//		{
//			len = strlen(dst) + 5;
//			buf = malloc(len + 1);
//			if (!buf)
//				exit(1);
//
//			memcpy(buf, "http:");
//			memcpy(buf + 5, dst);
//			buf[len] = 0;
//			free(dst);
//
//			return buf;
//		}
//	}
//
//	if (strncmp("/", dst, 1) == 0)//Root relative path
//	{
//		if (flag)
//		{
//		}
//		else
//		{
//		}
//	}
//
//	if ()//Relative path
//}

size_t	print_token(char* src)
{
	bool	flag = false;

	char*	start = strstr(src, "<img");
	if (!start)
		return 0;
	start+= 4;

	char*	end = strstr(start, ">");
	if (!end)
		return 0;

	char*	s_start = strstr(start, "src=\"");
	if (!s_start || s_start > end)
	{
		s_start = strstr(start, "src=\'");
		if (!s_start || s_start > end)
			return 0;
		flag = true;
	}
	s_start += 5;

	char*	s_end;
	if (!flag)
		s_end = strstr(s_start, "\"");
	else
		s_end = strstr(s_start, "\'");
	if (!s_end)
		return 0;
	size_t	len = s_end - s_start;

	char*	dst = malloc(len + 1);
	if (!dst)
		exit(1);

	memcpy(dst, s_start, len);
	dst[len] = 0;

//	dst = solve_url(dst);
	printf("%s\n", dst);
	free(dst);
	return (s_end - src + 1);
}

size_t	write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	t_buffer*	buf = (t_buffer*)userdata;
	size_t		realsize = size * nmemb;
	buf->data = realloc(buf->data, realsize + buf->size + 1);
	if (!buf->data)
		exit(1);

	for (int i = 0; i < realsize; i++)
		buf->data[buf->size + i] = ptr[i];
	buf->size += realsize;
	buf->data[buf->size] = 0;

	return (size * nmemb);
}

static void	find_root(char* url)//https://url.com/cat.img
{
	char*	path = strstr(url, "//");
	path += 2;

	char*	end = strstr(path, "/");
	if (!end)
	{
		root_url = calloc(strlen(path) + 1, 1);
		if (!root_url)
			exit(1);

		memcpy(root_url, path, strlen(path));
		root_url[strlen(path)] = 0;
		return ;
	}

	size_t	len = end - path;
	root_url = calloc(len + 1, 1);
	if (!root_url)
		exit(1);

	memcpy(root_url, path, len);
	root_url[len] = 0;
	return ;
}

int	main(int ac, char** av)
{
	if (ac < 2)
	{
		printf("Wrong number of arguments.\n");
		exit(1);
	}

	CURL *curl = curl_easy_init();

	if (curl)
	{
		CURLcode	res;

		t_buffer	buf;
		buf.data = malloc(1);
		if (!buf.data)
			exit(1);
		buf.size = 0;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
		curl_easy_setopt(curl, CURLOPT_URL, av[1]);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "my-spidar/1.0");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		char*	itr = buf.data;
		size_t	index;

		if (strncmp(av[1], "https://", 8))
			flag = 1;

		find_root(av[1]);
		printf("%s\n", root_url);

		while (1)
		{
			index = print_token(itr);
			if (!index)
				break ;
			itr += index + 1;
		}
	}
	free(root_url);
}
