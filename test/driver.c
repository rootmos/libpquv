#include "test-cases.h"

#include <time.h>
#include <stdlib.h>

int main()
{
    srand(time(NULL));
    sanity_check();
    simple_read();
    multiple_reads();
    simple_write();
    invalid_query();
    initial_bad_connection();
    return 0;
}
