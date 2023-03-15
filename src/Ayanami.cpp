#include <cstdio>

extern "C" {
    #define TEAIO_IMPLEMENTATION
    #include "Teaio.h"
}

int main(int argc, char** argv) {
    Error error;
    TeaioArgs args = teaioSplitArgs(argc, argv, &error);

    if (TEAIO_FAILED(error)){
        printf("[TEAIO Error %d (%s)] %s\n", error.code, teaioGetErrnoString(error.code), error.message);
        teaioPrintUsage();
        return 1;
    }

    error = teaioRemux(&args);
    if (TEAIO_FAILED(error)){
        printf("[TEAIO Error %d (%s)] %s\n", error.code, teaioGetErrnoString(error.code), error.message);
        return 1;
    }

    return 0;
}