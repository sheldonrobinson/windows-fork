#ifndef __FORK_WINDOWS_H__
#define __FORK_WINDOWS_H__
 
#ifdef __cplusplus
extern "C" {
#endif

/**
* simulate fork on Windows
*/
int fork(void);
 
#ifdef __cplusplus
}
#endif
 
#endif /* __FORK_WINDOWS_H__ */