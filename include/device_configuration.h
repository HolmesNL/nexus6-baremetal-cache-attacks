#define NUMBER_OF_SETS 2048 //2MiB / 8-way / 128B line length
#define NUMBER_OF_WAYS 8
#define LINE_LENGTH 128
#define LINE_LENGTH_LOG2 7

#define __include_strategy(x) #x
#define _include_strategy(x) __include_strategy(x)
#define include_strategy(x) _include_strategy(x)

#include include_strategy(STRATEGY)
