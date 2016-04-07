#include <iostream>
#include <dlfcn.h>

#include "signatures.h"

void* FindPattern(const char* pattern, void* dlHandle)
{
    void* ret = dlsym( dlHandle, pattern );

    return ret;
}

std::vector<SignatureF>* allSignatures = NULL;

SignatureSearch::SignatureSearch(void* adress, const char* signature, const char* mask, int offset){
	// lazy-init, container gets 'emptied' when initialized on compile.
	if (!allSignatures){
		allSignatures = new std::vector<SignatureF>();
	}

	SignatureF ins = { signature, mask, offset, adress };
	allSignatures->push_back(ins);
}

void SignatureSearch::Search(){
	printf("Scanning for signatures.\n");
	std::vector<SignatureF>::iterator it;
    void* dlHandle = dlopen( NULL, RTLD_LAZY );
    if( NULL != dlHandle )
    {
        for (it = allSignatures->begin(); it < allSignatures->end(); it++) {
            *((void**)it->address) = FindPattern(it->signature, dlHandle);
        }
        dlclose( dlHandle );
        printf("Signatures Found.\n");
    }
}
