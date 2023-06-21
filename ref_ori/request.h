#ifndef __REQUEST_H__

void requestHandle(int fd ,int* counter , int* c_static, int* c_dynamic , struct timeval* time, struct timeval* d_time, int tid);

#endif
