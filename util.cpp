#include <stdlib.h>

char *my_strdup(const char *str)
{
    if (str == NULL)
        return NULL;

    char *strat = (char *)str;
    int len = 0;
    while (*str++ != '\0')
        len++;
    char *ret = (char *)malloc(len + 1);

    while ((*ret++ = *strat++) != '\0')
    {
    }

    return ret - (len + 1);
}