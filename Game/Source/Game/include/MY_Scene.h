#pragma once

#include <Scene.h>
#include <sweet/UI.h>
#include <BulletWorld.h>

class PerspectiveCamera;
class MousePerspectiveCamera;

class MeshEntity;

class ShaderComponentHsv;

class Shader;
class RenderSurface;
class StandardFrameBuffer;
class Material;
class Sprite;

class PointLight;

class BulletMeshEntity;
class ComponentShaderText;
class Timeout;

class MY_Scene : public Scene{
public:
	Shader * screenSurfaceShader;
	RenderSurface * screenSurface;
	StandardFrameBuffer * screenFBO;
	
	ComponentShaderBase * baseShader;
	ComponentShaderText * textShader;

	BulletWorld * bulletWorld;

	virtual void update(Step * _step) override;
	virtual void render(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions) override;
	
	virtual void load() override;
	virtual void unload() override;

	UILayer uiLayer;

	MY_Scene(Game * _game);
	~MY_Scene();



	BulletMeshEntity * bulletFloor;
	BulletMeshEntity * ship;
	PerspectiveCamera * playerCam;
	float zoom;
	float bounds;
	float score;
	float accuracy;

	TextLabel * scoreIndicator;

	std::vector<BulletMeshEntity *> obstacles;
	std::vector<BulletMeshEntity *> trail;
	Timeout * trailTimeout;
	Timeout * obstacleTimeout;
	float shipAngle;
};