#pragma once

namespace UtilsExample
{
	const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

	class ShadowPass final
	{
	public:
		void Create(int width, int height)
		{
			m_width = width;
			m_height = height;

			depth.reset(new GLTexture2D(GL_DEPTH_COMPONENT32F, GL_DEPTH, GL_FLOAT, m_width, m_height, nullptr, GL_NEAREST, GL_CLAMP_TO_BORDER, { 1.0f, 1.0f, 1.0f, 1.0f }));

			fbo.reset(new GLFramebuffer({}, depth));


#pragma region VertexShader
			const char* vertSource = // vertex shader:
				R"(
#version 460 core

// -----------  Per vertex  -----------
layout (location = 0) in vec3 aPosition;

// --------- Output Variables ---------
out gl_PerVertex { vec4 gl_Position; };
out vec4 position;

// ------------- Uniform --------------
layout (location = 0) uniform mat4 uLightSpaceMatrix;
layout (location = 1) uniform mat4 uWorldMatrix;

void main()
{	
	gl_Position = uLightSpaceMatrix * uWorldMatrix * vec4(aPosition, 1.0);
	position = gl_Position;
}
)";
#pragma endregion

#pragma region FragmentShader
			const char* fragSource = // fragment shader:
				R"(
#version 460 core

in vec4 position;

layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = vec4(position.w, position.w, position.w, position.w);
}
)";
#pragma endregion

			program = std::make_shared<GLProgramPipeline>(vertSource, fragSource);
		}

		void Bind()
		{
			constexpr auto depthClearVal = 1.0f;
			fbo->ClearFramebuffer(GL_DEPTH, 0, &depthClearVal);
			fbo->Bind();
			Renderer::SetViewport(0, 0, m_width, m_height);
			program->Bind();
		}


		GLFramebufferRef fbo = nullptr;
		GLTexture2DRef depth = nullptr;

		GLProgramPipelineRef program = nullptr;

	private:
		int m_width = 0;
		int m_height = 0;
	};

	class GBuffer final
	{
	public:
		GBuffer() = delete;
		GBuffer(int width, int height);
		~GBuffer();

		void Resize(int width, int height);

		void BindForWriting();
		void BindForReading();

		GLProgramPipelineRef GetProgram();

	private:
		GLFramebufferRef m_fbo = nullptr;

		GLTexture2DRef m_position = nullptr;
		GLTexture2DRef m_normal = nullptr;
		GLTexture2DRef m_diffuse = nullptr;
		GLTexture2DRef m_specular = nullptr;
		GLTexture2DRef m_depth = nullptr;

		GLProgramPipelineRef m_program = nullptr;

		int m_width = 0;
		int m_height = 0;

	};
	using GBufferRef = std::shared_ptr<GBuffer>;

	GBuffer::GBuffer(int width, int height)
	{
		Resize(width, height);

#pragma region VertexShader
		const char* vertSource = R"(
#version 460 core

// -----------  Per vertex  -----------
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoords;
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBitangent;
layout (location = 6) in vec4 ids;
layout (location = 7) in vec4 weights;
// ----------- Per instance -----------
//layout (location = 5) in mat4 aModel; // TODO:
//layout (location = 8) in int aMaterialIdx;
//layout (location = 9) in mat3 aNormalMat;

// --------- Output Variables ---------

out gl_PerVertex { vec4 gl_Position; };

out DeferredData
{
	vec3 position;
	vec3 color;
	vec3 normal;
	vec2 texCoords;
	vec3 tangent;
	vec3 bitangent;
} outData;

// ------------- Uniform --------------
layout (location = 0) uniform mat4 uProjectionMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uWorldMatrix;

layout (location = 3) uniform bool bones;
layout (location = 4) uniform mat4 pose[64];


void main()
{	
	vec4 pos = vec4(aPosition, 1.0);

	if(bones)
	{
		mat4 transform = mat4(0.0);
		transform += pose[int(ids.x)] * weights.x;
		pos = transform * pos;
	}

	vec4 worldPosition = uWorldMatrix * pos;
	mat3 worldNormal = transpose(inverse(mat3(uWorldMatrix)));
	vec4 worldTangent = uWorldMatrix * vec4(aTangent, 0.0);

	outData.position = worldPosition.xyz;
	outData.color = aColor;
	outData.normal = worldNormal * aNormal;
	outData.texCoords = aTexCoords;
	outData.tangent = worldTangent.xyz;
	outData.bitangent = aBitangent;

	gl_Position = uProjectionMatrix * uViewMatrix * worldPosition;
}
)";
#pragma endregion

#pragma region FragmentShader
		const char* fragSource = R"(
#version 460 core

in DeferredData
{
	vec3 position;
	vec3 color;
	vec3 normal;
	vec2 texCoords;
	vec3 tangent;
	vec3 bitangent;
} inData;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outDiffuse;
layout (location = 3) out vec4 outSpecular;

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D SpecularTexture;

layout (location = 0) uniform vec4 uSpecularCol;

layout (location = 1) uniform bool uHasNormalMap;

void main()
{
	vec4 diffuseTex = texture(DiffuseTexture, inData.texCoords);
	if (diffuseTex.a < 0.02) discard;

	vec3 normal = normalize(inData.normal);
	//if(uHasNormalMap)
	//{
		// TODO:
	//}

	outPosition = inData.position;
	outNormal = normal;
	outDiffuse.rgb = diffuseTex.rgb * inData.color;
	outDiffuse.a = diffuseTex.a;
	outSpecular = uSpecularCol;// texture(SpecularTexture, inData.texCoords).r;
}
)";
#pragma endregion

		m_program = std::make_shared<GLProgramPipeline>(vertSource, fragSource);
	}

	GBuffer::~GBuffer()
	{
		m_program.reset();
		m_fbo.reset();
		m_position.reset();
		m_normal.reset();
		m_diffuse.reset();
		m_specular.reset();
		m_depth.reset();
	}

	void GBuffer::Resize(int width, int height)
	{
		m_width = width;
		m_height = height;

		m_position.reset(new GLTexture2D(GL_RGB32F, GL_RGB, GL_FLOAT, width, height, nullptr, GL_NEAREST));
		m_normal.reset(new GLTexture2D(GL_RGB32F, GL_RGB, GL_FLOAT, width, height, nullptr, GL_NEAREST));
		m_diffuse.reset(new GLTexture2D(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, width, height, nullptr, GL_NEAREST));
		m_specular.reset(new GLTexture2D(GL_RGBA32F, GL_RGBA, GL_FLOAT, width, height, nullptr, GL_NEAREST));
		m_depth.reset(new GLTexture2D(GL_DEPTH_COMPONENT32F, GL_DEPTH, GL_FLOAT, width, height, nullptr, GL_NEAREST));

		m_fbo.reset(new GLFramebuffer({ m_position, m_normal, m_diffuse, m_specular }, m_depth));
	}

	void GBuffer::BindForWriting()
	{
		constexpr auto depthClearVal = 1.0f;

		m_fbo->ClearFramebuffer(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.0f)));
		m_fbo->ClearFramebuffer(GL_COLOR, 1, glm::value_ptr(glm::vec3(0.0f)));
		m_fbo->ClearFramebuffer(GL_COLOR, 2, glm::value_ptr(glm::vec4(0.0f)));
		m_fbo->ClearFramebuffer(GL_COLOR, 3, glm::value_ptr(glm::vec4(0.0f)));

		m_fbo->ClearFramebuffer(GL_DEPTH, 0, &depthClearVal);

		m_fbo->Bind();
		Renderer::SetViewport(0, 0, m_width, m_height);

		m_program->Bind();
	}

	void GBuffer::BindForReading()
	{
		m_position->Bind(0);
		m_normal->Bind(1);
		m_diffuse->Bind(2);
		m_specular->Bind(3);
	}

	GLProgramPipelineRef GBuffer::GetProgram()
	{
		return m_program;
	}

	// TODO: объединить в один класс - LightingPass
	class CoreLightingPassFB
	{
	public:
		void Create(int inWidth, int inHeight)
		{
			Resize(inWidth, inHeight);

#pragma region VertexShader
			const char* vertSource = R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };
out vec2 TexCoords;

// full screen quad vertices
const vec2 verts[] = { vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f,-1.0f), vec2(-1.0f,-1.0f) };
const vec2 uvs[] = { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) };
const uint index[] = { 0, 3, 2, 2, 1, 0 };

void main()
{
	TexCoords = uvs[index[gl_VertexID]];
	gl_Position = vec4(verts[index[gl_VertexID]], 0.0, 1.0);
}
)";
#pragma endregion

#pragma region FragmentShader
			const char* fragSource = R"(
#version 460

struct Light {
	vec3 position;
	vec3 color;

	float linear;
	float quadratic;
	float radius;
};

const int nsamples = 8;

in vec2 TexCoords;

out vec4 outFragColor;

layout (binding = 0) uniform sampler2D positionTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D diffuseTexture;
layout (binding = 3) uniform sampler2D specularTexture;
layout (binding = 4) uniform sampler2D shadowMapTexture;

layout (location = 0) uniform mat4 uLightSpaceMatrix;
layout (location = 1) uniform float uGlossiness;
layout (location = 2) uniform vec3 uCameraPos;

layout (location = 3) uniform vec4 offset[nsamples] = { 
							vec4(0.000000, 0.000000, 0.0, 0.0),
							vec4(0.079821, 0.165750, 0.0, 0.0),
							vec4(-0.331500, 0.159642, 0.0, 0.0),
							vec4(-0.239463, -0.497250, 0.0, 0.0),
							vec4(0.662999, -0.319284, 0.0, 0.0),
							vec4(0.399104, 0.828749, 0.0, 0.0),
							vec4(-0.994499, 0.478925, 0.0, 0.0),
							vec4(-0.558746, -1.160249, 0.0, 0.0) };

uniform Light uLight;


float getOcclusionCoef(vec4 shadowCoord, float bias)
{
	// get the stored depth
	float shadow_d = texture(shadowMapTexture, shadowCoord.xy).r;
	float diff = shadow_d - (shadowCoord.z + bias);
	return shadowCoord.z - bias > shadow_d  ? 0.0 : 1.0;
}

// using percent closer filtering technique
float percentCloserFilteredShadow (vec3 fragPos, vec3 normal)
{
	vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(fragPos, 1.0);
	// perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
	// calculate bias
	vec3 lightDir = normalize(uLight.position - fragPos);
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	float scale = 1.0 / textureSize(shadowMapTexture, 0).x;
	// sum shadow samples
	float shadowCoef = 0.0;
	for(int i=0; i<nsamples; i++)
	{
		shadowCoef += getOcclusionCoef(vec4(projCoords, 0.0) + scale*offset[i], bias);
	}
	shadowCoef /= nsamples;
	
	// keep the shadow at 1.0 when outside the zFar region of the light's frustum.
    if(projCoords.z > 1.0)
        shadowCoef = 1.0;
	
	return shadowCoef;
}

void main()
{
	// retrieve data form gbuffer
	const vec3 FragPos = texture(positionTexture, TexCoords).rgb;
	const vec3 Normal = texture(normalTexture, TexCoords).rgb;
	const vec3 Diffuse = texture(diffuseTexture, TexCoords).rgb;
	const vec4 Specular = texture(specularTexture, TexCoords);

	// do Phong lighting calculation
	vec3 ambient  = Diffuse * 0.1; // hard-coded ambient component
	vec3 viewDir  = normalize(uCameraPos - FragPos);

	// diffuse
	vec3 lightDir = normalize(uLight.position - FragPos);
	vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * uLight.color;

	// specular
	vec3 halfwayDir = normalize(lightDir + viewDir);  
	float spec = pow(max(dot(Normal, halfwayDir), 0.0), uGlossiness) * Specular.a;
	vec3 specular = uLight.color * spec * Specular.rgb;
	
	// attenuation
	float distance = length(uLight.position - FragPos);
	float attenuation = 1.0 / (1.0 + uLight.linear * distance + uLight.quadratic * distance * distance);

	diffuse *= attenuation;
	specular *= attenuation;

	// calculate shadow using PCF
	float shadow = percentCloserFilteredShadow(FragPos, Normal);
	
	vec3 result = ambient + (diffuse + specular) * shadow;
			
	outFragColor = vec4(result, 1.0);
	//outFragColor = pow(outFragColor, vec3(1.0f/2.2f));
}
)";
#pragma endregion

			program = std::make_shared<GLProgramPipeline>(vertSource, fragSource);
		}
		void Destroy()
		{
			program.reset();
			fbo.reset();
			color.reset();
			width = height = 0;
		}

		void Bind()
		{
			constexpr auto depthClearVal = 1.0f;
			fbo->ClearFramebuffer(GL_COLOR, 0, glm::value_ptr(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			fbo->ClearFramebuffer(GL_DEPTH, 0, &depthClearVal);

			fbo->Bind();
			Renderer::SetViewport(0, 0, width, height);

			program->Bind();
		}

		void Resize(int inWidth, int inHeight)
		{
			width = inWidth;
			height = inHeight;

			color.reset(new GLTexture2D(GL_RGBA8, GL_RGBA, width, height, nullptr, GL_NEAREST));

			fbo.reset(new GLFramebuffer({ color }));
		}

		GLFramebufferRef fbo = nullptr;
		GLTexture2DRef color = nullptr;

		GLProgramPipelineRef program = nullptr;

		int width = 0;
		int height = 0;
	};
	class PointsLightingPassFB
	{
	public:
		void Create(int inWidth, int inHeight)
		{
			Resize(inWidth, inHeight);

#pragma region VertexShader
			const char* vertSource = R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };

layout (location = 0) in vec3 aPosition;
layout (location = 2) in vec4 aInstanceParam;  // (RGB) light color and (A) is light radius
layout (location = 3) in mat4 aInstanceMatrix;

out vec3 lightColor;
out vec3 lightPosition;
out float lightRadius;

// ------------- Uniform --------------
layout (location = 0) uniform mat4 uProjectionMatrix;
layout (location = 1) uniform mat4 uViewMatrix;

void main()
{
	lightColor = aInstanceParam.rgb;
	lightRadius = aInstanceParam.w;
	// extract light position from the instance model matrix
	lightPosition = vec3(aInstanceMatrix[3]);
    gl_Position = uProjectionMatrix * uViewMatrix * aInstanceMatrix * vec4(lightRadius * aPosition, 1.0);
}
)";
#pragma endregion

#pragma region FragmentShader
			const char* fragSource = R"(
#version 460

layout (location = 0) out vec4 outFragColor;

in vec3 lightColor;
in float lightRadius;
in vec3 lightPosition;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gDiffuse;
layout (binding = 3) uniform sampler2D gSpecular;

layout (location = 0) uniform vec3 uCameraPos;
layout (location = 1) uniform float lightIntensity;
layout (location = 2) uniform vec2 screenSize;
layout (location = 3) uniform float glossiness;

void main()
{
	vec2 uvCoords = gl_FragCoord.xy / screenSize;
	vec3 FragPos = texture(gPosition, uvCoords).rgb;
	vec3 Normal = texture(gNormal, uvCoords).rgb;
	vec3 Diffuse = texture(gDiffuse, uvCoords).rgb;
	vec4 Specular = texture(gSpecular, uvCoords);
	
	// do Phong lighting calculation
	vec3 ambient  = Diffuse * 0.2; // ambient contribution
	vec3 viewDir  = normalize(uCameraPos - FragPos);
	
	// diffuse
	vec3 lightDir = normalize(lightPosition - FragPos);
	vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lightColor;
	// specular
	vec3 halfwayDir = normalize(lightDir + viewDir);  
	float spec = pow(max(dot(Normal, halfwayDir), 0.0), glossiness) * Specular.a;
	vec3 specular = lightColor * spec * Specular.rgb;
	// attenuation
	float distToL = length(lightPosition - FragPos);
	float attenuation = 1.0 - pow(smoothstep(0.0, 1.0, clamp(distToL/lightRadius, 0.0, 1.0)), 4.0);
	vec3 result = ambient + diffuse + specular;
	float noZTestFix = step(0.0, lightRadius - distToL); //0.0 if distToL > radius, 1.0 otherwise
	vec4 outColor = vec4(result, noZTestFix) * attenuation * lightIntensity;
	
	outFragColor = outColor;
}
)";
#pragma endregion

			program = std::make_shared<GLProgramPipeline>(vertSource, fragSource);
		}
		void Destroy()
		{
			program.reset();
			fbo.reset();
			color.reset();
			width = height = 0;
		}

		void Bind()
		{
			constexpr auto depthClearVal = 1.0f;
			fbo->ClearFramebuffer(GL_COLOR, 0, glm::value_ptr(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			fbo->ClearFramebuffer(GL_DEPTH, 0, &depthClearVal);

			fbo->Bind();
			Renderer::SetViewport(0, 0, width, height);

			program->Bind();
		}

		void Resize(int inWidth, int inHeight)
		{
			width = inWidth;
			height = inHeight;

			color.reset(new GLTexture2D(GL_RGBA8, GL_RGBA, width, height, nullptr, GL_NEAREST));

			fbo.reset(new GLFramebuffer({ color }));
		}

		GLFramebufferRef fbo = nullptr;
		GLTexture2DRef color = nullptr;

		GLProgramPipelineRef program = nullptr;

		int width = 0;
		int height = 0;
	};

	class OldDeferredLightingPassFB
	{
	public:
		void Create(int inWidth, int inHeight)
		{
			Resize(inWidth, inHeight);

#pragma region VertexShader
			const char* vertSource = R"(
#version 460

out gl_PerVertex { vec4 gl_Position; };
out vec2 TexCoords;

// full screen quad vertices
const vec2 verts[] = { vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f,-1.0f), vec2(-1.0f,-1.0f) };
const vec2 uvs[] = { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) };
const uint index[] = { 0, 3, 2, 2, 1, 0 };

void main()
{
	TexCoords = uvs[index[gl_VertexID]];
	gl_Position = vec4(verts[index[gl_VertexID]], 0.0, 1.0);
}
)";
#pragma endregion

#pragma region FragmentShader
			const char* fragSource = R"(
#version 460

in vec2 TexCoords;

out vec4 outFragColor;

layout (binding = 0) uniform sampler2D positionTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D albedoTexture;

struct Light {
	vec3 position;
	vec3 color;

	float linear;
	float quadratic;
	float radius;
};
const int NR_LIGHTS = 64;

layout (location = 0) uniform vec3 uCameraPos;
layout (location = 1) uniform Light uLights[NR_LIGHTS];

void main()
{
	// retrieve data form gbuffer
	const vec3 fragPos = texture(positionTexture, TexCoords).rgb;
	const vec3 normalTex = texture(normalTexture, TexCoords).rgb;
	const vec4 diffuseTex = texture(albedoTexture, TexCoords);
	const vec3 Diffuse = diffuseTex.rgb;
	const float Specular = diffuseTex.a;

	vec3 lighting = Diffuse * 0.1; // hard-coded ambient component
	vec3 viewDir = normalize(uCameraPos - fragPos);

	// point lights
	for(int i = 0; i < NR_LIGHTS; ++i)
	{
		// calculate distance between light source and current fragment
		float distance = length(uLights[i].position - fragPos);
		if (distance < uLights[i].radius)
		{
			// diffuse
			vec3 lightDir = normalize(uLights[i].position - fragPos);
			vec3 diffuse = max(dot(normalTex, lightDir), 0.0) * Diffuse * uLights[i].color;
			// specular
			vec3 halfwayDir = normalize(lightDir + viewDir);
			float spec = pow(max(dot(normalTex, halfwayDir), 0.0), 16.0);
			vec3 specular = uLights[i].color * spec * Specular;
			// attenuation
			float attenuation = 1.0 / (1.0 + uLights[i].linear * distance + uLights[i].quadratic * distance * distance);
			diffuse *= attenuation;
			specular *= attenuation;
			lighting += diffuse + specular;
		}
	}
	
	outFragColor = vec4(lighting, 1.0);
	//outFragColor = pow(outFragColor, vec3(1.0f/2.2f));
}
)";
#pragma endregion

			program = std::make_shared<GLProgramPipeline>(vertSource, fragSource);
		}
		void Destroy()
		{
			program.reset();
			fbo.reset();
			color.reset();
			width = height = 0;
		}

		void Bind()
		{
			constexpr auto depthClearVal = 1.0f;
			fbo->ClearFramebuffer(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.2, 0.6f, 1.0f)));
			fbo->ClearFramebuffer(GL_DEPTH, 0, &depthClearVal);

			fbo->Bind();
			Renderer::SetViewport(0, 0, width, height);

			program->Bind();
		}

		void Resize(int inWidth, int inHeight)
		{
			width = inWidth;
			height = inHeight;

			color.reset(new GLTexture2D(GL_RGBA8, GL_RGBA, width, height, nullptr, GL_NEAREST));

			fbo.reset(new GLFramebuffer({ color }));
		}

		GLFramebufferRef fbo = nullptr;
		GLTexture2DRef color = nullptr;

		GLProgramPipelineRef program = nullptr;

		int width = 0;
		int height = 0;
	};

	const float INITIAL_POINT_LIGHT_RADIUS = 1.663f;
	const unsigned int LIGHT_GRID_WIDTH = 10;  // point light grid size
	const unsigned int LIGHT_GRID_HEIGHT = 3;  // point light vertical grid height

	struct InstanceData {
		glm::vec4 instanceParam;
		glm::mat4 instanceMatrix;
	};

	// Node: separation < 1.0 will cause lights to penetrate each other, and > 1.0 they will separate (1.0 is just touching)
	void configurePointLights(std::vector<InstanceData>& modelData, float radius, float separation, float yOffset)
	{
		srand(glfwGetTime());
		// add some uniformly spaced point lights
		for (unsigned int lightIndexX = 0; lightIndexX < LIGHT_GRID_WIDTH; lightIndexX++)
		{
			for (unsigned int lightIndexZ = 0; lightIndexZ < LIGHT_GRID_WIDTH; lightIndexZ++)
			{
				for (unsigned int lightIndexY = 0; lightIndexY < LIGHT_GRID_HEIGHT; lightIndexY++)
				{
					float diameter = 2.0f * radius;
					float xPos = (lightIndexX - (LIGHT_GRID_WIDTH - 1.0f) / 2.0f) * (diameter * separation);
					float zPos = (lightIndexZ - (LIGHT_GRID_WIDTH - 1.0f) / 2.0f) * (diameter * separation);
					float yPos = (lightIndexY - (LIGHT_GRID_HEIGHT - 1.0f) / 2.0f) * (diameter * separation) + yOffset;
					double angle = double(rand()) * 2.0 * M_PI / (double(RAND_MAX));
					double length = double(rand()) * 0.5 / (double(RAND_MAX));
					float xOffset = cos(angle) * length;
					float zOffset = sin(angle) * length;
					xPos += xOffset;
					zPos += zOffset;
					// also calculate random color
					float rColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
					float gColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
					float bColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0

					int curLight = lightIndexX * LIGHT_GRID_WIDTH * LIGHT_GRID_HEIGHT + lightIndexZ * LIGHT_GRID_HEIGHT + lightIndexY;
					glm::mat4 model = glm::mat4(1.0f);
					model = glm::translate(model, glm::vec3(xPos, yPos, zPos));
					// now add to list of matrices
					InstanceData d;
					d.instanceParam = glm::vec4(rColor, gColor, bColor, radius);
					d.instanceMatrix = model;
					modelData.emplace_back(d);
				}
			}
		}
	}


}

/*
* Загрузка и вывод 3д модели из файла
* - mesh, model, material
*/
void Example002()
{
	Window::Create("Game", 1024, 768);
	Renderer::Init();
	IMGUI::Init();

	float lastFrameTime = static_cast<float>(glfwGetTime());

#pragma region VertexShader
	const char* mainVertSource = R"(
#version 460
#pragma debug(on)

out gl_PerVertex { vec4 gl_Position; };

out out_block
{
	vec3 pos;
	vec3 col;
	vec3 nrm;
	vec2 uvs;
} o;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;
layout (location = 2) in vec3 nrm;
layout (location = 3) in vec2 uvs;

layout (location = 0) uniform mat4 proj;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 modl;

void main()
{
	const vec4 mpos = (view * modl * vec4(pos, 1.0));
	o.pos = (modl * vec4(pos, 1.0)).xyz;
	o.col = col;
	o.nrm = mat3(transpose(inverse(modl))) * nrm;
	o.uvs = uvs;
	gl_Position = proj * mpos;
}
)";
#pragma endregion

#pragma region FragmentShader
	const char* mainFragSource = R"(
#version 460
#pragma debug(on)

in in_block
{
	vec3 pos;
	vec3 col;
	vec3 nrm;
	vec2 uvs;
} i;

layout (location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sDiffuseTexture;

void main()
{
	outColor = texture(sDiffuseTexture, i.uvs);
	outColor.rgb = outColor.rgb * i.col;
}
)";
#pragma endregion
	
	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	Camera camera;
	camera.Set({ 0.0f, 0.3f, -1.0f });

	glEnable(GL_DEPTH_TEST);

	GLProgramPipelineRef ppMain{ new GLProgramPipeline(mainVertSource, mainFragSource) };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };

	while (!Window::ShouldClose())
	{
#pragma region deltatime
		float currentFrame = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrame - lastFrameTime;
		lastFrameTime = currentFrame;
#pragma endregion

		Window::Update();

		if (Window::IsResize())
		{
			perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		}

		// Update
		{
			auto change = Mouse::GetDelta();

			if (Keyboard::IsPressed(GLFW_KEY_W)) camera.Move(Camera::Forward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_S)) camera.Move(Camera::Backward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_A)) camera.Move(Camera::Left, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_D)) camera.Move(Camera::Right, deltaTime);

			if (Mouse::IsPressed(Mouse::Button::Right))
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Disabled);
				if (change.x != 0.0f || change.y != 0.0f) camera.Rotate(change.x, -change.y);
			}
			else
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Normal);
			}
		}

#pragma region imgui

		IMGUI::Update();
		{
			ImGui::Begin((const char*)u8"Тест");
			ImGui::Text((const char*)u8"Test/Тест/%s", u8"тест 2");
			ImGui::End();
		}
#pragma endregion

#pragma region render
		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ppMain->Bind();
		ppMain->SetVertexUniform(0, perspective);
		ppMain->SetVertexUniform(1, camera.GetViewMatrix());
		ppMain->SetVertexUniform(2, glm::mat4(1.0f));

		model->Draw(ppMain);

		IMGUI::Draw();

		Window::Swap();
#pragma endregion
	}

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}

/*
* Deferred shading
* - фреймбуферы
*/
void Example003()
{
	Window::Create("Game", 1600, 900);
	Renderer::Init();
	IMGUI::Init();

	float lastFrameTime = static_cast<float>(glfwGetTime());

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	Camera camera;
	camera.Set({ 0.0f, 0.3f, -1.0f });

	UtilsExample::GBufferRef gbuffer{ new UtilsExample::GBuffer(Window::GetWidth(), Window::GetHeight()) };
	
	UtilsExample::OldDeferredLightingPassFB lightingPassFB;
	lightingPassFB.Create(Window::GetWidth(), Window::GetHeight());

	GLVertexArrayRef VAOEmpty{ new GLVertexArray };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };
	//ModelRef model{ new Model("Data/Models/sponza2/sponza.obj") };
	//ModelRef model{ new Model("Data/Models/holodeck/holodeck.obj") };
	//ModelRef model{ new Model("Data/Models/lost-empire/lost_empire.obj") };
	//ModelRef model{ new Model("Data/Models/sibenik/sibenik.obj") };

#pragma region lighting info
	const unsigned int NR_LIGHTS = 64;
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(123);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
		float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);

		//auto bmin = glm::abs(model->GetBounding().min);
		//auto bsize = bmin + glm::abs(model->GetBounding().max);
		//float xPos = static_cast<float>(rand() % (int)bsize.x - bmin.x);
		//float yPos = static_cast<float>(rand() % (int)bsize.y - bmin.y);
		//float zPos = static_cast<float>(rand() % (int)bsize.z - bmin.z);

		lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
		// also calculate random color
		float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
		float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
		float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
		lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

#pragma endregion


	while (!Window::ShouldClose())
	{
#pragma region deltatime
		float currentFrame = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrame - lastFrameTime;
		lastFrameTime = currentFrame;
#pragma endregion

		Window::Update();

		if (Window::IsResize())
		{
			perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
			gbuffer->Resize(Window::GetWidth(), Window::GetHeight());
			lightingPassFB.Resize(Window::GetWidth(), Window::GetHeight());
		}

		// Update
		{
			auto change = Mouse::GetDelta();

			if (Keyboard::IsPressed(GLFW_KEY_W)) camera.Move(Camera::Forward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_S)) camera.Move(Camera::Backward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_A)) camera.Move(Camera::Left, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_D)) camera.Move(Camera::Right, deltaTime);

			if (Mouse::IsPressed(Mouse::Button::Right))
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Disabled);
				if (change.x != 0.0f || change.y != 0.0f) camera.Rotate(change.x, -change.y);
			}
			else
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Normal);
			}
		}

#pragma region imgui

		IMGUI::Update();
		{
			ImGui::Begin((const char*)u8"Тест");
			ImGui::Text((const char*)u8"Test/Тест/%s", u8"тест 2");
			ImGui::End();
		}
#pragma endregion

#pragma region render
		// GBuffer
		{
			glEnable(GL_DEPTH_TEST);
			gbuffer->BindForWriting();
			gbuffer->GetProgram()->SetVertexUniform(0, perspective);
			gbuffer->GetProgram()->SetVertexUniform(1, camera.GetViewMatrix());
			gbuffer->GetProgram()->SetVertexUniform(2, glm::mat4(1.0f));
			model->Draw(gbuffer->GetProgram());
		}

		// Lighting pass framebuffer
		{
			lightingPassFB.Bind();
			lightingPassFB.program->SetFragmentUniform(0, camera.position);
			// send light relevant uniforms
			for (unsigned int i = 0; i < lightPositions.size(); i++)
			{
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].position"), lightPositions[i]);
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].color"), lightColors[i]);
				// update attenuation parameters and calculate radius
				const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
				const float linear = 0.7f;
				const float quadratic = 1.8f;
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].linear"), linear);
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].quadratic"), quadratic);
				// then calculate radius of light volume/sphere
				const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].x, lightColors[i].y), lightColors[i].z);
				float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
				lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLights[" + std::to_string(i) + "].radius"), radius);
			}

			gbuffer->BindForReading();
			VAOEmpty->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// Main frame
		{
			glDisable(GL_DEPTH_TEST);
			Renderer::MainFrameBuffer();
			Renderer::BlitFrameBuffer(lightingPassFB.fbo, nullptr,
				0, 0, Window::GetWidth(), Window::GetHeight(),
				0, 0, Window::GetWidth(), Window::GetHeight(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		IMGUI::Draw();

		Window::Swap();
#pragma endregion
	}

	gbuffer.reset();
	lightingPassFB.Destroy();

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}


void Example004()
{
	Window::Create("Game", 1600, 900);
	Renderer::Init();
	IMGUI::Init();

	float lastFrameTime = static_cast<float>(glfwGetTime());

	glm::mat4 perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
	glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	Camera camera;
	camera.Set({ 0.0f, 0.3f, -1.0f });

	bool enableShadows = true;
	bool drawPointLights = false;
	bool showDepthMap = false;
	bool drawPointLightsWireframe = true;
	glm::vec3 diffuseColor = glm::vec3(0.847f, 0.52f, 0.19f);
	glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
	const float glossiness = 16.0f;
	float gLinearAttenuation = 0.09f;
	float gQuadraticAttenuation = 0.032f;
	float pointLightIntensity = 0.736f;
	float pointLightRadius = UtilsExample::INITIAL_POINT_LIGHT_RADIUS;
	float pointLightVerticalOffset = 0.636f;
	float pointLightSeparation = 0.670f;
	const int totalLights = UtilsExample::LIGHT_GRID_WIDTH * UtilsExample::LIGHT_GRID_WIDTH * UtilsExample::LIGHT_GRID_HEIGHT;

	// initialize point lights
	// lighting info
	std::vector<UtilsExample::InstanceData> instanceData;
	UtilsExample::configurePointLights(instanceData, pointLightRadius, pointLightSeparation, pointLightVerticalOffset);

	class SceneLight
	{
	public:
		SceneLight(const glm::vec3& _position, const glm::vec3& _color, float _radius) : position(_position), color(_color), radius(_radius) {}
		glm::vec3 position; // world light position
		glm::vec3 color; // light's color
		float radius; // light's radius
	};

	SceneLight globalLight(glm::vec3(-2.5f, 5.0f, -1.25f), glm::vec3(1.0f, 1.0f, 1.0f), 0.125f);

	UtilsExample::ShadowPass simpleShadowMapFB;
	simpleShadowMapFB.Create(UtilsExample::SHADOW_WIDTH, UtilsExample::SHADOW_HEIGHT);


	UtilsExample::GBufferRef gbuffer{ new UtilsExample::GBuffer(Window::GetWidth(), Window::GetHeight()) };


	// TODO: объединить в один шейдер, так как в разделении нет смысла
	UtilsExample::CoreLightingPassFB lightingPassFB;
	lightingPassFB.Create(Window::GetWidth(), Window::GetHeight());
	UtilsExample::PointsLightingPassFB pointsLightingPassFB;
	pointsLightingPassFB.Create(Window::GetWidth(), Window::GetHeight());

	GLVertexArrayRef VAOEmpty{ new GLVertexArray };

	ModelRef model{ new Model("Data/Models/sponza/sponza.obj") };
	ModelRef model2{ new Model("Data/Models/Dragon.obj") };
	ModelRef sphereModel{ new Model("Data/Models/Sphere.obj") };
	auto sphereVao = (*sphereModel)[0]->GetVAO();
	GLBufferRef instanceBuffer{ new GLBuffer(instanceData) };
	// TODO
	{
		// vertex data
		glEnableVertexArrayAttrib(*sphereVao, 0);
		glVertexArrayAttribBinding(*sphereVao, 0, 0);
		glVertexArrayAttribFormat(*sphereVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(MeshVertex, position));

		// instance data
		glEnableVertexArrayAttrib(*sphereVao, 2);
		glVertexArrayAttribBinding(*sphereVao, 2, 1);
		glVertexArrayAttribFormat(*sphereVao, 2, 4, GL_FLOAT, GL_FALSE, offsetof(UtilsExample::InstanceData, instanceParam));

		glEnableVertexArrayAttrib(*sphereVao, 3);
		glVertexArrayAttribBinding(*sphereVao, 3, 1);
		glVertexArrayAttribFormat(*sphereVao, 3, 4, GL_FLOAT, GL_FALSE, offsetof(UtilsExample::InstanceData, instanceMatrix));

		glEnableVertexArrayAttrib(*sphereVao, 4);
		glVertexArrayAttribBinding(*sphereVao, 4, 1);
		glVertexArrayAttribFormat(*sphereVao, 4, 4, GL_FLOAT, GL_FALSE, offsetof(UtilsExample::InstanceData, instanceMatrix) + sizeof(float) * 4);

		glEnableVertexArrayAttrib(*sphereVao, 5);
		glVertexArrayAttribBinding(*sphereVao, 5, 1);
		glVertexArrayAttribFormat(*sphereVao, 5, 4, GL_FLOAT, GL_FALSE, offsetof(UtilsExample::InstanceData, instanceMatrix) + sizeof(float) * 8);

		glEnableVertexArrayAttrib(*sphereVao, 6);
		glVertexArrayAttribBinding(*sphereVao, 6, 1);
		glVertexArrayAttribFormat(*sphereVao, 6, 4, GL_FLOAT, GL_FALSE, offsetof(UtilsExample::InstanceData, instanceMatrix) + sizeof(float) * 12);

		glVertexArrayVertexBuffer(*sphereVao, 1, *instanceBuffer, 0, sizeof(UtilsExample::InstanceData));
		glVertexArrayBindingDivisor(*sphereVao, 1, 1);

	}

	QuadShapeRef quad{ new QuadShape{} };
	CubeShapeRef cube{ new CubeShape{} };
	SphereShapeRef sphere{ new SphereShape{} };

	while (!Window::ShouldClose())
	{
#pragma region deltatime
		float currentFrame = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrame - lastFrameTime;
		lastFrameTime = currentFrame;
#pragma endregion

		Window::Update();

		if (Window::IsResize())
		{
			perspective = glm::perspective(glm::radians(60.0f), (float)Window::GetWidth() / (float)Window::GetHeight(), 0.1f, 1000.f);
			glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
			gbuffer->Resize(Window::GetWidth(), Window::GetHeight());
			lightingPassFB.Resize(Window::GetWidth(), Window::GetHeight());
			pointsLightingPassFB.Resize(Window::GetWidth(), Window::GetHeight());
		}

		// Update
		{
			auto change = Mouse::GetDelta();

			if (Keyboard::IsPressed(GLFW_KEY_W)) camera.Move(Camera::Forward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_S)) camera.Move(Camera::Backward, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_A)) camera.Move(Camera::Left, deltaTime);
			if (Keyboard::IsPressed(GLFW_KEY_D)) camera.Move(Camera::Right, deltaTime);

			if (Mouse::IsPressed(Mouse::Button::Right))
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Disabled);
				if (change.x != 0.0f || change.y != 0.0f) camera.Rotate(change.x, -change.y);
			}
			else
			{
				Mouse::SetCursorMode(Mouse::CursorMode::Normal);
			}
		}

#pragma region imgui
		IMGUI::Update();
		{
			ImGui::Begin((const char*)u8"Тест");
			ImGui::Text((const char*)u8"Test/Тест/%s", u8"тест 2");
			ImGui::End();
		}
#pragma endregion

#pragma region render
		glEnable(GL_DEPTH_TEST);

		// SHADOW STAGE
		// 1. render depth of scene to texture (from light's perspective)
		// TODO: для каждого глобального (прямого) источника света генерить свою карту теней
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		{
			simpleShadowMapFB.Bind();

			if (enableShadows)
			{
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);

				lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
				lightView = glm::lookAt(globalLight.position, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
				lightSpaceMatrix = lightProjection * lightView;
				// render scene from light's point of view

				simpleShadowMapFB.program->SetVertexUniform(0, lightSpaceMatrix);

				// DRAW MODEL
				{
					simpleShadowMapFB.program->SetVertexUniform(1, glm::mat4(1.0f));
					model->Draw(simpleShadowMapFB.program);

					glm::mat4 modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
					glm::mat4 modelScale = glm::scale(modelTranslate, glm::vec3(0.2f));
					simpleShadowMapFB.program->SetVertexUniform(1, modelScale);
					model2->Draw(simpleShadowMapFB.program);

					modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.65f, 0.0f));
					modelScale = glm::scale(modelTranslate, glm::vec3(10.0f));
					simpleShadowMapFB.program->SetVertexUniform(1, modelScale);
					quad->Draw();

					modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.0f, 0.0f));
					modelScale = glm::scale(modelTranslate, glm::vec3(2.0f));
					simpleShadowMapFB.program->SetVertexUniform(1, modelScale);
					cube->Draw();

					modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, 0.0f, 0.0f));
					modelScale = glm::scale(modelTranslate, glm::vec3(1.5f));
					simpleShadowMapFB.program->SetVertexUniform(1, modelScale);
					sphere->Draw();
				}
			}
		}

		// 2. geometry pass: render scene's geometry/color data into gbuffer
		{
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);

			gbuffer->BindForWriting();
			gbuffer->GetProgram()->SetVertexUniform(0, perspective);
			gbuffer->GetProgram()->SetVertexUniform(1, camera.GetViewMatrix());
			gbuffer->GetProgram()->SetVertexUniform(2, glm::mat4(1.0f));

			glm::vec4 sponzaSpecular = glm::vec4(0.5f, 0.5f, 0.5f, 0.8f);
			gbuffer->GetProgram()->SetFragmentUniform(0, sponzaSpecular);
			model->Draw(gbuffer->GetProgram());

			glm::mat4 modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
			glm::mat4 modelScale = glm::scale(modelTranslate, glm::vec3(0.2f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			glm::vec4 modelSpecular = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
			gbuffer->GetProgram()->SetFragmentUniform(0, modelSpecular);
			model2->Draw(gbuffer->GetProgram());

			modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.65f, 0.0f));
			modelScale = glm::scale(modelTranslate, glm::vec3(10.0f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			quad->Draw();

			modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.0f, 0.0f));
			modelScale = glm::scale(modelTranslate, glm::vec3(2.0f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			cube->Draw();

			modelTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, 0.0f, 0.0f));
			modelScale = glm::scale(modelTranslate, glm::vec3(1.5f));
			gbuffer->GetProgram()->SetVertexUniform(2, modelScale);
			sphere->Draw();
		}

		// 3. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content and shadow map
		//if (gBufferMode == 0) // если цифра, то дебажный режим для вывода выбранной текстуры из gbuffer
		{
			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			lightingPassFB.Bind();

			lightingPassFB.program->SetFragmentUniform(0, lightSpaceMatrix);
			lightingPassFB.program->SetFragmentUniform(1, glossiness);
			lightingPassFB.program->SetFragmentUniform(2, camera.position);

			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.position"), globalLight.position);
			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.color"), globalLight.color);
			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.linear"), gLinearAttenuation);
			lightingPassFB.program->SetFragmentUniform(lightingPassFB.program->GetFragmentUniform("uLight.quadratic"), gQuadraticAttenuation);

			gbuffer->BindForReading();
			simpleShadowMapFB.depth->Bind(4);
			VAOEmpty->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		//else
		//{
		//	// отрисовка буферов из gbuffer
		//}

		// 3.5 lighting pass: render point lights on top of main scene with additive blending and utilizing G-Buffer for lighting.
		//if (gBufferMode == 0)
		{
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CW); // TODO: чтобы не рисовало сзади?
			//glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			pointsLightingPassFB.Bind();
			Renderer::BlitFrameBuffer(lightingPassFB.fbo, pointsLightingPassFB.fbo,
				0, 0, Window::GetWidth(), Window::GetHeight(),
				0, 0, Window::GetWidth(), Window::GetHeight(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);

			pointsLightingPassFB.program->SetVertexUniform(0, perspective);
			pointsLightingPassFB.program->SetVertexUniform(1, camera.GetViewMatrix());
			pointsLightingPassFB.program->SetFragmentUniform(0, camera.position);
			pointsLightingPassFB.program->SetFragmentUniform(1, pointLightIntensity);
			pointsLightingPassFB.program->SetFragmentUniform(2, glm::vec2(Window::GetWidth(), Window::GetHeight()));
			pointsLightingPassFB.program->SetFragmentUniform(3, glossiness);

			gbuffer->BindForReading();

			// draw instances
			sphereVao->Bind();
			glDrawElementsInstanced(GL_TRIANGLES, 2280, GL_UNSIGNED_INT, 0, totalLights);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glFrontFace(GL_CCW);
			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);
		}

		// Main frame
		{
			glDisable(GL_DEPTH_TEST);
			Renderer::MainFrameBuffer();
			Renderer::BlitFrameBuffer(pointsLightingPassFB.fbo, nullptr,
				0, 0, Window::GetWidth(), Window::GetHeight(),
				0, 0, Window::GetWidth(), Window::GetHeight(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);




		}

		IMGUI::Draw();

		Window::Swap();
#pragma endregion
	}

	gbuffer.reset();
	lightingPassFB.Destroy();

	IMGUI::Close();
	Renderer::Close();
	Window::Destroy();
}