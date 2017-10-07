#include "test-cases.h"

#include <time.h>
#include <stdlib.h>

int main()
{
    srand(time(NULL));
    sanity_check();
    return 0;
}
