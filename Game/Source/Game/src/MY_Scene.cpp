#pragma once

#include <MY_Scene.h>
#include <MY_ResourceManager.h>

#include <Game.h>
#include <MeshEntity.h>
#include <MeshInterface.h>
#include <MeshFactory.h>
#include <Resource.h>

#include <DirectionalLight.h>
#include <PointLight.h>
#include <SpotLight.h>

#include <shader\ComponentShaderBase.h>
#include <shader\ComponentShaderText.h>
#include <shader\ShaderComponentText.h>
#include <shader\ShaderComponentTexture.h>
#include <shader\ShaderComponentDiffuse.h>
#include <shader\ShaderComponentHsv.h>
#include <shader\ShaderComponentMVP.h>

#include <MousePerspectiveCamera.h>

#include <Mouse.h>
#include <Keyboard.h>
#include <GLFW\glfw3.h>

#include <RenderSurface.h>
#include <StandardFrameBuffer.h>
#include <NumberUtils.h>

#include <NodeBulletBody.h>
#include <BulletMeshEntity.h>
#include <TextArea.h>
#include <MeshInterface.h>

#include <RenderOptions.h>

#include <Timeout.h>

MY_Scene::MY_Scene(Game * _game) :
	Scene(_game),
	screenSurfaceShader(new Shader("assets/RenderSurface", false, true)),
	screenSurface(new RenderSurface(screenSurfaceShader)),
	screenFBO(new StandardFrameBuffer(true)),
	baseShader(new ComponentShaderBase(true)),
	textShader(new ComponentShaderText(true)),
	uiLayer(0,0,0,0),
	bounds(10),
	bulletWorld(new BulletWorld()),
	score(0),
	accuracy(3.f)
{
	baseShader->addComponent(new ShaderComponentMVP(baseShader));
	//baseShader->addComponent(new ShaderComponentDiffuse(baseShader));
	baseShader->addComponent(new ShaderComponentTexture(baseShader));
	baseShader->compileShader();

	textShader->textComponent->setColor(glm::vec3(0.0f, 64.f/255.f, 165.f/255.f));
	
	screenSurface->setScaleMode(GL_NEAREST);

	// remove initial camera
	childTransform->removeChild(cameras.at(0)->parents.at(0));
	delete cameras.at(0)->parents.at(0);
	cameras.pop_back();
	

	// reference counting for member variables
	++baseShader->referenceCount;
	++textShader->referenceCount;

	++screenSurface->referenceCount;
	++screenSurfaceShader->referenceCount;
	++screenFBO->referenceCount;

	float floorL = 1000, floorW = 1000;

	// create floor/ceiling as static bullet planes
	bulletFloor = new BulletMeshEntity(bulletWorld, MeshFactory::getPlaneMesh(), baseShader);
	bulletFloor->setColliderAsStaticPlane(0, 1, 0, 0);
	bulletFloor->createRigidBody(0);
	bulletFloor->body->setFriction(0);
	childTransform->addChild(bulletFloor);
	bulletFloor->meshTransform->scale(-floorL, floorW, 1.f);
	bulletFloor->meshTransform->rotate(-90, 1, 0, 0, kOBJECT);
	bulletFloor->body->getWorldTransform().setRotation(btQuaternion(btVector3(0, 1, 0), glm::radians(180.f)));
	//bulletFloor->mesh->setScaleMode(GL_NEAREST);
	bulletFloor->mesh->scaleModeMag = GL_NEAREST;
	bulletFloor->mesh->scaleModeMin = GL_LINEAR;
	bulletFloor->mesh->pushTexture2D(MY_ResourceManager::scenario->getTexture("CLOUDS")->texture);
	// adjust UVs so that the texture tiles in squares
	for(Vertex & v : bulletFloor->mesh->vertices){
		v.u *= 30.f;
		v.v *= 30.f;
	}



	// car
	TriMesh * shipMesh = MY_ResourceManager::scenario->getMesh("SHIP")->meshes.at(0);
	ship = new BulletMeshEntity(bulletWorld, shipMesh, baseShader);
	childTransform->addChild(ship);
	ship->mesh->setScaleMode(GL_NEAREST);
	ship->mesh->pushTexture2D(MY_ResourceManager::scenario->getTexture("SHIP")->texture);

	ship->setColliderAsBoundingBox();
	ship->createRigidBody(10);
	
	ship->body->setLinearFactor(btVector3(1, 1, 0));
	ship->body->setAngularFactor(btVector3(0, 0, 0));

	ship->maxVelocity = btVector3(10, 10, 0);

	ship->body->setActivationState(4); // always active
	ship->translatePhysical(glm::vec3(0, 10, 0));
	
	//Set up player camera
	zoom = 3.f;
	playerCam = new PerspectiveCamera();
	cameras.push_back(playerCam);
	ship->meshTransform->addChild(playerCam);
	playerCam->nearClip = 0.01f;
	playerCam->farClip = 1000.f;
	playerCam->childTransform->rotate(90, 0, 1, 0, kWORLD);
	playerCam->firstParent()->translate(0, 1.5f, zoom, false);
	playerCam->yaw = 90.0f;
	playerCam->pitch = -10.0f;
	activeCamera = playerCam;

	shipAngle = 0;

	MeshInterface * sphereMesh = MY_ResourceManager::scenario->getMesh("SPHERE")->meshes.at(0);
	sphereMesh->setScaleMode(GL_NEAREST);
	sphereMesh->pushTexture2D(MY_ResourceManager::scenario->getTexture("TRAIL")->texture);

	MeshInterface * ringMesh = MY_ResourceManager::scenario->getMesh("RING")->meshes.at(0);
	ringMesh->setScaleMode(GL_NEAREST);
	ringMesh->pushTexture2D(MY_ResourceManager::scenario->getTexture("RING")->texture);

	trailTimeout = new Timeout(0.05f, [this, sphereMesh](sweet::Event * _event){
		// add player trail
		BulletMeshEntity * t = new BulletMeshEntity(bulletWorld, sphereMesh, baseShader);
		t->setColliderAsBoundingSphere();
		t->createRigidBody(1, 0, 0);
		childTransform->addChild(t);
		trail.push_back(t);
		t->translatePhysical(ship->meshTransform->getWorldPos(), false);
		t->childTransform->scale(sweet::NumberUtils::randomFloat(1,3));
		t->meshTransform->rotate(sweet::NumberUtils::randomFloat(0, 360), 0, 0, 1, kOBJECT);
		t->body->applyCentralImpulse(btVector3(sweet::NumberUtils::randomFloat(-3,3), sweet::NumberUtils::randomFloat(-3,3), sweet::NumberUtils::randomFloat(-3,3) + 10));
		trailTimeout->restart();
	});
	trailTimeout->start();
	//childTransform->addChild(trailTimeout);

	
	obstacleTimeout = new Timeout(0.5f, [this, ringMesh](sweet::Event * _event){
		// add player trail
		BulletMeshEntity * t = new BulletMeshEntity(bulletWorld, ringMesh, baseShader);
		t->setColliderAsBoundingSphere();
		t->createRigidBody(0, 0, 0);
		childTransform->addChild(t);
		obstacles.push_back(t);
		t->translatePhysical(glm::vec3(sweet::NumberUtils::randomFloat(-bounds,bounds)*0.5, sweet::NumberUtils::randomFloat(bounds*0.2,bounds*0.8), -300), false);
		t->meshTransform->rotate(sweet::NumberUtils::randomFloat(0, 360), 0, 0, 1, kOBJECT);
		t->body->setActivationState(4); // always active
		obstacleTimeout->restart();
	});
	obstacleTimeout->start();
	//childTransform->addChild(obstacleTimeout);

	VerticalLinearLayout * vl = new VerticalLinearLayout(uiLayer.world);
	vl->setRationalHeight(1.0f);
	vl->setRationalWidth(1.f);
	vl->setMarginTop(0.1f);
	vl->setRenderMode(kTEXTURE);
	vl->verticalAlignment = kTOP;


	scoreIndicator = new TextLabel(uiLayer.world, MY_ResourceManager::scenario->getFont("FONT")->font, textShader, 1.f);
	scoreIndicator->horizontalAlignment = kCENTER;
	scoreIndicator->setText("SCORE: 0");
	scoreIndicator->invalidateLayout();

	vl->addChild(scoreIndicator);
	uiLayer.addChild(vl);



	MY_ResourceManager::scenario->getAudio("BGM")->sound->play(true);
}

MY_Scene::~MY_Scene(){
	deleteChildTransform();

	
	// auto-delete member variables
	baseShader->decrementAndDelete();
	textShader->decrementAndDelete();

	screenSurface->decrementAndDelete();
	screenSurfaceShader->decrementAndDelete();
	screenFBO->decrementAndDelete();
}


void MY_Scene::update(Step * _step){
	bulletWorld->update(_step);




	glm::vec3 playerPos = ship->meshTransform->getWorldPos();

	trailTimeout->update(_step);
	obstacleTimeout->update(_step);

	// update scrolling floor tex
	for(Vertex & v : bulletFloor->mesh->vertices){
		v.v += 0.1f;
		//v.v += floorW;
	}
	bulletFloor->mesh->dirty = true;


	// clear and update trail
	for(unsigned long int i = trail.size(); i > 0; --i){
		auto p = trail.at(i-1);
		if(p->meshTransform->getWorldPos().z > playerCam->childTransform->getWorldPos().z){
			childTransform->removeChild(p->firstParent());
			delete p->firstParent();
			trail.erase(trail.begin() + (i-1));
		}else{
			p->body->applyCentralImpulse(btVector3(0,0,2));
		}
	}

	// clear and update rings
	for(unsigned long int i = obstacles.size(); i > 0; --i){
		auto p = obstacles.at(i-1);
		glm::vec3 pos = p->meshTransform->getWorldPos();
		if(pos.z > playerCam->childTransform->getWorldPos().z){
			childTransform->removeChild(p->firstParent());
			delete p->firstParent();
			obstacles.erase(obstacles.begin() + (i-1));
		}else if(glm::distance2(pos, playerPos) < accuracy){
			childTransform->removeChild(p->firstParent());
			delete p->firstParent();
			obstacles.erase(obstacles.begin() + (i-1));
			score += 10;
			
			std::stringstream ss;
			ss << "SCORE: " << score;
			scoreIndicator->setText(ss.str());
			scoreIndicator->invalidateLayout();

			MY_ResourceManager::scenario->getAudio("BLIP")->sound->play();
		}else{
			p->body->translate(btVector3(0,0,1));
			//p->realign();
		}
	}


	
	if(keyboard->keyJustDown(GLFW_KEY_ESCAPE)){
		game->exit();
	}

	// player controls
	float playerSpeed = 30;
	if (keyboard->keyDown(GLFW_KEY_UP) || keyboard->keyDown(GLFW_KEY_W)){
		ship->body->applyCentralImpulse(btVector3(0, playerSpeed, 0));
	}if (keyboard->keyDown(GLFW_KEY_DOWN) || keyboard->keyDown(GLFW_KEY_S)){
		ship->body->applyCentralImpulse(btVector3(0, -playerSpeed, 0));
	}if (keyboard->keyDown(GLFW_KEY_LEFT) || keyboard->keyDown(GLFW_KEY_A)){
		ship->body->applyCentralImpulse(btVector3(-playerSpeed, 0, 0));
		//ship->body->applyTorqueImpulse(btVector3(0, 0, 1));
	}if (keyboard->keyDown(GLFW_KEY_RIGHT) || keyboard->keyDown(GLFW_KEY_D)){
		ship->body->applyCentralImpulse(btVector3(playerSpeed, 0, 0));
		//ship->body->applyTorqueImpulse(btVector3(0, 0, -1));
	}

	btVector3 vel = ship->body->getLinearVelocity();
	if(playerPos.x > bounds){
		if(vel.x() > 10){
			ship->body->setLinearVelocity(btVector3(vel.x() * 0.1f, vel.y(), vel.z()));
		}
	}if(playerPos.x < -bounds){
		if(vel.x() < -10){
			ship->body->setLinearVelocity(btVector3(vel.x() * 0.1f, vel.y(), vel.z()));
		}
	}if(playerPos.y < -bounds){
		if(vel.y() < -10){
			ship->body->setLinearVelocity(btVector3(vel.x(), vel.y() * 0.1f, vel.z()));
		}
	}if(playerPos.y > bounds){
		if(vel.y() > 10){
			ship->body->setLinearVelocity(btVector3(vel.x(), vel.y() * 0.1f, vel.z()));
		}
	}

	shipAngle += (-vel.x()*3.f - shipAngle)*0.1f;

	ship->meshTransform->setOrientation(glm::angleAxis(shipAngle, glm::vec3(0,0,1)));
	
	// keep camera on player
	zoom += (glm::max(3.0f, vel.length()*0.5f) - zoom) * 0.2f;
	playerCam->firstParent()->translate(0, 1.f, zoom, false);


	glm::uvec2 sd = sweet::getWindowDimensions();
	uiLayer.resize(0, sd.x, 0, sd.y);
	Scene::update(_step);
	uiLayer.update(_step);
}

void MY_Scene::render(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions){
	glm::uvec4 v(game->viewPortX, game->viewPortY, game->viewPortWidth, game->viewPortHeight);
	int w = game->viewPortWidth;
	int h = game->viewPortHeight;
	int wtarget = 320, htarget = 240;
	int wfinal = wtarget, hfinal = htarget;
	
	if(w > wtarget){
		wfinal = glm::min(w/(w/wtarget), w);
		hfinal = glm::min(h/(w/wtarget), h);
	}if(h > htarget){
		wfinal = glm::min(w/(h/htarget), w);
		hfinal = glm::min(h/(h/htarget), h);
	}

	game->setViewport(0,0,wfinal,hfinal);
	screenFBO->resize(game->viewPortWidth, game->viewPortHeight);
	//Bind frameBuffer
	screenFBO->bindFrameBuffer();
	//render the scene to the buffer
	
	_renderOptions->setClearColour(0,90.f/255.f,191.f/255.f, 1);
	_renderOptions->clear();
	Scene::render(_matrixStack, _renderOptions);
	
	// resize back to normal
	game->setViewport(v.x, v.y, v.z, v.w);

	//Render the buffer to the render surface
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	screenSurface->render(screenFBO->getTextureId());
	uiLayer.render(_matrixStack, _renderOptions);
}

void MY_Scene::load(){
	Scene::load();	

	screenSurface->load();
	screenFBO->load();
	uiLayer.load();
}

void MY_Scene::unload(){
	uiLayer.unload();
	screenFBO->unload();
	screenSurface->unload();

	Scene::unload();	
}