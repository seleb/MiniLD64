#pragma once

#include <MY_Game.h>
#include <MY_Scene.h>

MY_Game::MY_Game() :
	Game("test", new MY_Scene(this), true)
{
}

MY_Game::~MY_Game(){

}

void MY_Game::addSplashes(){
	// add default splashes
	//Game::addSplashes();

	// add custom splashes
}