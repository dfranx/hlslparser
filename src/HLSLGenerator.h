//=============================================================================
//
// Render/HLSLGenerator.h
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#ifndef HLSL_GENERATOR_H
#define HLSL_GENERATOR_H

#include "CodeWriter.h"
#include "HLSLTree.h"

namespace M4
{

struct Logger;
class  HLSLTree;
struct HLSLFunction;
struct HLSLStruct;

/**
 * This class is used to generate HLSL which is compatible with the D3D9
 * compiler (i.e. no cbuffers).
 */
class HLSLGenerator
{

public:

    enum Target
    {
        Target_VertexShader,
        Target_PixelShader,
        Target_ComputeShader,
    };

    HLSLGenerator(Logger* logger);

    bool Generate(HLSLTree* tree, Target target, const char* entryName, bool legacy, const char* customHeader = NULL);
    const char* GetResult(size_t& outLength) const;

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

    void OutputExpressionList(HLSLExpression* expression);
    void OutputExpression(HLSLExpression* expression);
    void OutputArguments(HLSLArgument* argument);
    void OutputOptionalSamplerArgument(HLSLArgument* argument);
    void OutputAttributes(int indent, HLSLAttribute* attribute);
    void OutputStatements(int indent, HLSLStatement* statement);
    void OutputDeclaration(HLSLDeclaration* declaration);
    void OutputDeclaration(const HLSLType& type, const char* name, const char* semantic = NULL, const char* registerName = NULL, HLSLExpression* defaultValue = NULL);
    void OutputDeclarationType(const HLSLType& type);
    void OutputDeclarationBody(const HLSLType& type, const char* name, const char* semantic = NULL, const char* registerName = NULL, HLSLExpression * assignment = NULL);
    void OutputRegisterName(const char* registerName, HLSLRegisterType type);

    /** Generates a name of the format "base+n" where n is an integer such that the name
     * isn't used in the syntax tree. */
    bool ChooseUniqueName(const char* base, char* dst, int dstLength) const;

private:

    CodeWriter      m_writer;
    Logger*         m_logger;

    const HLSLTree* m_tree;
    const char*     m_entryName;
    const char*     m_samplerPostfix;
    const char*     m_texturePostfix;
    Target          m_target;
    bool            m_legacy;
    bool            m_isInsideBuffer;

    const char* const*    m_constantBufferBindSlots;
    int             m_numConstantBufferBindSlots;
    const char* const*    m_textureBindSlots;
    int             m_numTextureBindSlots;
    const char* const*    m_RWTextureBindSlots;
    int             m_numRWTextureBindSlots;


    char            m_tex2DFunction[64];
    char            m_tex2DLodFunction[64];
    char            m_tex2DBiasFunction[64];
    char            m_tex2DGradFunction[64];
    char            m_tex2DGatherFunction[64];
    char            m_tex2DSizeFunction[64];
    char            m_tex2DFetchFunction[64];
    char            m_tex2DCmpFunction[64];
    char            m_tex2DMSFetchFunction[64];
    char            m_tex2DMSSizeFunction[64];
    char            m_tex3DFunction[64];
    char            m_tex3DLodFunction[64];
    char            m_tex3DBiasFunction[64];
    char            m_tex3DSizeFunction[64];
    char            m_texCubeFunction[64];
    char            m_texCubeLodFunction[64];
    char            m_texCubeBiasFunction[64];
    char            m_texCubeSizeFunction[64];
};

} // M4

#endif