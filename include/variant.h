#ifndef VAR_H
#define VAR_H
#include <vector>
#include <string>
#include <string.h>
#include <cstdlib>
char *strdup (const char *s) {
    char *d = (char*) malloc (strlen (s) + 1);   // Space for length plus nul
    if (d == NULL) return NULL;          // No memory
    strcpy (d,s);                        // Copy the characters
    return d;                            // Return the new string
}

class   Variant
{
    enum class Type { Int,Float,String,Obj};
public:
    Type type;
    union {int intValue; float floatValue; const char* stringValue; Variant*  _Obj; };
    int asInt() { return intValue;}
    float asFloat() { return floatValue;}
    void operator=(int n) { intValue=n;type=Type::Int;}
    void operator=(float f) { floatValue=f;type=Type::Float;}
    void operator=(const char* str){ stringValue=strdup(str);type=Type::String;}
    Variant();
};

#endif
