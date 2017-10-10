#include "test-cases.h"

#include <time.h>
#include <stdlib.h>

int main()
{
    srand(time(NULL));
    sanity_check();
    simple_read();
    parametrized_query();
    prepared_statement();
    multiple_reads();
    simple_write();
    invalid_query();
    initial_drop_connection();
    initial_reject_connection();
    return 0;
}
