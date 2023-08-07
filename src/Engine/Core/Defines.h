#pragma once
#include <cstdint>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

constexpr u64 g_NumFrames = 3;

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

enum GenericRootParameters
{
	CameraCB,         // ConstantBuffer<CameraBuffer> MatCB : register(b0);
	MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
	InstancesSB,        // StructuredBuffer<matrix> InstancesSB : register( t0, space0 );
	//LightPropertiesCB,  // ConstantBuffer<LightProperties> LightPropertiesCB : register( b1 );
	//PointLights,        // StructuredBuffer<PointLight> PointLights : register( t0 );
	//SpotLights,         // StructuredBuffer<SpotLight> SpotLights : register( t1 );
	Textures,           // Texture2D DiffuseTexture : register( t2 );
	NumGenericRootParameters
};
