#ifndef TEAIO_H
#define TEAIO_H

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

enum Errno {
    SUCCESS,
    FAILURE,
    MISSING_ARGUMENTS,
    INVALID_ARGUMENTS,
    FILE_ERROR,
    ALLOCATION_ERROR
};

struct Error {
    Errno code = Errno::SUCCESS;
    char message[256] = {0};
};

struct TeaioArgs {
    char* inputFile;
    char* outputFile;
    int verbose = 0;
};

#define TEAIO_FAILED(error) (error.code != Errno::SUCCESS)
#define TEAIO_ERROR_TYPE(error) (teaioGetErrnoString(error.code))

Errno __TeaioArgs_complete(TeaioArgs* _args);
Error __teaioOpenInput(const char* _filepath, AVFormatContext** _formatContext, AVDictionary** _options = NULL);
Error __teaioOpenOutput(const char* _filepath, AVFormatContext** _formatContext, const AVOutputFormat** _format);
int* __teaioMapStreams(AVFormatContext* _iFormatContext, AVFormatContext* _oFormatContext, Error* _error);
void teaioPrintUsage();
const char* teaioGetErrnoString(Errno _error);
void teaioFree(void* _address);
TeaioArgs teaioSplitArgs(int _argc, char** _argv, Error* _error);
Error teaioRemux(TeaioArgs* _args);

#ifdef TEAIO_IMPLEMENTATION
Errno __TeaioArgs_complete(TeaioArgs* _args) {
    if (_args->inputFile && _args->outputFile)
        return Errno::SUCCESS;
    return Errno::FAILURE;
}

Error __teaioOpenInput(const char* _filepath, AVFormatContext** _formatContext, AVDictionary** _options) {
    Error error;

    if (avformat_open_input(_formatContext, _filepath, NULL, _options) < 0) {
        error.code = Errno::FILE_ERROR;
        sprintf(error.message, "Couldn't open %s", _filepath);
        return error;
    }

    if (avformat_find_stream_info(*_formatContext, NULL) < 0) {
        error.code = Errno::FILE_ERROR;
        sprintf(error.message, "Couldn't find input stream information in %s", _filepath);
        return error;
    }

    return error;
}

Error __teaioOpenOutput(const char* _filepath, AVFormatContext** _formatContext, const AVOutputFormat** _format) {
    Error error;
    
    if (avformat_alloc_output_context2(_formatContext, NULL, NULL, _filepath) < 0) {
        error.code = Errno::ALLOCATION_ERROR;
        sprintf(error.message, "Couldn't allocate output context for %s", _filepath);
        return error;
    }

    *_format = (*_formatContext)->oformat;
    return error;
}

int* __teaioMapStreams(AVFormatContext* _iFormatContext, AVFormatContext* _oFormatContext, Error* _error) {
    int* streamMap = (int*)calloc(_iFormatContext->nb_streams, sizeof(int));
    if (!streamMap) {
        _error->code = Errno::ALLOCATION_ERROR;
        sprintf(_error->message, "Couldn't allocate stream map");
        return NULL;
    }

    int streamIndex = 0;
    for (int i = 0; i < _iFormatContext->nb_streams; i++) {
        AVStream* oStream;
        AVStream* iStream = _iFormatContext->streams[i];
        AVCodecParameters* codecParams = iStream->codecpar;
        if (codecParams->codec_type != AVMEDIA_TYPE_AUDIO &&
            codecParams->codec_type != AVMEDIA_TYPE_VIDEO &&
            codecParams->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            streamMap[i] = -1;
            continue;
        }

        streamMap[i] = streamIndex++;

        oStream = avformat_new_stream(_oFormatContext, NULL);
        if (!oStream) {
            _error->code = Errno::ALLOCATION_ERROR;
            sprintf(_error->message, "Couldn't allocate output stream");
            return streamMap;
        }

        if (avcodec_parameters_copy(oStream->codecpar, codecParams) < 0) {
            _error->code = Errno::ALLOCATION_ERROR;
            sprintf(_error->message, "Couldn't copy codec parameters");
            return streamMap;
        }

        oStream->codecpar->codec_tag = 0;
    }

    return streamMap;
}

void teaioPrintUsage() {
    printf("Usage: \n"
        "  %s -i <input> <output>\n", __argv[0]);
}

const char* teaioGetErrnoString(Errno _error) {
    switch (_error) {
        case Errno::SUCCESS:
            return "Success";
            break;
        case Errno::FAILURE:
            return "Failure";
            break;
        case Errno::MISSING_ARGUMENTS:
            return "Missing arguments";
            break;
        case Errno::INVALID_ARGUMENTS:
            return "Invalid arguments";
            break;
        case Errno::FILE_ERROR:
            return "File error";
            break;
        case Errno::ALLOCATION_ERROR:
            return "Allocation error";
            break;
        
        default:
            static char buf[64];
            sprintf(buf, "Unknown Error %d", _error);
            return buf;
    }
}

void teaioFree(void* _address) {
    free(_address);
}

// Splits CLI arguments into a TeaioArgs struct
TeaioArgs teaioSplitArgs(int _argc, char** _argv, Error* _error) {
    TeaioArgs args{};

    if (_argc < 5) {
        _error->code = Errno::MISSING_ARGUMENTS;
        sprintf(_error->message, "Not enough arguments (%d out of 4 required)", _argc-1);
        return args;
    }

    // TODO: Make more robust
    while (_argv) {
        if (strcmp(*_argv, "-i") == 0) { // Check for input
            args.inputFile = *(_argv + 1);
            _argv++;
        } else if (strcmp(*_argv, "-o") == 0) { // Check for output
            args.outputFile = *(_argv + 1);
            _argv++;
        } else if (strcmp(*_argv, "-v") == 0) // Check for verbosity
            args.verbose = 1;

        // Check if there are any remaining arguments
        if (!*(_argv + 1))
            break;
        
        _argv++;
    }

    if (__TeaioArgs_complete(&args) != Errno::SUCCESS) {
        _error->code = Errno::MISSING_ARGUMENTS;
        sprintf(_error->message, "Not enough arguments");
    }

    return args;
}

Error teaioRemux(TeaioArgs* _args) {
    AVPacket*             packet              = NULL;
    AVFormatContext*      iFormatContext      = NULL;
    AVFormatContext*      oFormatContext      = NULL;
    const AVOutputFormat* oFormat             = NULL;
    int*                  streamMap           = NULL;

    Error                 error;
    
    if (!(packet = av_packet_alloc())) {
        error.code = Errno::ALLOCATION_ERROR;
        sprintf(error.message, "Couldn't allocate data packet");
        return error;
    }

    error = __teaioOpenInput(_args->inputFile, &iFormatContext);
    if (TEAIO_FAILED(error))
        goto free;

    if (_args->verbose)
        av_dump_format(iFormatContext, 0, _args->inputFile, 0);

    error = __teaioOpenOutput(_args->inputFile, &oFormatContext, &oFormat);
    if (TEAIO_FAILED(error))
        goto free;

    streamMap = __teaioMapStreams(iFormatContext, oFormatContext, &error);
    if (TEAIO_FAILED(error))
        goto free;

    if (_args->verbose)
        av_dump_format(oFormatContext, 0, _args->outputFile, 1);

    if (!(oFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&oFormatContext->pb, _args->outputFile, AVIO_FLAG_WRITE) < 0) {
            error.code = Errno::FILE_ERROR;
            sprintf(error.message, "Couldn't open output for %s", _args->outputFile);
            goto free;
        }
    }

    if (avformat_write_header(oFormatContext, NULL) < 0) {
        error.code = Errno::FILE_ERROR;
        sprintf(error.message, "Couldn't write header for %s", _args->outputFile);
        goto free;
    }

    for (;;) {
        AVStream *iStream, *oStream;

        if (av_read_frame(iFormatContext, packet) < 0)
            break;

        iStream = iFormatContext->streams[packet->stream_index];
        if (packet->stream_index >= iFormatContext->nb_streams || streamMap[packet->stream_index] < 0) {
            av_packet_unref(packet);
            continue;
        }

        packet->stream_index = streamMap[packet->stream_index];
        oStream = oFormatContext->streams[packet->stream_index];

        // Copy packet
        av_packet_rescale_ts(packet, iStream->time_base, oStream->time_base);

        if (av_write_frame(oFormatContext, packet) < 0) {
            fprintf(stderr, "Couldn't remux packet\n");
            break;
        }
    }

    if (av_write_trailer(oFormatContext) != 0) {
        error.code = Errno::FILE_ERROR;
        sprintf(error.message, "Couldn't write trailer for %s", _args->outputFile);
        goto free;
    }
    
free:
    av_packet_free(&packet);
    avformat_close_input(&iFormatContext);

    if (oFormatContext && !(oFormat->flags & AVFMT_NOFILE))
        avio_closep(&oFormatContext->pb);

    avformat_free_context(oFormatContext);
    teaioFree(streamMap);
    return error;
}

#endif // TEAIO_IMPLEMENTATION
#endif // TEAIO_H