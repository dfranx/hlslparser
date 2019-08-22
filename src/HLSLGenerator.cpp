//=============================================================================
//
// Render/HLSLGenerator.cpp
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

//#include "Engine/String.h"
//#include "Engine/Log.h"
#include "Engine.h"

#include "HLSLGenerator.h"
#include "HLSLParser.h"
#include "HLSLTree.h"

namespace M4
{

static const char* GetBaseTypeName(const HLSLBaseType& type, const char* userDefined = NULL)
{
    switch (type)
    {
    case HLSLBaseType_Void:         return "void";
    case HLSLBaseType_Float:        return "float";
    case HLSLBaseType_Float2:       return "float2";
    case HLSLBaseType_Float3:       return "float3";
    case HLSLBaseType_Float4:       return "float4";
    case HLSLBaseType_Float2x2:     return "float2x2";
    case HLSLBaseType_Float3x3:     return "float3x3";
    case HLSLBaseType_Float4x4:     return "float4x4";
    case HLSLBaseType_Float4x3:     return "float4x3";
    case HLSLBaseType_Float4x2:     return "float4x2";
    case HLSLBaseType_Half:         return "float";
    case HLSLBaseType_Half2:        return "float2";
    case HLSLBaseType_Half3:        return "float3";
    case HLSLBaseType_Half4:        return "float4";
    case HLSLBaseType_Half2x2:      return "float2x2";
    case HLSLBaseType_Half3x3:      return "float3x3";
    case HLSLBaseType_Half4x4:      return "float4x4";
    case HLSLBaseType_Half4x3:      return "float4x3";
    case HLSLBaseType_Half4x2:      return "float4x2";
    case HLSLBaseType_Bool:         return "bool";
    case HLSLBaseType_Bool2:        return "bool2";
    case HLSLBaseType_Bool3:        return "bool3";
    case HLSLBaseType_Bool4:        return "bool4";
    case HLSLBaseType_Int:          return "int";
    case HLSLBaseType_Int2:         return "int2";
    case HLSLBaseType_Int3:         return "int3";
    case HLSLBaseType_Int4:         return "int4";
    case HLSLBaseType_Uint:         return "uint";
    case HLSLBaseType_Uint2:        return "uint2";
    case HLSLBaseType_Uint3:        return "uint3";
    case HLSLBaseType_Uint4:        return "uint4";
    case HLSLBaseType_Texture1D:    return "Texture1D";
    case HLSLBaseType_Texture2D:    return "Texture2D";
    case HLSLBaseType_Texture3D:    return "Texture3D";
    case HLSLBaseType_TextureCube:  return "TextureCube";
    case HLSLBaseType_TextureCubeArray: return "TextureCubeArray";
    case HLSLBaseType_Texture2DMS:      return "Texture2DMS";
    case HLSLBaseType_Texture1DArray:   return "Texture1DArray";
    case HLSLBaseType_Texture2DArray:   return "Texture2DArray";
    case HLSLBaseType_Texture2DMSArray: return "Texture2DMSArray";
    case HLSLBaseType_RWTexture1D: return "RWTexture1D";
    case HLSLBaseType_RWTexture2D: return "RWTexture2D";
    case HLSLBaseType_RWTexture3D: return "RWTexture3D";
    case HLSLBaseType_UserDefined:      return userDefined;
    default: return "<unknown type>";
    }
}

static const char* GetTypeName(const HLSLType& type)
{
    return GetBaseTypeName(type.baseType, type.typeName);
}

static int GetFunctionArguments(HLSLFunctionCall* functionCall, HLSLExpression* expression[], int maxArguments)
{
    HLSLExpression* argument = functionCall->argument;
    int numArguments = 0;
    while (argument != NULL)
    {
        if (numArguments < maxArguments)
        {
            expression[numArguments] = argument;
        }
        argument = argument->nextExpression;
        ++numArguments;
    }
    return numArguments;
}

HLSLGenerator::HLSLGenerator(Logger* logger)
{
    m_logger                        = logger;
    m_tree                          = NULL;
    m_entryName                     = NULL;
    m_legacy                        = false;
    m_target                        = Target_VertexShader;
    m_isInsideBuffer                = false;
    m_tex2DFunction[0]              = 0;
    m_tex2DLodFunction[0]           = 0;
    m_tex2DBiasFunction[0]          = 0;
    m_tex2DGradFunction[0]          = 0;
    m_tex2DGatherFunction[0]        = 0;
    m_tex2DSizeFunction[0]          = 0;
    m_tex2DFetchFunction[0]         = 0;
    m_tex2DCmpFunction[0]           = 0;
    m_tex2DMSFetchFunction[0]       = 0;
    m_tex3DFunction[0]              = 0;
    m_tex3DLodFunction[0]           = 0;
    m_tex3DBiasFunction[0]          = 0;
    m_texCubeFunction[0]            = 0;
    m_texCubeLodFunction[0]         = 0;
    m_texCubeBiasFunction[0]        = 0;
    m_samplerPostfix                = "_sampler";
    m_texturePostfix                = "_texture";
    m_constantBufferBindSlots       = NULL;
    m_numConstantBufferBindSlots    = 0;
    m_textureBindSlots              = NULL;
    m_numTextureBindSlots           = 0;
    m_RWTextureBindSlots            = NULL;
    m_numRWTextureBindSlots         = 0;
}


// @@ We need a better way of doing semantic replacement:
// - Look at the function being generated.
// - Return semantic, semantics associated to fields of the return structure, or output arguments, or fields of structures associated to output arguments -> output semantic replacement.
// - Semantics associated input arguments or fields of the input arguments -> input semantic replacement.
static const char * TranslateSemantic(const char* semantic, bool output, HLSLGenerator::Target target)
{
    if (target == HLSLGenerator::Target_VertexShader)
    {
        if (output) 
        {
            if (String_Equal("POSITION", semantic))     return "SV_Position";
        }
        else {
            if (String_Equal("INSTANCE_ID", semantic))  return "SV_InstanceID";
        }
    }
    else if (target == HLSLGenerator::Target_PixelShader)
    {
        if (output)
        {
            if (String_Equal("DEPTH", semantic))      return "SV_Depth";
            if (String_Equal("COLOR", semantic))      return "SV_Target";
            if (String_Equal("COLOR0", semantic))     return "SV_Target0";
            if (String_Equal("COLOR0_1", semantic))   return "SV_Target1";
            if (String_Equal("COLOR1", semantic))     return "SV_Target1";
            if (String_Equal("COLOR2", semantic))     return "SV_Target2";
            if (String_Equal("COLOR3", semantic))     return "SV_Target3";
        }
        else
        {
            if (String_Equal("VPOS", semantic))       return "SV_Position";
            if (String_Equal("VFACE", semantic))      return "SV_IsFrontFace";    // bool   @@ Should we do type replacement too?
        }
    }
    return NULL;
}

bool HLSLGenerator::Generate(HLSLTree* tree, Target target, const char* entryName, bool legacy, const char* customHeader)
{
    m_tree      = tree;
    m_entryName = entryName;
    m_target    = target;
    m_legacy    = legacy;
    m_isInsideBuffer = false;

    m_writer.Reset();

    // @@ Should we generate an entirely new copy of the tree so that we can modify it in place?
    if (!legacy)
    {
        HLSLFunction * function = tree->FindFunction(entryName);

        // Handle return value semantics
        if (function->semantic != NULL) {
            function->sv_semantic = TranslateSemantic(function->semantic, /*output=*/true, target);
        }
        if (function->returnType.baseType == HLSLBaseType_UserDefined) {
            HLSLStruct * s = tree->FindGlobalStruct(function->returnType.typeName);

			HLSLStructField * sv_fields = NULL;

			HLSLStructField * lastField = NULL;
            HLSLStructField * field = s->field;
            while (field) {
				HLSLStructField * nextField = field->nextField;

                if (field->semantic) {
					field->hidden = false;
                    field->sv_semantic = TranslateSemantic(field->semantic, /*output=*/true, target);

					// Fields with SV semantics are stored at the end to avoid linkage problems.
					if (field->sv_semantic != NULL) {
						// Unlink from last.
						if (lastField != NULL) lastField->nextField = nextField;
						else s->field = nextField;

						// Add to sv_fields.
						field->nextField = sv_fields;
						sv_fields = field;
					}
                }

				if (field != sv_fields) lastField = field;
                field = nextField;
            }

			// Append SV fields at the end.
			if (sv_fields != NULL) {
				if (lastField == NULL) {
					s->field = sv_fields;
				}
				else {
					ASSERT(lastField->nextField == NULL);
					lastField->nextField = sv_fields;
				}
			}
        }

        // Handle argument semantics.
        // @@ It would be nice to flag arguments that are used by the program and skip or hide the unused ones.
        HLSLArgument * argument = function->argument;
        while (argument) {
            bool output = argument->modifier == HLSLArgumentModifier_Out;
            if (argument->semantic) {
                argument->sv_semantic = TranslateSemantic(argument->semantic, output, target); 
            }

            if (argument->type.baseType == HLSLBaseType_UserDefined) {
                HLSLStruct * s = tree->FindGlobalStruct(argument->type.typeName);

                HLSLStructField * field = s->field;
                while (field) {
                    if (field->semantic) {
						field->hidden = false;

						if (target == Target_PixelShader && !output && String_EqualNoCase(field->semantic, "POSITION")) {
							ASSERT(String_EqualNoCase(field->sv_semantic, "SV_Position"));
							field->hidden = true;
						}

                        field->sv_semantic = TranslateSemantic(field->semantic, output, target);
                    }

                    field = field->nextField;
                }
            }

            argument = argument->nextArgument;
        }
    }

    ChooseUniqueName("TextureSample",               m_tex2DFunction,            sizeof(m_tex2DFunction));
    ChooseUniqueName("TextureSampleLod",            m_tex2DLodFunction,         sizeof(m_tex2DLodFunction));
    ChooseUniqueName("tex2Dgather",                 m_tex2DGatherFunction,      sizeof(m_tex2DGatherFunction));
    ChooseUniqueName("tex2Dsize",                   m_tex2DSizeFunction,        sizeof(m_tex2DSizeFunction));
    ChooseUniqueName("tex2Dfetch",                  m_tex2DFetchFunction,       sizeof(m_tex2DFetchFunction));
    ChooseUniqueName("tex2Dcmp",                    m_tex2DCmpFunction,         sizeof(m_tex2DCmpFunction));
    ChooseUniqueName("tex2DMSfetch",                m_tex2DMSFetchFunction,     sizeof(m_tex2DMSFetchFunction));
    ChooseUniqueName("tex2DMSsize",                 m_tex2DMSSizeFunction,      sizeof(m_tex2DMSSizeFunction));
    ChooseUniqueName("tex3D",                       m_tex3DFunction,            sizeof(m_tex3DFunction));
    ChooseUniqueName("tex3Dlod",                    m_tex3DLodFunction,         sizeof(m_tex3DLodFunction));
    ChooseUniqueName("tex3Dbias",                   m_tex3DBiasFunction,        sizeof(m_tex3DBiasFunction));
    ChooseUniqueName("tex3Dsize",                   m_tex3DSizeFunction,        sizeof(m_tex3DSizeFunction));
    ChooseUniqueName("texCUBE",                     m_texCubeFunction,          sizeof(m_texCubeFunction));
    ChooseUniqueName("texCUBElod",                  m_texCubeLodFunction,       sizeof(m_texCubeLodFunction));
    ChooseUniqueName("texCUBEbias",                 m_texCubeBiasFunction,      sizeof(m_texCubeBiasFunction));
    ChooseUniqueName("texCUBEsize",                 m_texCubeSizeFunction,      sizeof(m_texCubeSizeFunction));

    if(customHeader != NULL)
        m_writer.WriteLine(0, customHeader);

    if (!m_legacy)
    {        
        if (m_tree->GetContainsString("ImageSize"))
        {
            m_writer.WriteLine(0, "int2 ImageSize(RWTexture2D<float> img) { int2 dims; img.GetDimensions(dims.x, dims.y); return dims; }");
            m_writer.WriteLine(0, "int2 ImageSize(RWTexture2D<float2> img) { int2 dims; img.GetDimensions(dims.x, dims.y); return dims; }");
            m_writer.WriteLine(0, "int2 ImageSize(RWTexture2D<float3> img) { int2 dims; img.GetDimensions(dims.x, dims.y); return dims; }");
            m_writer.WriteLine(0, "int2 ImageSize(RWTexture2D<float4> img) { int2 dims; img.GetDimensions(dims.x, dims.y); return dims; }");
        }
        /*
        if (m_tree->GetContainsString("tex2Dgather"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float2 texCoord, int component, int2 offset=0) {", m_tex2DGatherFunction, m_textureSampler2DStruct);
            m_writer.WriteLine(1, "if(component == 0) return ts.t.GatherRed(ts.s, texCoord, offset);");
            m_writer.WriteLine(1, "if(component == 1) return ts.t.GatherGreen(ts.s, texCoord, offset);");
            m_writer.WriteLine(1, "if(component == 2) return ts.t.GatherBlue(ts.s, texCoord, offset);");
            m_writer.WriteLine(1, "return ts.t.GatherAlpha(ts.s, texCoord, offset);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex2Dsize"))
        {
            m_writer.WriteLine(0, "int2 %s(%s ts) {", m_tex2DSizeFunction, m_textureSampler2DStruct);
            m_writer.WriteLine(1, "int2 size; ts.t.GetDimensions(size.x, size.y); return size;");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex2Dfetch"))
        {
            m_writer.WriteLine(0, "int2 %s(%s ts, int3 texCoord) {", m_tex2DFetchFunction, m_textureSampler2DStruct);
            m_writer.WriteLine(1, "return ts.t.Load(texCoord.xyz);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex2Dcmp"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_tex2DCmpFunction, m_textureSampler2DShadowStruct);
            m_writer.WriteLine(1, "return ts.t.SampleCmpLevelZero(ts.s, texCoord.xy, texCoord.z);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex2DMSfetch"))
        {
            m_writer.WriteLine(0, "float4 %s(Texture2DMS<float4> t, int2 texCoord, int sample) {", m_tex2DMSFetchFunction);
            m_writer.WriteLine(1, "return t.Load(texCoord, sample);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex2DMSsize"))
        {
            m_writer.WriteLine(0, "int3 %s(Texture2DMS<float4> t) {", m_tex2DMSSizeFunction);
            m_writer.WriteLine(1, "int3 size; t.GetDimensions(size.x, size.y, size.z); return size;");   // @@ Not tested, does this return the number of samples in the third argument?
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex3D"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float3 texCoord) {", m_tex3DFunction, m_textureSampler3DStruct);
            m_writer.WriteLine(1, "return ts.t.Sample(ts.s, texCoord);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex3Dlod"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_tex3DLodFunction, m_textureSampler3DStruct);
            m_writer.WriteLine(1, "return ts.t.SampleLevel(ts.s, texCoord.xyz, texCoord.w);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex3Dbias"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_tex3DBiasFunction, m_textureSampler3DStruct);
            m_writer.WriteLine(1, "return ts.t.SampleBias(ts.s, texCoord.xyz, texCoord.w);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("tex3Dsize"))
        {
            m_writer.WriteLine(0, "int3 %s(%s ts) {", m_tex3DSizeFunction, m_textureSampler3DStruct);
            m_writer.WriteLine(1, "int3 size; ts.t.GetDimensions(size.x, size.y, size.z); return size;");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("texCUBE"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float3 texCoord) {", m_texCubeFunction, m_textureSamplerCubeStruct);
            m_writer.WriteLine(1, "return ts.t.Sample(ts.s, texCoord);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("texCUBElod"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_texCubeLodFunction, m_textureSamplerCubeStruct);
            m_writer.WriteLine(1, "return ts.t.SampleLevel(ts.s, texCoord.xyz, texCoord.w);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("texCUBEbias"))
        {
            m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_texCubeBiasFunction, m_textureSamplerCubeStruct);
            m_writer.WriteLine(1, "return ts.t.SampleBias(ts.s, texCoord.xyz, texCoord.w);");
            m_writer.WriteLine(0, "}");
        }
        if (m_tree->GetContainsString("texCUBEsize"))
        {
            m_writer.WriteLine(0, "int %s(%s ts) {", m_texCubeSizeFunction, m_textureSamplerCubeStruct);
            m_writer.WriteLine(1, "int size; ts.t.GetDimensions(size); return size;");   // @@ Not tested, does this return a single value?
            m_writer.WriteLine(0, "}");
        }*/
    }

    HLSLRoot* root = m_tree->GetRoot();
    OutputStatements(0, root->statement);

    m_tree = NULL;
    return true;
}

const char* HLSLGenerator::GetResult(size_t& outLength) const
{
    outLength = m_writer.GetResultLength();
    return m_writer.GetResult();
}

void HLSLGenerator::OutputExpressionList(HLSLExpression* expression)
{
    int numExpressions = 0;
    while (expression != NULL)
    {
        if (numExpressions > 0)
        {
            m_writer.Write(", ");
        }
        OutputExpression(expression);
        expression = expression->nextExpression;
        ++numExpressions;
    }
}

void HLSLGenerator::OutputExpression(HLSLExpression* expression)
{
    if (expression->nodeType == HLSLNodeType_IdentifierExpression)
    {
        HLSLIdentifierExpression* identifierExpression = static_cast<HLSLIdentifierExpression*>(expression);
        const char* name = identifierExpression->name;
        if (!m_legacy && IsReadTextureType(identifierExpression->expressionType)/* && identifierExpression->global*/)
        {
            const char* samplerType = GetBaseTypeName(identifierExpression->expressionType.samplerType);

            if (identifierExpression->expressionType.baseType == HLSLBaseType_Texture2DMS ||
                identifierExpression->expressionType.baseType == HLSLBaseType_Texture2DMSArray)
                m_writer.Write("%s", name);
            else
                m_writer.Write("%s%s, %s%s", name, m_texturePostfix, name, m_samplerPostfix);
        }
        else
        {
            m_writer.Write("%s", name);
        }
    }
    else if (expression->nodeType == HLSLNodeType_CastingExpression)
    {
        HLSLCastingExpression* castingExpression = static_cast<HLSLCastingExpression*>(expression);
        m_writer.Write("(");
        OutputDeclaration(castingExpression->type, "");
        m_writer.Write(")(");
        OutputExpression(castingExpression->expression);
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_ConstructorExpression)
    {
        HLSLConstructorExpression* constructorExpression = static_cast<HLSLConstructorExpression*>(expression);
        if(IsReadTextureType(constructorExpression->type))
            m_logger->LogError("Texture type %s is not constructable", GetTypeName(constructorExpression->type));
        m_writer.Write("%s(", GetTypeName(constructorExpression->type));
        OutputExpressionList(constructorExpression->argument);
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_LiteralExpression)
    {
        HLSLLiteralExpression* literalExpression = static_cast<HLSLLiteralExpression*>(expression);
        switch (literalExpression->type)
        {
        case HLSLBaseType_Half:
        case HLSLBaseType_Float:
            {
                // Don't use printf directly so that we don't use the system locale.
                char buffer[64];
                String_FormatFloat(buffer, sizeof(buffer), literalExpression->fValue);
                m_writer.Write("%s", buffer);
            }
            break;        
        case HLSLBaseType_Int:
            m_writer.Write("%d", literalExpression->iValue);
            break;
        case HLSLBaseType_Bool:
            m_writer.Write("%s", literalExpression->bValue ? "true" : "false");
            break;
        default:
            ASSERT(0);
        }
    }
    else if (expression->nodeType == HLSLNodeType_UnaryExpression)
    {
        HLSLUnaryExpression* unaryExpression = static_cast<HLSLUnaryExpression*>(expression);
        const char* op = "?";
        bool pre = true;
        switch (unaryExpression->unaryOp)
        {
        case HLSLUnaryOp_Negative:      op = "-";  break;
        case HLSLUnaryOp_Positive:      op = "+";  break;
        case HLSLUnaryOp_Not:           op = "!";  break;
        case HLSLUnaryOp_PreIncrement:  op = "++"; break;
        case HLSLUnaryOp_PreDecrement:  op = "--"; break;
        case HLSLUnaryOp_PostIncrement: op = "++"; pre = false; break;
        case HLSLUnaryOp_PostDecrement: op = "--"; pre = false; break;
        case HLSLUnaryOp_BitNot:        op = "~";  break;
        }
        m_writer.Write("(");
        if (pre)
        {
            m_writer.Write("%s", op);
            OutputExpression(unaryExpression->expression);
        }
        else
        {
            OutputExpression(unaryExpression->expression);
            m_writer.Write("%s", op);
        }
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_BinaryExpression)
    {
        HLSLBinaryExpression* binaryExpression = static_cast<HLSLBinaryExpression*>(expression);
        m_writer.Write("(");
        OutputExpression(binaryExpression->expression1);
        const char* op = "?";
        switch (binaryExpression->binaryOp)
        {
        case HLSLBinaryOp_Add:          op = " + "; break;
        case HLSLBinaryOp_Sub:          op = " - "; break;
        case HLSLBinaryOp_Mul:          op = " * "; break;
        case HLSLBinaryOp_Div:          op = " / "; break;
        case HLSLBinaryOp_Less:         op = " < "; break;
        case HLSLBinaryOp_Greater:      op = " > "; break;
        case HLSLBinaryOp_LessEqual:    op = " <= "; break;
        case HLSLBinaryOp_GreaterEqual: op = " >= "; break;
        case HLSLBinaryOp_Equal:        op = " == "; break;
        case HLSLBinaryOp_NotEqual:     op = " != "; break;
        case HLSLBinaryOp_Assign:       op = " = "; break;
        case HLSLBinaryOp_AddAssign:    op = " += "; break;
        case HLSLBinaryOp_SubAssign:    op = " -= "; break;
        case HLSLBinaryOp_MulAssign:    op = " *= "; break;
        case HLSLBinaryOp_DivAssign:    op = " /= "; break;
        case HLSLBinaryOp_And:          op = " && "; break;
        case HLSLBinaryOp_Or:           op = " || "; break;
		case HLSLBinaryOp_BitAnd:       op = " & "; break;
        case HLSLBinaryOp_BitOr:        op = " | "; break;
        case HLSLBinaryOp_BitXor:       op = " ^ "; break;
        default:
            ASSERT(0);
        }
        m_writer.Write("%s", op);
        OutputExpression(binaryExpression->expression2);
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_ConditionalExpression)
    {
        HLSLConditionalExpression* conditionalExpression = static_cast<HLSLConditionalExpression*>(expression);
        m_writer.Write("((");
        OutputExpression(conditionalExpression->condition);
        m_writer.Write(")?(");
        OutputExpression(conditionalExpression->trueExpression);
        m_writer.Write("):(");
        OutputExpression(conditionalExpression->falseExpression);
        m_writer.Write("))");
    }
    else if (expression->nodeType == HLSLNodeType_MemberAccess)
    {
        HLSLMemberAccess* memberAccess = static_cast<HLSLMemberAccess*>(expression);
        m_writer.Write("(");
        OutputExpression(memberAccess->object);
        m_writer.Write(").%s", memberAccess->field);
    }
    else if (expression->nodeType == HLSLNodeType_ArrayAccess)
    {
        HLSLArrayAccess* arrayAccess = static_cast<HLSLArrayAccess*>(expression);
        OutputExpression(arrayAccess->array);
        m_writer.Write("[");
        OutputExpression(arrayAccess->index);
        m_writer.Write("]");
    }
    else if (expression->nodeType == HLSLNodeType_FunctionCall)
    {
        HLSLFunctionCall* functionCall = static_cast<HLSLFunctionCall*>(expression);
        const char* name = functionCall->function->name;
        if (!m_legacy)
        {
            if (String_Equal(name, "TextureSample"))
            {
                HLSLExpression* textureArgument = functionCall->argument;
                HLSLExpression* coordinateArgument = textureArgument ? textureArgument->nextExpression : NULL;
                ASSERT(coordinateArgument);
                ASSERT(textureArgument->nodeType == HLSLNodeType_IdentifierExpression);
                ASSERT(samplerArgument->nodeType == HLSLNodeType_IdentifierExpression);

                const char* textureName = ((HLSLIdentifierExpression*)textureArgument)->name;
                m_writer.Write("%s%s.Sample(%s%s, ", textureName, m_texturePostfix, textureName, m_samplerPostfix);
                OutputExpression(coordinateArgument);
                m_writer.Write(")");

                int elemCount = GetElementCount(textureArgument->expressionType.samplerType);
                if (elemCount == 1)
                    m_writer.Write(".r");
                else if (elemCount == 2)
                    m_writer.Write(".rg");
                else if (elemCount == 3)
                    m_writer.Write(".rgb");

                name = NULL;
            }
            else if (String_Equal(name, "TextureSampleLod"))
            {
                HLSLExpression* textureArgument = functionCall->argument;
                HLSLExpression* coordinateArgument = textureArgument ? textureArgument->nextExpression : NULL;
                HLSLExpression* lodArgument = coordinateArgument ? coordinateArgument->nextExpression : NULL;

                ASSERT(lodArgument);
                ASSERT(textureArgument->nodeType == HLSLNodeType_IdentifierExpression);
                ASSERT(samplerArgument->nodeType == HLSLNodeType_IdentifierExpression);

                const char* textureName = ((HLSLIdentifierExpression*)textureArgument)->name;
                m_writer.Write("%s%s.SampleLevel(%s%s, ", textureName, m_texturePostfix, textureName, m_samplerPostfix);
                OutputExpression(coordinateArgument);
                m_writer.Write(", ");
                OutputExpression(lodArgument);
                m_writer.Write(")");

                name = NULL;
            }
            else if (String_Equal(name, "TextureSampleLodOffset"))
                name = m_tex2DBiasFunction;
            else if (String_Equal(name, "TextureGather"))
                name = m_tex2DGatherFunction;
            else if (String_Equal(name, "TextureFetch"))
                name = m_tex2DFetchFunction;
            else if (String_Equal(name, "TextureSize"))
                name = m_tex2DSizeFunction;
            else if (String_Equal(name, "ImageLoad"))
            {
                HLSLExpression* textureArgument = functionCall->argument;
                HLSLExpression* coordinateArgument = textureArgument ? textureArgument->nextExpression : NULL;

                ASSERT(coordinateArgument);
                ASSERT(textureArgument->nodeType == HLSLNodeType_IdentifierExpression);

                const char* textureName = ((HLSLIdentifierExpression*)textureArgument)->name;
                m_writer.Write("%s.Load(", textureName);
                OutputExpression(coordinateArgument);
                m_writer.Write(")");

                name = NULL;
            }
            else if (String_Equal(name, "ImageStore"))
            {
                HLSLExpression* textureArgument = functionCall->argument;
                HLSLExpression* coordinateArgument = textureArgument ? textureArgument->nextExpression : NULL;
                HLSLExpression* valueArgument = coordinateArgument ? coordinateArgument->nextExpression : NULL;
                ASSERT(valueArgument);
                ASSERT(textureArgument->nodeType == HLSLNodeType_IdentifierExpression);

                const char* textureName = ((HLSLIdentifierExpression*)textureArgument)->name;
                m_writer.Write("%s[", textureName);
                OutputExpression(coordinateArgument);
                m_writer.Write("] = ");
                OutputExpression(valueArgument);

                name = NULL;
            }
            else if (String_Equal(name, "ImageSize"))
            {
                name = "ImageSize";
            }
            else if (String_Equal(name, "ImageAtomicExchange"))
                name = "imageAtomicExchange";
            else if (String_Equal(name, "ImageAtomicCompSwap"))
                name = "imageAtomicCompSwap";
            else if (String_Equal(name, "ImageAtomicAdd"))
                name = "imageAtomicAdd";
            else if (String_Equal(name, "ImageAtomicAnd"))
                name = "imageAtomicAnd";
            else if (String_Equal(name, "ImageAtomicOr"))
                name = "imageAtomicOr";
            else if (String_Equal(name, "ImageAtomicXor"))
                name = "imageAtomicXor";
            else if (String_Equal(name, "ImageAtomicMin"))
                name = "imageAtomicMin";
            else if (String_Equal(name, "ImageAtomicMax"))
                name = "imageAtomicMax";
        }

        if (name)
        {
            m_writer.Write("%s(", name);
            OutputExpressionList(functionCall->argument);
            m_writer.Write(")");
        }
    }
    else
    {
        m_writer.Write("<unknown expression>");
    }
}

void HLSLGenerator::OutputArguments(HLSLArgument* argument)
{
    int numArgs = 0;
    while (argument != NULL)
    {
        if (numArgs > 0)
        {
            m_writer.Write(", ");
        }

        switch (argument->modifier)
        {
        case HLSLArgumentModifier_In:
            m_writer.Write("in ");
            break;
        case HLSLArgumentModifier_Out:
            m_writer.Write("out ");
            break;
        case HLSLArgumentModifier_Inout:
            m_writer.Write("inout ");
            break;
        case HLSLArgumentModifier_Uniform:
            m_writer.Write("uniform ");
            break;
        default:
            break;
        }

        const char * semantic = argument->sv_semantic ? argument->sv_semantic : argument->semantic;

        OutputDeclaration(argument->type, argument->name, semantic, /*registerName=*/NULL, argument->defaultValue);
        OutputOptionalSamplerArgument(argument);
        argument = argument->nextArgument;
        ++numArgs;
    }
}

void HLSLGenerator::OutputOptionalSamplerArgument(HLSLArgument* argument)
{
    if (IsReadTextureType(argument->type))
    {
        m_writer.Write(", SamplerState %s%s", argument->name, m_samplerPostfix);
    }
}

static const char * GetAttributeName(HLSLAttributeType attributeType)
{
    if (attributeType == HLSLAttributeType_Unroll) return "unroll";
    if (attributeType == HLSLAttributeType_Branch) return "branch";
    if (attributeType == HLSLAttributeType_Flatten) return "flatten";
    return NULL;
}

void HLSLGenerator::OutputAttributes(int indent, HLSLAttribute* attribute)
{
    while (attribute != NULL)
    {
        const char * attributeName = GetAttributeName(attribute->attributeType);

        if (m_target == Target_ComputeShader && attribute->attributeType == HLSLAttributeType_NumThreads)
        {
            m_writer.BeginLine(indent, attribute->fileName, attribute->line);
            HLSLExpression* exp1 = attribute->argument;
            HLSLExpression* exp2 = exp1 ? exp1->nextExpression : NULL;
            HLSLExpression* exp3 = exp2 ? exp2->nextExpression : NULL;
            if (exp3 && !exp3->nextExpression)
            {
                m_writer.Write("[numthreads(");
                OutputExpression(exp1);
                m_writer.Write(", ");
                OutputExpression(exp2);
                m_writer.Write(", ");
                OutputExpression(exp3);
                m_writer.Write(")]");
                m_writer.EndLine();
            }
            else
            {
                //Error("Something went wrong while declaring numThreads! Expected three Expressions for dispatch dimensions");
            }
        }
        else if (attributeName != NULL)
        {
            m_writer.WriteLineTagged(indent, attribute->fileName, attribute->line, "[%s]", attributeName);
        }

        attribute = attribute->nextAttribute;
    }
}

void HLSLGenerator::OutputStatements(int indent, HLSLStatement* statement)
{
    while (statement != NULL)
    {
        if (statement->hidden) 
        {
            statement = statement->nextStatement;
            continue;
        }

        OutputAttributes(indent, statement->attributes);

        if (statement->nodeType == HLSLNodeType_Declaration)
        {
            HLSLDeclaration* declaration = static_cast<HLSLDeclaration*>(statement);
            m_writer.BeginLine(indent, declaration->fileName, declaration->line);
            OutputDeclaration(declaration);
            m_writer.EndLine(";");
        }
        else if (statement->nodeType == HLSLNodeType_Struct)
        {
            HLSLStruct* structure = static_cast<HLSLStruct*>(statement);
            m_writer.WriteLineTagged(indent, structure->fileName, structure->line, "struct %s {", structure->name);
            HLSLStructField* field = structure->field;
            while (field != NULL)
            {
                if (!field->hidden)
                {
                    m_writer.BeginLine(indent + 1, field->fileName, field->line);
                    const char * semantic = field->sv_semantic ? field->sv_semantic : field->semantic;
                    OutputDeclaration(field->type, field->name, semantic);
                    m_writer.Write(";");
                    m_writer.EndLine();
                }
                field = field->nextField;
            }
            m_writer.WriteLine(indent, "};");
        }
        else if (statement->nodeType == HLSLNodeType_Buffer)
        {
            HLSLBuffer* buffer = static_cast<HLSLBuffer*>(statement);
            HLSLDeclaration* field = buffer->field;

            if (!m_legacy)
            {
                m_writer.BeginLine(indent, buffer->fileName, buffer->line);
                m_writer.Write("struct %sType {", buffer->name);
                m_writer.EndLine();
            }

            m_isInsideBuffer = true;

            while (field != NULL)
            {
                if (!field->hidden)
                {
                    m_writer.BeginLine(indent + 1, field->fileName, field->line);
                    OutputDeclaration(field->type, field->name, /*semantic=*/NULL, /*registerName*/field->registerName, field->assignment);
                    m_writer.Write(";");
                    m_writer.EndLine();
                }
                field = (HLSLDeclaration*)field->nextDeclaration;
            }

            m_isInsideBuffer = false;

            if (!m_legacy)
            {
                m_writer.WriteLine(indent, "};");

                m_writer.BeginLine(indent, buffer->fileName, buffer->line);
                m_writer.Write("cbuffer cb_%s", buffer->name);
                OutputRegisterName(buffer->registerName, HLSLRegister_ConstantBuffer);

                m_writer.EndLine(" {");
                m_writer.WriteLine(indent + 1, "%sType %s;", buffer->name, buffer->name);
                m_writer.WriteLine(indent, "};");
            }
        }
        else if (statement->nodeType == HLSLNodeType_Function)
        {
            HLSLFunction* function = static_cast<HLSLFunction*>(statement);

            // Use an alternate name for the function which is supposed to be entry point
            // so that we can supply our own function which will be the actual entry point.
            const char* functionName   = function->name;
            const char* returnTypeName = GetTypeName(function->returnType);
            const char* samplerTypeName = GetBaseTypeName(function->returnType.samplerType);

            m_writer.BeginLine(indent, function->fileName, function->line);
            if (IsMultisampledTexture(function->returnType.baseType))
            {
                m_writer.Write("%s<%s, %d>, %s(", returnTypeName, samplerTypeName, function->returnType.sampleCount, functionName);
            }
            else if (IsReadTextureType(function->returnType))
            {
                m_writer.Write("%s %s(", returnTypeName, functionName);
            }
            else
            {
                m_writer.Write("%s %s(", returnTypeName, functionName);
            }

            OutputArguments(function->argument);

            const char * semantic = function->sv_semantic ? function->sv_semantic : function->semantic;
            if (semantic != NULL)
            {
                m_writer.Write(") : %s {", semantic);
            }
            else
            {
                m_writer.Write(") {");
            }

            m_writer.EndLine();

            OutputStatements(indent + 1, function->statement);
            m_writer.WriteLine(indent, "};");
        }
        else if (statement->nodeType == HLSLNodeType_ExpressionStatement)
        {
            HLSLExpressionStatement* expressionStatement = static_cast<HLSLExpressionStatement*>(statement);
            m_writer.BeginLine(indent, statement->fileName, statement->line);
            OutputExpression(expressionStatement->expression);
            m_writer.EndLine(";");
        }
        else if (statement->nodeType == HLSLNodeType_ReturnStatement)
        {
            HLSLReturnStatement* returnStatement = static_cast<HLSLReturnStatement*>(statement);
            if (returnStatement->expression != NULL)
            {
                m_writer.BeginLine(indent, returnStatement->fileName, returnStatement->line);
                m_writer.Write("return ");
                OutputExpression(returnStatement->expression);
                m_writer.EndLine(";");
            }
            else
            {
                m_writer.WriteLineTagged(indent, returnStatement->fileName, returnStatement->line, "return;");
            }
        }
        else if (statement->nodeType == HLSLNodeType_DiscardStatement)
        {
            HLSLDiscardStatement* discardStatement = static_cast<HLSLDiscardStatement*>(statement);
            m_writer.WriteLineTagged(indent, discardStatement->fileName, discardStatement->line, "discard;");
        }
        else if (statement->nodeType == HLSLNodeType_BreakStatement)
        {
            HLSLBreakStatement* breakStatement = static_cast<HLSLBreakStatement*>(statement);
            m_writer.WriteLineTagged(indent, breakStatement->fileName, breakStatement->line, "break;");
        }
        else if (statement->nodeType == HLSLNodeType_ContinueStatement)
        {
            HLSLContinueStatement* continueStatement = static_cast<HLSLContinueStatement*>(statement);
            m_writer.WriteLineTagged(indent, continueStatement->fileName, continueStatement->line, "continue;");
        }
        else if (statement->nodeType == HLSLNodeType_IfStatement)
        {
            HLSLIfStatement* ifStatement = static_cast<HLSLIfStatement*>(statement);
            m_writer.BeginLine(indent, ifStatement->fileName, ifStatement->line);
            m_writer.Write("if (");
            OutputExpression(ifStatement->condition);
            m_writer.Write(") {");
            m_writer.EndLine();
            OutputStatements(indent + 1, ifStatement->statement);
            m_writer.WriteLine(indent, "}");
            if (ifStatement->elseStatement != NULL)
            {
                m_writer.WriteLine(indent, "else {");
                OutputStatements(indent + 1, ifStatement->elseStatement);
                m_writer.WriteLine(indent, "}");
            }
        }
        else if (statement->nodeType == HLSLNodeType_ForStatement)
        {
            HLSLForStatement* forStatement = static_cast<HLSLForStatement*>(statement);
            m_writer.BeginLine(indent, forStatement->fileName, forStatement->line);
            m_writer.Write("for (");
            OutputDeclaration(forStatement->initialization);
            m_writer.Write("; ");
            OutputExpression(forStatement->condition);
            m_writer.Write("; ");
            OutputExpression(forStatement->increment);
            m_writer.Write(") {");
            m_writer.EndLine();
            OutputStatements(indent + 1, forStatement->statement);
            m_writer.WriteLine(indent, "}");
        }
        else if (statement->nodeType == HLSLNodeType_BlockStatement)
        {
            HLSLBlockStatement* blockStatement = static_cast<HLSLBlockStatement*>(statement);
            m_writer.WriteLineTagged(indent, blockStatement->fileName, blockStatement->line, "{");
            OutputStatements(indent + 1, blockStatement->statement);
            m_writer.WriteLine(indent, "}");
        }
        else
        {
            // Unhanded statement type.
            ASSERT(0);
        }

        statement = statement->nextStatement;
    }
}

void HLSLGenerator::OutputDeclaration(HLSLDeclaration* declaration)
{
    bool isReadTextureType = IsReadTextureType(declaration->type);

    if (!m_legacy && isReadTextureType)
    {
        const char* textureType = GetTypeName(declaration->type);
        const char* samplerType = "SamplerState";
        // @@ Handle generic sampler type.

        //if (declaration->type.baseType == HLSLBaseType_Texture2DShadow)
        //{
        //    samplerType = "SamplerComparisonState";
        //}
        if (declaration->type.baseType == HLSLBaseType_Texture2DMS)
        {
            textureType = "Texture2DMS<float4>";  // @@ Is template argument required?
            samplerType = NULL;
        }

        if (samplerType != NULL)
        {
            m_writer.Write("%s<%s> %s%s", textureType, GetBaseTypeName(declaration->type.samplerType), declaration->name, m_texturePostfix);
            OutputRegisterName(declaration->registerName, HLSLRegister_ShaderResource);
            m_writer.Write("; %s %s%s", samplerType, declaration->name, m_samplerPostfix);
            OutputRegisterName(declaration->registerName, HLSLRegister_Sampler);
        }
        else
        {
            m_writer.Write("%s %s", textureType, declaration->name);
            OutputRegisterName(declaration->registerName, HLSLRegister_ShaderResource);
        }
        return;
    }

    OutputDeclarationType(declaration->type);
    OutputDeclarationBody(declaration->type, declaration->name, declaration->semantic, declaration->registerName, declaration->assignment);
    declaration = declaration->nextDeclaration;

    while(declaration != NULL) {
        m_writer.Write(", ");
        OutputDeclarationBody(declaration->type, declaration->name, declaration->semantic, declaration->registerName, declaration->assignment);
        declaration = declaration->nextDeclaration;
    };
}

void HLSLGenerator::OutputDeclarationType(const HLSLType& type)
{
    const char* typeName = GetTypeName(type);

    if (type.flags & HLSLTypeFlag_Const)
    {
        m_writer.Write("const ");
    }
    if (type.flags & HLSLTypeFlag_Static)
    {
        m_writer.Write("static ");
    }

    // Interpolation modifiers.
    if (type.flags & HLSLTypeFlag_Centroid)
    {
        m_writer.Write("centroid ");
    }
    if (type.flags & HLSLTypeFlag_Linear)
    {
        m_writer.Write("linear ");
    }
    if (type.flags & HLSLTypeFlag_NoInterpolation)
    {
        m_writer.Write("nointerpolation ");
    }
    if (type.flags & HLSLTypeFlag_NoPerspective)
    {
        m_writer.Write("noperspective ");
    }
    if (type.flags & HLSLTypeFlag_Sample)   // @@ Only in shader model >= 4.1
    {
        m_writer.Write("sample ");
    }

    if (!m_legacy && (IsReadTextureType(type) || IsWriteTextureType(type)))
    {
        if (IsReadTextureType(type))
        {
            if (IsMultisampledTexture(type.baseType))
            {
                m_writer.Write("%s<%s, %d> ", typeName, GetBaseTypeName(type.samplerType), type.sampleCount);
            }
            else
            {
                m_writer.Write("%s<%s> ", typeName, GetBaseTypeName(type.samplerType));
            }
        }
        else if (IsWriteTextureType(type))
        {
                m_writer.Write("%s<%s> ", typeName, GetBaseTypeName(type.samplerType));
        }
    }
    else
    {
        m_writer.Write("%s ", typeName);
    }
}

void HLSLGenerator::OutputDeclarationBody(const HLSLType& type, const char* name, const char* semantic/*=NULL*/, const char* registerName/*=NULL*/, HLSLExpression * assignment/*=NULL*/)
{
    if (IsReadTextureType(type))
        m_writer.Write("%s%s", name, m_texturePostfix);
    else
        m_writer.Write("%s", name);

    if (type.array)
    {
        ASSERT(semantic == NULL);
        m_writer.Write("[");
        if (type.arraySize != NULL)
        {
            OutputExpression(type.arraySize);
        }
        m_writer.Write("]");
    }

    if (semantic != NULL) 
    {
        m_writer.Write(" : %s", semantic);
    }

    if (registerName != NULL)
    {
        if (m_isInsideBuffer)
        {
            m_writer.Write(" : packoffset(%s)", registerName);
        }
        else 
        {
            if (IsReadTextureType(type))
            {
                OutputRegisterName(registerName, HLSLRegister_ShaderResource);
            }
            else if (IsWriteTextureType(type))
            {
                OutputRegisterName(registerName, HLSLRegister_UnorderedAccess);
            }
            else
            {
                m_writer.Write(" : register(%s)", registerName);
            }
        }
    }

    if (assignment != NULL && !IsReadTextureType(type))
    {
        m_writer.Write(" = ");
        if (type.array)
        {
            m_writer.Write("{ ");
            OutputExpressionList(assignment);
            m_writer.Write(" }");
        }
        else
        {
            OutputExpression(assignment);
        }
    }
}

void HLSLGenerator::OutputRegisterName(const char* registerName, HLSLRegisterType type)
{
    if (registerName == NULL)
        return;

    int registerIndex = -1;
    if (type == HLSLRegisterType::HLSLRegister_ConstantBuffer)
    {
        for (int i = 0; i < m_numConstantBufferBindSlots; ++i)
        {
            if (String_Equal(registerName, m_constantBufferBindSlots[i]))
            {
                registerIndex = i; break;
            }
        }

        if (registerIndex == -1)
        {
            if (strncmp(registerName, "ConstantBuffer", 14) == 0)
            {
                registerIndex = atoi(registerName + 14);
            }
            else
                m_logger->LogError("Undefined register use %s", registerName);
        }

        m_writer.Write(" : register(b%d)", registerIndex);
    }
    else if (type == HLSLRegister_ShaderResource || type == HLSLRegister_Sampler)
    {
        for (int i = 0; i < m_numTextureBindSlots; ++i)
        {
            if (String_Equal(registerName, m_textureBindSlots[i]))
            {
                registerIndex = i; break;
            }
        }

        if (registerIndex == -1)
        {
            if (strncmp(registerName, "Texture", 7) == 0)
            {
                registerIndex = atoi(registerName + 7);
            }
            else
                m_logger->LogError("Undefined register use %s", registerName);
        }

        if (type == HLSLRegister_ShaderResource)
            m_writer.Write(" : register(t%d)", registerIndex);
        else
            m_writer.Write(" : register(s%d)", registerIndex);
    }
    else if (type == HLSLRegister_UnorderedAccess)
    {

    }
}

void HLSLGenerator::OutputDeclaration(const HLSLType& type, const char* name, const char* semantic/*=NULL*/, const char* registerName/*=NULL*/, HLSLExpression * assignment/*=NULL*/)
{
    OutputDeclarationType(type);
    OutputDeclarationBody(type, name, semantic, registerName, assignment);
}

bool HLSLGenerator::ChooseUniqueName(const char* base, char* dst, int dstLength) const
{
    // IC: Try without suffix first.
    String_Printf(dst, dstLength, "%s", base);
    if (!m_tree->GetContainsString(base))
    {
        return true;
    }

    for (int i = 1; i < 1024; ++i)
    {
        String_Printf(dst, dstLength, "%s%d", base, i);
        if (!m_tree->GetContainsString(dst))
        {
            return true;
        }
    }
    return false;
}

}
