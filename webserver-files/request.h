#ifndef __REQUEST_H__

void requestHandle(int fd, int* th_dyn_count, int* th_stat_count, struct timeval* arrival, struct timeval* handle);

#endif
