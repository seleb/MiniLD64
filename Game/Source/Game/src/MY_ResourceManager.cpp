#pragma once

#include <MY_ResourceManager.h>

Scenario * MY_ResourceManager::scenario = nullptr;

void MY_ResourceManager::init(){
	scenario = new Scenario("assets/scenario.json");
	resources.push_back(scenario);
}