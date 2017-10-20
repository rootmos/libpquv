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
    subsequent_drop_connection();
    subsequent_reject_connection();
    idle();
    large_write_and_read();
    return 0;
}
