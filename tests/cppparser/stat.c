struct stat;

void stat();

// CHECK: void *ptr = (void *)(stat)
void *ptr = (void *)stat;
