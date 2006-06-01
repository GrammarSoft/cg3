#include "stdafx.h"

void Test_HashSave() {
    struct hashtable *wookie;
    char buffer[10000];
    
    for (int i=0, j=5000;i<5000;i++, j--) {
        hashtable_insert(wookie, i, j);
    }
    
    FILE *f = fopen("/tmp/saved_hash.tmp", "wb");
//    outfile.write(&wookie, sizeof(wookie));
}
