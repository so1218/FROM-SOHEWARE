#include "Weapon.h"
#include "Engine.h"
#include "Player.h"

Weapon::Weapon(Engine* engine, Player* owner) : engine_(engine), owner_(owner) {}