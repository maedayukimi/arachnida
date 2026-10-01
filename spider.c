#include <curl/curl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>

static bool	flag = 0;
static char*	root_url = 0;
static char*	full_path = 0;

typedef struct
{
	char*	data;
	size_t	size;
}	t_buffer;

// u need to check is it "Absolute Path(https://url/cat.jpg)", "Protocol Relative(//url/cat.jpg)", "Root Relative(/cat.img)" or "Relative Path(cat.jpg || ../cat.jpg)"
char*	solve_url(char* dst)// now dst = cat.jpg
{
	char*	buf;
	size_t	len;

	if (strncmp("https://", dst, 8) == 0 || strncmp("http://", dst, 7) == 0)//Abs path
		return (dst);

	if (strncmp("//", dst, 2) == 0)//Protocol relative path
	{
		if (flag)
		{
			len = strlen(dst) + 6;
			buf = malloc(len + 1);
			if (!buf)
				exit(1);

			memcpy(buf, "https:", 6);
			memcpy(buf + 6, dst, strlen(dst));
			buf[len] = 0;
			free(dst);

			return buf;
		}
		else
		{
			len = strlen(dst) + 5;
			buf = malloc(len + 1);
			if (!buf)
				exit(1);

			memcpy(buf, "http:", 5);
			memcpy(buf + 5, dst, strlen(dst));
			buf[len] = 0;
			free(dst);

			return buf;
		}
	}

	if (strncmp("/", dst, 1) == 0)//Root relative path
	{
		if (flag)
		{
			len = strlen(dst) + strlen(root_url) + 8;
			buf = calloc(len + 1, 1);
			if (!buf)
				exit(1);

			memcpy(buf, "https://", 8);
			memcpy(buf + 8, root_url, strlen(root_url));
			memcpy(buf + strlen(root_url) + 8, dst, strlen(dst));
			buf[len] = 0;
			free(dst);

			return buf;
		}

		else// http://test.com/cat.img	http://, root_url = test.com/, dst = cat.img
		{
			len = strlen(dst) + strlen(root_url) + 7;
			buf = calloc(len + 1, 1);
			if (!buf)
				exit(1);

			memcpy(buf, "http://", 7);
			memcpy(buf + 7, root_url, strlen(root_url) - 1);
			memcpy(buf + strlen(root_url) + 6, dst, strlen(dst));
			buf[len] = 0;
			free(dst);
			return buf;
		}
	}

	// Relative path
	// root_url = url/test/ want to return => url/cat.jpg

	int	count = 0;
	char*	src;// dst = ../cat.jpg
	char*	last;
	char*	url;

	src = calloc(strlen(dst) + 1, 1);
	if (!src)
		exit(1);

	char*	f_src = src;
	memcpy(src, dst, strlen(dst));
	src[strlen(dst)] = 0;
	while (strncmp(src, "../", 3) == 0)
	{
		count++;
		src += 3;
	}

	url = malloc(strlen(full_path) + 1);
	if (!url)
		exit(1);

	memcpy(url, full_path, strlen(full_path));
	url[strlen(full_path)] = 0;

	char*	host_path;
	if (flag)
		host_path = url + 8;
	else
		host_path = url + 7;

	for (int i = 0; i < count + 1; i++)
	{
		char* found = strrchr(url, '/');//url[9]
		if (!found || found < host_path)
			break ;
		last = found;
		url[last - url] = 0;
	}
	size_t	offset = last - url;
	url = realloc(url, strlen(url) + 2);
	url[offset] = '/';
	url[offset + 1] = 0;
	len = strlen(url) + strlen(src);
	buf = malloc(len + 1);
	if (!buf)
		exit(1);

	memcpy(buf, url, strlen(url));
	memcpy(buf + strlen(url), src, strlen(src));
	buf[len] = 0;
	free(f_src);
	free(url);
	free(dst);
	return buf;
}

size_t	write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	t_buffer*	buf = (t_buffer*)userdata;
	size_t		realsize = size * nmemb;
	buf->data = realloc(buf->data, realsize + buf->size + 1);
	if (!buf->data)
		exit(1);

	for (size_t i = 0; i < realsize; i++)
		buf->data[buf->size + i] = ptr[i];
	buf->size += realsize;
	buf->data[buf->size] = 0;

	return (size * nmemb);
}

size_t	spider(char* src)
{
	bool	s_flag = false;

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
			return (end - src + 1); //continue whatever img broken
		s_flag = true;
	}
	s_start += 5;

	char*	s_end;
	if (!s_flag)
		s_end = strstr(s_start, "\"");
	else
		s_end = strstr(s_start, "\'");
	if (!s_end || s_end > end)
		return (end - src + 1); //continue whatever img broken
	size_t	len = s_end - s_start;

	char*	dst = malloc(len + 1);
	if (!dst)
		exit(1);

	memcpy(dst, s_start, len);
	dst[len] = 0;

	dst = solve_url(dst);

	CURL*	img_curl = curl_easy_init();
	if (img_curl)
	{
		t_buffer	img_buf;
		CURLcode	res;

		img_buf.data = malloc(1);
		if (!img_buf.data)
			exit(1);

		img_buf.size = 0;

		curl_easy_setopt(img_curl, CURLOPT_URL, dst);
		curl_easy_setopt(img_curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(img_curl, CURLOPT_WRITEDATA, &img_buf);
		curl_easy_setopt(img_curl, CURLOPT_USERAGENT, "my-spidar/1.0");
		curl_easy_setopt(img_curl, CURLOPT_FAILONERROR, 1L);// 4xx/5xx -> res != CURLE_OK
		res = curl_easy_perform(img_curl);
		curl_easy_cleanup(img_curl);

		char*	name = strrchr(dst, '/') + 1;// "http://host/img/cat.jpg" -> "cat.jpg"
		size_t	name_len = strlen(name);

		if (res != CURLE_OK)
			fprintf(stderr, "spider: %s: %s\n", dst, curl_easy_strerror(res));
		else if (name_len == 0)// "http://host/img/" -> no file name
			fprintf(stderr, "spider: %s: no file name\n", dst);
		else
		{
			char*	save_path = malloc(7 + name_len + 1);
			if (!save_path)
				exit(1);

			memcpy(save_path, "./data/", 7);
			memcpy(save_path + 7, name, name_len);
			save_path[7 + name_len] = 0;
			if (access(save_path, F_OK) == 0)
			{
				// TODO: rename to cat_1.jpg, cat_2.jpg ...

			}

			FILE*	file = fopen(save_path, "wb");
			if (!file)
				perror(save_path);
			else
			{
				fwrite(img_buf.data, 1, img_buf.size, file);
				fclose(file);
			}
			free(save_path);
		}
		free(img_buf.data);
	}

	printf("%s\n", dst);
	free(dst);
	return (s_end - src + 1);
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
	root_url = calloc(len + 2, 1);
	if (!root_url)
		exit(1);

	memcpy(root_url, path, len);
	root_url[len] = '/';
	root_url[len + 1] = 0;
	return ;
}

int	main(int ac, char** av)
{
	if (ac < 2)
	{
		printf("Wrong number of arguments.\n");
		exit(1);
	}


	if (strncmp(av[1], "https://", 8) == 0)
		flag = 1;
	else if (strncmp(av[1], "http://", 7) != 0)
	{
		fprintf(stderr, "usage: %s URL\n", av[0]);
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

		find_root(av[1]);
		full_path = av[1];

		if (mkdir("./data", 0755) == -1 && errno != EEXIST)
			exit(1);

		while (1)
		{
			index = spider(itr);
			if (!index)
				break ;
			itr += index;
		}
		free(buf.data);
	}
	free(root_url);
}
