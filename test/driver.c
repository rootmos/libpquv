#include "test-cases.h"

#include <time.h>
#include <stdlib.h>

int main()
{
    srand(time(NULL));
    sanity_check();
    simple_read();
    return 0;
}
