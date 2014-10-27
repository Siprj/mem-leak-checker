#ifndef CONFIG_H
#define CONFIG_H

// Set log files names.
#define LOG_FILE "mem-leak-analysis.bin"
#define LOG_FILE_SUMARY "mem-leak-analysis-summary.txt"

// Environment variable name to set outputpath
#define ENV_OUTPUT_PATH "MEM_LEAK_OUT"

#define CACHE_SIZE 100
#define NUMBER_OF_CACHES 2

#define DEBUG_ENABLE
#define DEBUG_LEVEL DEBUG_LEVEL_WARN

#endif // CONFIG_H
