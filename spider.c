#include <curl/curl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
	char*	data;
	size_t	size;
}	t_buffer;

bool	print_token(char* src)
{
	char*	start = strstr(src, "<img");
	start+= 4;

	char*	s_start = strstr(start, "src=\"");
	s_start += 5;

	char*	s_end = strstr(s_start, "\"");
	size_t	len = s_end - s_start;

	char*	dst = malloc(len + 1);
	if (!dst)
		exit(1);

	memcpy(dst, s_start, len);
	dst[len] = 0;

	printf("%s\n", dst);
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

int	main(void)
{
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
		curl_easy_setopt(curl, CURLOPT_URL, "http://books.toscrape.com/");
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "my-spidar/1.0");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		print_token(buf.data);
	}
}
