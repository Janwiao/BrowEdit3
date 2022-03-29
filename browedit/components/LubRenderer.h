#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/util/Singleton.h>

namespace gl { class Texture; }
class RswObject;
class Gnd;
class LubEffect;

class LubRenderer : public Renderer
{
public:
	class LubShader : public gl::Shader
	{
	public:
		LubShader() : gl::Shader("data/shaders/lub", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				projectionMatrix,
				cameraMatrix,
				modelMatrix,
				s_texture,
				color,
				billboard,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::cameraMatrix, "cameraMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::modelMatrix, "modelMatrix");
			bindUniform(Uniforms::color, "color");
		}
	};
private:
	RswObject* rswObject = nullptr;
	LubEffect* lubEffect = nullptr;

	gl::Texture* texture;

	float lastTime;
	float emitTime = 0;
	class Particle
	{
	public:
		glm::vec3 position;
		glm::vec3 speed;
		float size;
		float life;
	};
	std::vector<Particle> particles;

public:
	Gnd* gnd;
	class LubRenderContext : public Renderer::RenderContext, public util::Singleton<LubRenderContext>
	{
	public:
		LubShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);

		LubRenderContext();
		virtual void preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) override;
	};

	LubRenderer();
	virtual void render();
	bool selected = false;
};