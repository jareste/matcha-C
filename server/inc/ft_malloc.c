#include <ft_malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#undef malloc
#undef realloc

void* ft_malloc(size_t size)
{
#undef malloc
    void *ptr = malloc(size);
    ft_assert(ptr != NULL, "malloc failed");
#define malloc(x) ft_malloc(x)
    return ptr;
}

void* ft_realloc(void *ptr, size_t size)
{
#undef realloc
    void* new_ptr = realloc(ptr, size);
    ft_assert(new_ptr != NULL, "realloc failed");
#define realloc(x, y) ft_realloc(x, y)
    return new_ptr;
}

char *ft_strdup(const char *s)
{
    size_t len = strlen(s);
    char *new_s = ft_malloc(len + 1);
    strcpy(new_s, s);
    new_s[len] = '\0';
    return new_s;
}

void *ft_memmem(const void *haystack, size_t haystacklen,
                const void *needle, size_t needlelen)
{
    if (!haystack || !needle || haystacklen < needlelen)
        return NULL;

    for (size_t i = 0; i <= haystacklen - needlelen; i++)
    {
        if (memcmp((char *)haystack + i, needle, needlelen) == 0)
            return (char *)haystack + i;
    }

    return NULL;
}
