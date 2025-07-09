/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *query_string = getenv("QUERY_STRING");
    int a, b;

    if (query_string == NULL || sscanf(query_string, "%d&%d", &a, &b) != 2) {
        printf("Content-type: text/html\r\n\r\n");
        printf("<p>Error: Invalid query string.</p>\n");
        return 1;
    }

    printf("Content-type: text/html\r\n\r\n");
    printf("<html><body>");
    printf("<p>%d + %d = %d</p>", a, b, a + b);
    printf("</body></html>\n");
    
    return 0;
}

/* $end adder */
