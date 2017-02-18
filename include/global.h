

#define assert(x,comp,y) if (!((x) comp (y))) {printk("***** [%s:%u]" "'" #x "'=%08lx " #comp " '" "'" #y "'=%08lx " "is false", __func__, __LINE__, (unsigned long)x, (unsigned long)y); while (1);} 

extern int printk(const char *fmt, ...);


