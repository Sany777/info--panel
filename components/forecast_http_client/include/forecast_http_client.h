#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif






int get_weather(const char *city, const char *api_key);

extern char network_buf[];










#ifdef __cplusplus
}
#endif









#endif