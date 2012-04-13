#include <windows.h>

BOOL FileExists(const char *path);
char *GetDirectory(const char *path);
char *GetFile(const char *path);

int protocol_exists();
int protocol_register(const char *name, const char *exe);
int protocol_unregister(const char *name);
