#ifndef HLSL_PREPROCESSOR_H
#define HLSL_PREPROCESSOR_H

#include "Engine.h"
#include "CodeWriter.h"

namespace M4
{
    struct Logger;
    struct Allocator;

class HLSLPreprocessor
{
public:
    static const int s_maxPathLength = 260;

    HLSLPreprocessor(Logger* logger, Allocator* allocator, FileReadCallback fileReadCallback, const char* fileName);

    bool Generate();
    const char* GetResult(size_t& outLength) const;

private:
    struct FileReadContext
    {
        char m_fileName[s_maxPathLength];
        int         m_lineNumber;
        const char* m_readPtr;
        const char* m_buffer;
        const char* m_bufferEnd;
    };

    void OpenFile(const char* fileName);
    void CloseCurrentFile();

    void Error(const char* format, ...) const;

    Logger*                 m_logger;
    Allocator*              m_allocator;
    CodeWriter              m_writer;
    FileReadCallback        m_fileReadCallback;

    FileReadContext*        m_currentContext;
    Array<FileReadContext>  m_openFileContexts;
    Array<std::string>      m_filesAlreadyOpened;
};

}

#endif
