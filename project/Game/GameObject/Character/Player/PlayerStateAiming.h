#pragma once
#include "Player.h"

class PlayerStateAiming : public State<Player>
{
public:
	static PlayerStateAiming* GetInstance()
	{
		static PlayerStateAiming instance;
		return &instance;
	}

	void Enter(Player* player) override;
	void Exit(Player* player) override;
	void Update(Player* pplayer) override;

	std::string GetName() override { return "Aiming"; }
};