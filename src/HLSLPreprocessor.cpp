#include "HLSLPreprocessor.h"

#include <stdlib.h>
#include <string.h>

namespace M4
{

HLSLPreprocessor::HLSLPreprocessor(Logger* logger, Allocator* allocator, FileReadCallback fileReadCallback, const char* fileName)
    : m_openFileContexts(allocator)
    , m_filesAlreadyOpened(allocator)
{
    m_logger            = logger;
    m_allocator         = allocator;
    m_fileReadCallback  = fileReadCallback;

    OpenFile(fileName);
}

bool HLSLPreprocessor::Generate()
{
    while (m_openFileContexts.GetSize() > 0)
    {
        const char* currentWritePtr = m_currentContext->m_readPtr;
        while (m_currentContext->m_readPtr < m_currentContext->m_bufferEnd)
        {
            if ((m_currentContext->m_bufferEnd - m_currentContext->m_readPtr) >= 8 && strncmp(m_currentContext->m_readPtr, "#include", 8) == 0 && isspace(m_currentContext->m_readPtr[8]))
            {
                m_writer.Write("%.*s", m_currentContext->m_readPtr - currentWritePtr, currentWritePtr);
                m_currentContext->m_readPtr += 8;

                while (m_currentContext->m_readPtr < m_currentContext->m_bufferEnd && isspace(m_currentContext->m_readPtr[0]))
                {
                    char c = m_currentContext->m_readPtr[0];
                    ++m_currentContext->m_readPtr;
                    if (c == '\n')
                    {
                        Error("Error! Expected \" after #include in file %s line %d", m_currentContext->m_fileName, m_currentContext->m_lineNumber);
                        return false;
                    }
                }

                if (m_currentContext->m_readPtr >= m_currentContext->m_bufferEnd)
                {
                    Error("Error! Unexpected end of file found in file %s line %d", m_currentContext->m_fileName, m_currentContext->m_lineNumber);
                    return false;
                }

                if (m_currentContext->m_readPtr[0] != '"')
                {
                    Error("Syntax error: expected '\"' after line number near #line");
                    return false;
                }

                ++m_currentContext->m_readPtr;

                int i = 0;
                char fileName[s_maxPathLength];
                while (i + 1 < s_maxPathLength && m_currentContext->m_readPtr < m_currentContext->m_bufferEnd && m_currentContext->m_readPtr[0] != '"')
                {
                    if (m_currentContext->m_readPtr[0] == '\n')
                    {
                        Error("Syntax error: expected '\"' before end of line near #line");
                        return false;
                    }

                    fileName[i] = *m_currentContext->m_readPtr;
                    ++m_currentContext->m_readPtr;
                    ++i;
                }

                fileName[i] = 0;
                while (m_currentContext->m_readPtr < m_currentContext->m_bufferEnd && *(m_currentContext->m_readPtr) != '\n')
                    ++m_currentContext->m_readPtr;

                m_currentContext->m_readPtr++;

                OpenFile(fileName);
                currentWritePtr = m_currentContext->m_readPtr;
            }
            else
            {
                if (m_currentContext->m_readPtr[0] == '\n')
                {
                    m_writer.Write("%.*s", m_currentContext->m_readPtr - currentWritePtr, currentWritePtr);
                    ++m_currentContext->m_lineNumber;
                    currentWritePtr = m_currentContext->m_readPtr;
                }
                m_currentContext->m_readPtr++;
            }
        }
        size_t s = strlen(currentWritePtr);
        m_writer.Write("%.*s", m_currentContext->m_readPtr - currentWritePtr, currentWritePtr); m_writer.EndLine();
        CloseCurrentFile();
        currentWritePtr = m_currentContext->m_readPtr;
    }
    return true;
}

const char* HLSLPreprocessor::GetResult(size_t& outLength) const
{
    outLength = m_writer.GetResultLength();
    return m_writer.GetResult();
}


void HLSLPreprocessor::OpenFile(const char* fileName)
{
    for (int i = 0; i < m_filesAlreadyOpened.GetSize(); ++i)
    {
        if(String_EqualNoCase(fileName, m_filesAlreadyOpened[i].c_str()))
            return;
    }

    m_currentContext = &m_openFileContexts.PushBackNew();
    strcpy(m_currentContext->m_fileName, fileName);
    m_currentContext->m_lineNumber = 1;
    m_currentContext->m_buffer = m_fileReadCallback(fileName);
    m_currentContext->m_bufferEnd = m_currentContext->m_buffer + strlen(m_currentContext->m_buffer);
    m_currentContext->m_readPtr = m_currentContext->m_buffer;

    std::string& path = m_filesAlreadyOpened.PushBackNew();
    path = fileName;

    m_writer.BeginLine(0, m_currentContext->m_fileName, m_currentContext->m_lineNumber);
}

void HLSLPreprocessor::CloseCurrentFile()
{
    if (m_openFileContexts.GetSize() == 0)
        return;

    m_allocator->Delete(m_allocator->m_userData, (void*)m_currentContext->m_buffer);
    m_openFileContexts.PopBack();

    if (m_openFileContexts.GetSize() == 0)
        return;

    m_currentContext = &m_openFileContexts[m_openFileContexts.GetSize() - 1];
    
    m_writer.BeginLine(0, m_currentContext->m_fileName, m_currentContext->m_lineNumber);
}

void HLSLPreprocessor::Error(const char* format, ...) const
{
    va_list args;
    va_start(args, format);
    m_logger->LogErrorArgList(m_logger, format, args);
    va_end(args);
}

}
