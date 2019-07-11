//--------------------------------------------------------------------------------------
// File: Tutorial022.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
Texture2D tex : register(t0);
Texture2D psheightmap : register(t1);
Texture2D pswaterheightmap : register(t2);
SamplerState samLinear : register(s0);
SamplerState samLinear1 : register(s1);
SamplerState samLinear2 : register(s2);

//*****************************************************************
// Constant Buffer for model sky
//*****************************************************************
cbuffer VS_CONSTANT_BUFFER : register(b0)
	{
	float mx;
	float my;
	float scale;
	float trans;
	float div_tex_x;	//dividing of the texture coordinates in x
	float div_tex_y;	//dividing of the texture coordinates in x
	float slice_x;		//which if the 4x4 images
	float slice_y;		//which if the 4x4 images
	
	matrix world;
	matrix view;
	matrix projection;
	float4 campos;
	};

//*****************************************************************
// Terrain Constant Buffer
//*****************************************************************

cbuffer CONSTANT_MATRIX_BUFFER : register(b1)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 offsets;

	float4 lightPos; //<-- always use float4 instead of float3
	float4 camPos; //<-- float3 will bite you in the butt!
};
//*****************************************************************
// Simple Vertex
//*****************************************************************
struct SimpleVertex
	{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
	};
//*****************************************************************
// PS_INPUT
//*****************************************************************
struct PS_INPUT
	{
	float4 Pos : SV_POSITION;
	float3 WorldPos : POSITION1;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
	};

//*****************************************************************
// PS_INPUT Height Map
//*****************************************************************

struct PS_INPUT_HM
{
	float4 Pos : SV_POSITION;
	float3 ViewPos : POSITION1;
	float2 Tex : TEXCOORD0;
};

//*****************************************************************
// Vertex Shader
//*****************************************************************
PS_INPUT VShader(SimpleVertex input)
	{
	PS_INPUT output;
	float4 pos = input.Pos;

	pos = mul(world, pos);	
	output.WorldPos = pos.xyz;
	pos = mul(view, pos);
	pos = mul(projection, pos);	
	
	matrix w = world;
	w._14 = 0;
	w._24 = 0;
	w._34 = 0;

	float4 norm;
	norm.xyz = input.Norm;
	norm.w = 1;
	norm = mul(norm,w);
	output.Norm = normalize(norm.xyz);

	output.Pos = pos;
	output.Tex = input.Tex;
	return output;
	}
//*****************************************************************\
// Vertex Shader Height Map
//*****************************************************************
	PS_INPUT_HM VShaderHeight(SimpleVertex input)
	{
		PS_INPUT_HM output;
		float4 pos = input.Pos;

		pos = mul(World, pos);

		float4 color = psheightmap.SampleLevel(samLinear, input.Tex+camPos.xy, 0);
		pos.y += color.r * 36.5; // heightmapping!!!!

		pos = mul(View, pos);

		output.ViewPos = pos.xyz;

		pos = mul(Projection, pos);

		output.Pos = pos;
		output.Tex = input.Tex;
		return output;
	}
	//*****************************************************************
	//VSWATER SHADER
	//*****************************************************************
	PS_INPUT_HM VShaderWaterHeight(SimpleVertex input)
	{
		PS_INPUT_HM output;
		float4 pos = input.Pos;

		pos = mul(World, pos);

		float4 color1 = psheightmap.SampleLevel(samLinear, input.Tex + float2(camPos.x, camPos.y), 0);
		float4 color2 = pswaterheightmap.SampleLevel(samLinear, input.Tex + float2(offsets.z, offsets.w), 0);

		float4 color = color1 + color2;
		pos.y += color.r * 6.5; // heightmapping!!!!


		pos = mul(View, pos);

		output.ViewPos = pos.xyz;

		pos = mul(Projection, pos);

		output.Pos = pos;
		output.Tex = input.Tex;
		return output;
	}

//*****************************************************************
// Pixel Shader
//*****************************************************************


//normal pixel shader
float4 PS(PS_INPUT input) : SV_Target
{
		float4 color = tex.Sample(samLinear, input.Tex);

		float3 lightposition = float3(1000, 1000, 0);
		float3 normal = normalize(input.Norm);

		//diffuse light:
		float3 lightdirection = lightposition - input.WorldPos;
		lightdirection = normalize(lightdirection);
		float diffuselight = dot(normal, lightdirection);
		diffuselight = saturate(diffuselight);//saturate(x) ... if(x<0)x=0;else if(x>1) x=1;

		//specular light:
		float3 r = reflect(lightdirection, normal);
		float3 camdir = campos - input.WorldPos;
		camdir = normalize(camdir);
		float specular = dot(normalize(r), camdir);
		specular = saturate(specular);
		specular = pow(specular, 8);

		//light calculation:
		diffuselight = diffuselight;

		float4 SpecLightColor = float4(1, 1, 1, 1);//light color

		float ambientLevel = 0.25f;
		float3 ambientColor = float3(0.0f, 0.0f, 0.125f);

		color.rgb = (color.rgb * diffuselight * (1 - ambientLevel)) + ((color.rgb + ambientColor) * ambientLevel);
		color.rgb += specular;
		color.a = 1;
		return color;
}

//shader for the sky sphere
float4 PSsky(PS_INPUT input) : SV_Target
	{
	
	float4 color = tex.Sample(samLinear, input.Tex);
	color.a = 1;
	return color;
	}

	float4 PScloud(PS_INPUT input) : SV_Target
	{
		
		float4 color = tex.Sample(samLinear, input.Tex);
		return color;
	}


	//***************************************************************************
	//PSTERRAIN
	//*****************************************************************
	float4 PSterrain(PS_INPUT_HM input) : SV_Target
	{


		
		float4 color = tex.Sample(samLinear, (input.Tex+camPos.xy)*50);
		float4 colorheight = psheightmap.Sample(samLinear, input.Tex+offsets.xy);

		float3 vpos = input.ViewPos;
		vpos.y = 0;
		float dist = length(vpos);
		dist /= 200.0;
		if (dist > 1) dist = 1;
		float4 farcolor = float4(0.9, 0.93, 0.94, 1);
		farcolor.r *= dist;
		farcolor.g *= dist;
		farcolor.b *= dist;

		color.r *= colorheight.r + 0.2;
		color.g *= colorheight.r + 0.2;
		color.b *= colorheight.r + 0.2;
		color.a = 1;

		//linear interpolation:
		float4 resultcolor = color*(1 - dist) + farcolor*dist;
		resultcolor.a = 1;
		return resultcolor;
	}

	//*****************************************************************
	//PSWATERTERRAIN
	//*****************************************************************
	float4 PSWater(PS_INPUT_HM input) : SV_Target
	{
		float4 color1 = psheightmap.SampleLevel(samLinear, input.Tex + float2(offsets.x, offsets.y), 0);
		float4 color2 = pswaterheightmap.SampleLevel(samLinear, input.Tex + float2(offsets.w, offsets.z), 0);
		float4 color = (color1 + color2) / 2;

		float3 vpos = input.ViewPos;
		vpos.y = 0;
		float dist = length(vpos);
		dist /= 200.0;
		if (dist > 1) dist = 1;
		float4 farcolor = float4(0.9, 0.93, 0.94, 1);
		farcolor.r *= dist;
		farcolor.g *= dist;
		farcolor.b *= dist;

		color.r *= 0.1;
		color.g *= 0.1;
		color.b *= 1;

		//linear interpolation:

		float4 resultcolor = color*(1 - dist) + farcolor*dist;
		resultcolor.a = 1;
		return resultcolor;
	}
