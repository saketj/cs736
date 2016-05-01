#define DEBUG_PRINT 0
#if DEBUG_PRINT
# define Dprintf(x...) fprintf(stderr, x)
#else
# define Dprintf(x...) do {} while(0)
#endif
