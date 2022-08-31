#include "../include/helper.h"
#include <ctype.h>
#include <math.h>

char *trimWhitespace(char *inStr)
{
    char *end;
    while(isspace(*inStr))
    {
        ++inStr;
    }

    end = inStr + strnlen(inStr, 20) - 1;

    while(end > inStr && isspace(*end))
    {
        --end;
    }

    *(end+1) = '\0';

    return inStr;
}

int parseFirstNum(char *inStr)
{
    // Note max stat is a triple digit number
    int cnt = 0;
    char temp[3];
    int prev = 0;
    for (size_t i = 0; i <= strlen(inStr); ++i)
    {
        if (isdigit(inStr[i]))
        {
            if (prev == 0)
            {
                prev = 1;
                for (size_t j = i; j <= strlen(inStr) - 1; ++j)
                {
                    if (isdigit(inStr[j+1]))
                    {
                        temp[cnt] = inStr[j];
                        ++cnt;
                    }
                    else
                    {
                        temp[cnt] = inStr[j];
                        ++cnt;
                        goto exit;
                    }
                }
            }
        }
    }

exit:
    prev = 0;

    for (int i = 0; i < cnt; ++i)
    {
        prev += pow(10, cnt - i - 1) * (temp[i] - '0');
    }
    return prev;
}

