//=============================================================================
//
// Render/GLSLGenerator.h
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#ifndef GLSL_GENERATOR_H
#define GLSL_GENERATOR_H

#include "CodeWriter.h"
#include "HLSLTree.h"

namespace M4
{

class GLSLGenerator
{

public:
    enum Target
    {
        Target_VertexShader,
        Target_FragmentShader,
        Target_ComputeShader,
    };

    enum Version
    {
        Version_110, // OpenGL 2.0
        Version_140, // OpenGL 3.1
        Version_150, // OpenGL 3.2
        Version_430,
        Version_100_ES, // OpenGL ES 2.0
        Version_300_ES, // OpenGL ES 3.0
    };

    enum Flags
    {
        Flag_FlipPositionOutput = 1 << 0,
        Flag_PackMatrixRowMajor = 1 << 1,
        Flag_LowerMatrixMultiplication = 1 << 2,
    };

    struct Options
    {
        unsigned int flags;
        const char* constantBufferPrefix;

        Options()
        {
            flags = 0;
            constantBufferPrefix = "cb_";
        }
    };

    GLSLGenerator(Logger* logger);
    
    bool Generate(HLSLTree* tree, Target target, Version versiom, const char* entryName, const Options& options = Options(), const char* customHeader = NULL);
    const char* GetResult() const;

    void SetConstantBufferBindSlots(const char* const* bindSlotNames, int numBindSlots)
    {
        m_constantBufferBindSlots = bindSlotNames;
        m_numConstantBufferBindSlots = numBindSlots;
    }
    void SetTextureBindSlots(const char* const* bindSlotNames, int numBindSlots)
    {
        m_textureBindSlots = bindSlotNames;
        m_numTextureBindSlots = numBindSlots;
    }
    void SetRWTextureBindSlots(const char* const* bindSlotNames, int numBindSlots)
    {
        m_RWTextureBindSlots = bindSlotNames;
        m_numRWTextureBindSlots = numBindSlots;
    }
private:

    enum AttributeModifier
    {
        AttributeModifier_In,
        AttributeModifier_Out,
    };

    void OutputExpressionList(HLSLExpression* expression, HLSLArgument* argument = NULL);
    void OutputExpression(HLSLExpression* expression, const HLSLType* dstType = NULL);
    void OutputFunctionCall(const HLSLExpression* expression);
    void OutputArguments(HLSLArgument* argument);
    
    /**
     * If the statements are part of a function, then returnType can be used to specify the type
     * that a return statement is expected to produce so that correct casts will be generated.
     */
    void OutputStatements(int indent, HLSLStatement* statement, const HLSLType* returnType = NULL);
    void OutputStatementAttributes(int indent, HLSLAttribute* attribute);

    void OutputAttribute(const HLSLType& type, const char* semantic, AttributeModifier modifier);
    void OutputAttributes(HLSLFunction* entryFunction);
    void OutputEntryCaller(HLSLFunction* entryFunction);
    void OutputDeclaration(HLSLDeclaration* declaration, bool uniform);
	void OutputDeclarationType( const HLSLType& type );
	void OutputDeclarationBody( const HLSLType& type, const char* name );
    void OutputDeclaration(const HLSLType& type, const char* name);
    void OutputQualifier(const char* registerName, const HLSLType& type);
    void OutputCast(const HLSLType& type);

    void OutputSetOutAttribute(const char* semantic, const char* resultName);

    void LayoutBuffer(HLSLBuffer* buffer, unsigned int& offset);
    void LayoutBuffer(const HLSLType& type, unsigned int& offset);
    void LayoutBufferElement(const HLSLType& type, unsigned int& offset);
    void LayoutBufferAlign(const HLSLType& type, unsigned int& offset);

    HLSLBuffer* GetBufferAccessExpression(HLSLExpression* expression);
    void OutputBufferAccessExpression(HLSLBuffer* buffer, HLSLExpression* expression, const HLSLType& type, unsigned int postOffset);
    unsigned int OutputBufferAccessIndex(HLSLExpression* expression, unsigned int postOffset);

    void OutputBuffer(int indent, HLSLBuffer* buffer);

    HLSLFunction* FindFunction(HLSLRoot* root, const char* name);
    HLSLStruct* FindStruct(HLSLRoot* root, const char* name);

    void Error(const char* format, ...) const;

    /** GLSL contains some reserved words that don't exist in HLSL. This function will
     * sanitize those names. */
    const char* GetSafeIdentifierName(const char* name) const;

    /** Generates a name of the format "base+n" where n is an integer such that the name
     * isn't used in the syntax tree. */
    bool ChooseUniqueName(const char* base, char* dst, int dstLength) const;

    const char* GetBuiltInSemantic(const char* semantic, AttributeModifier modifier, int* outputIndex = 0);
    const char* GetAttribQualifier(AttributeModifier modifier);
    int MapRegisterNameToIndex(const char* registerName, HLSLRegisterType registerType) const;
private:

    static const int    s_numReservedWords = 7;
    static const char*  s_reservedWord[s_numReservedWords];

    CodeWriter          m_writer;
    Logger*             m_logger;

    HLSLTree*           m_tree;
    const char*         m_entryName;
    Target              m_target;
    Version             m_version;
    bool                m_versionLegacy;
    Options             m_options;

    bool                m_outputPosition;
    int                 m_outputTargets;

    const char*         m_outAttribPrefix;
    const char*         m_inAttribPrefix;

    const char* const*    m_constantBufferBindSlots;
    int             m_numConstantBufferBindSlots;
    const char* const*    m_textureBindSlots;
    int             m_numTextureBindSlots;
    const char* const*    m_RWTextureBindSlots;
    int             m_numRWTextureBindSlots;

    char                m_matrixRowFunction[64];
    char                m_matrixCtorFunction[64];
    char                m_matrixMulFunction[64];
    char                m_clipFunction[64];
    char                m_scalarSwizzle2Function[64];
    char                m_scalarSwizzle3Function[64];
    char                m_scalarSwizzle4Function[64];
    char                m_sinCosFunction[64];
	char                m_bvecTernary[ 64 ];

    mutable bool                m_error;

    char                m_reservedWord[s_numReservedWords][64];

};

}

#endif