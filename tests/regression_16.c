// Copyright 2020 Inria (https://www.inria.fr/)

// Regression test for Melissa issue #16:
// Undefined behavior in `common/melissa_utils.c`

#include <common/melissa_utils.h>
#include <stdint.h>

int main()
{
	uint32_t actual_value = 0;
	uint32_t expected_value = 1;

	for(int i = 0; i < 32; ++i)
	{
		set_bit(&actual_value, i);

		if(actual_value != expected_value)
			return 3*i + 0;

		if(test_bit(&actual_value, i) != 1)
			return 3*i + 1;

		clear_bit(&actual_value, i);

		if(actual_value != 0)
			return 3*i + 2;

		expected_value *= 2;
	}
}
