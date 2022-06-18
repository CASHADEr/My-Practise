#include "Parser.hpp"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>





void copy_file_contents(char *file_path, off_t file_size, struct iovec *iov) {
                int fd;

                char *buf = malloc_throws(file_size);
                fd = open(file_path, O_RDONLY);
                if (fd < 0)
                                fatal_error("read");

                /* We should really check for short reads here */
                int ret = read(fd, buf, file_size);
                if (ret < file_size) {
                                fprintf(stderr, "Encountered a short read.\n");
                }
                close(fd);

                iov->iov_base = buf;
                iov->iov_len = file_size;
}

/*
 * Simple function to get the file extension of the file that we are about to serve.
 * */

const char *get_filename_ext(const char *filename) {
                const char *dot = strrchr(filename, '.');
                if (!dot || dot == filename)
                                return "";
                return dot + 1;
}

/*
 * Sends the HTTP 200 OK header, the server string, for a few types of files, it can also
 * send the content type based on the file extension. It also sends the content length
 * header. Finally it send a '\r\n' in a line by itself signalling the end of headers
 * and the beginning of any content.
 * */

void send_headers(const char *path, off_t len, struct iovec *iov) {
                char small_case_path[1024];
                char send_buffer[1024];
                strcpy(small_case_path, path);
                strtolower(small_case_path);

                char *str = "HTTP/1.0 200 OK\r\n";
                unsigned long slen = strlen(str);
                iov[0].iov_base = malloc_throws(slen);
                iov[0].iov_len = slen;
                memcpy(iov[0].iov_base, str, slen);

                slen = strlen(SERVER_STRING);
                iov[1].iov_base = malloc_throws(slen);
                iov[1].iov_len = slen;
                memcpy(iov[1].iov_base, SERVER_STRING, slen);

                /*
                 * Check the file extension for certain common types of files
                 * on web pages and send the appropriate content-type header.
                 * Since extensions can be mixed case like JPG, jpg or Jpg,
                 * we turn the extension into lower case before checking.
                 * */
                const char *file_ext = get_filename_ext(small_case_path);
                if (strcmp("jpg", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: image/jpeg\r\n");
                if (strcmp("jpeg", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: image/jpeg\r\n");
                if (strcmp("png", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: image/png\r\n");
                if (strcmp("gif", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: image/gif\r\n");
                if (strcmp("htm", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: text/html\r\n");
                if (strcmp("html", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: text/html\r\n");
                if (strcmp("js", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: application/javascript\r\n");
                if (strcmp("css", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: text/css\r\n");
                if (strcmp("txt", file_ext) == 0)
                                strcpy(send_buffer, "Content-Type: text/plain\r\n");
                slen = strlen(send_buffer);
                iov[2].iov_base = malloc_throws(slen);
                iov[2].iov_len = slen;
                memcpy(iov[2].iov_base, send_buffer, slen);

                /* Send the content-length header, which is the file size in this case. */
                sprintf(send_buffer, "content-length: %ld\r\n", len);
                slen = strlen(send_buffer);
                iov[3].iov_base = malloc_throws(slen);
                iov[3].iov_len = slen;
                memcpy(iov[3].iov_base, send_buffer, slen);

                /*
                 * When the browser sees a '\r\n' sequence in a line on its own,
                 * it understands there are no more headers. Content may follow.
                 * */
                strcpy(send_buffer, "\r\n");
                slen = strlen(send_buffer);
                iov[4].iov_base = malloc_throws(slen);
                iov[4].iov_len = slen;
                memcpy(iov[4].iov_base, send_buffer, slen);
}