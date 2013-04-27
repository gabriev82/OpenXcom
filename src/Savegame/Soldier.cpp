/*
 * Copyright 2010-2012 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Soldier.h"
#include "../Engine/RNG.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Savegame/Craft.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Ruleset/SoldierNamePool.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"

namespace OpenXcom
{

/**
 * Initializes a new soldier, either blank or randomly generated.
 * @param rules Soldier ruleset.
 * @param armor Soldier armor.
 * @param names List of name pools for soldier generation.
 * @param id Pointer to unique soldier id for soldier generation.
 */
Soldier::Soldier(RuleSoldier *rules, Armor *armor, const std::vector<SoldierNamePool*> *names, int id) : _name(L""), _id(0), _improvement(0), _rules(rules), _initialStats(), _currentStats(), _rank(RANK_ROOKIE), _craft(0), _gender(GENDER_MALE), _look(LOOK_BLONDE), _missions(0), _kills(0), _recovery(0), _recentlyPromoted(false), _psiTraining(false), _armor(armor), _equipmentLayout()
{
	if (names != 0)
	{
		UnitStats minStats = rules->getMinStats();
		UnitStats maxStats = rules->getMaxStats();

		_initialStats.tu = RNG::generate(minStats.tu, maxStats.tu);
		_initialStats.stamina = RNG::generate(minStats.stamina, maxStats.stamina);
		_initialStats.health = RNG::generate(minStats.health, maxStats.health);
		_initialStats.bravery = RNG::generate(minStats.bravery/10, maxStats.bravery/10)*10;
		_initialStats.reactions = RNG::generate(minStats.reactions, maxStats.reactions);
		_initialStats.firing = RNG::generate(minStats.firing, maxStats.firing);
		_initialStats.throwing = RNG::generate(minStats.throwing, maxStats.throwing);
		_initialStats.strength = RNG::generate(minStats.strength, maxStats.strength);
		_initialStats.psiStrength = RNG::generate(minStats.psiStrength, maxStats.psiStrength);
		_initialStats.melee = RNG::generate(minStats.melee, maxStats.melee);
		_initialStats.psiSkill = 0;

		_currentStats = _initialStats;	

		if (!names->empty())
		{
			_name = names->at(RNG::generate(0, names->size()-1))->genName(&_gender);
		}
		else
		{
			_name = L"";
			_gender = (SoldierGender)RNG::generate(0, 1);
		}
		_look = (SoldierLook)RNG::generate(0, 3);
	}
	if (id != 0)
	{
		_id = id;
	}
}

/**
 *
 */
Soldier::~Soldier()
{
	for (std::vector<EquipmentLayoutItem*>::iterator i = _equipmentLayout.begin(); i != _equipmentLayout.end(); ++i)
	{
		delete *i;
	}
}

/**
 * Loads the soldier from a YAML file.
 * @param node YAML node.
 * @param rule Game ruleset.
 */
void Soldier::load(const YAML::Node &node, const Ruleset *rule)
{
	int a = 0;
	node["id"] >> _id;
	std::string name;
	node["name"] >> name;
	_name = Language::utf8ToWstr(name);
	node["initialStats"] >> _initialStats;
	node["currentStats"] >> _currentStats;
	node["rank"] >> a;
	_rank = (SoldierRank)a;
	node["gender"] >> a;
	_gender = (SoldierGender)a;
	node["look"] >> a;
	_look = (SoldierLook)a;
	node["missions"] >> _missions;
	node["kills"] >> _kills;
	node["recovery"] >> _recovery;
	std::string armor;
	node["armor"] >> armor;
	_armor = rule->getArmor(armor);
	node["psiTraining"] >> _psiTraining;
	if (const YAML::Node *layoutNode = node.FindValue("equipmentLayout"))
		for (YAML::Iterator i = layoutNode->begin(); i != layoutNode->end(); ++i)
			_equipmentLayout.push_back(new EquipmentLayoutItem(*i));
}

/**
 * Saves the soldier to a YAML file.
 * @param out YAML emitter.
 */
void Soldier::save(YAML::Emitter &out) const
{
	out << YAML::BeginMap;
	out << YAML::Key << "id" << YAML::Value << _id;
	out << YAML::Key << "name" << YAML::Value << Language::wstrToUtf8(_name);
	out << YAML::Key << "initialStats" << YAML::Value << _initialStats;
	out << YAML::Key << "currentStats" << YAML::Value << _currentStats;
	out << YAML::Key << "rank" << YAML::Value << _rank;
	if (_craft != 0)
	{
		out << YAML::Key << "craft" << YAML::Value;
		_craft->saveId(out);
	}
	out << YAML::Key << "gender" << YAML::Value << _gender;
	out << YAML::Key << "look" << YAML::Value << _look;
	out << YAML::Key << "missions" << YAML::Value << _missions;
	out << YAML::Key << "kills" << YAML::Value << _kills;
	out << YAML::Key << "recovery" << YAML::Value << _recovery;
	out << YAML::Key << "armor" << YAML::Value << _armor->getType();
	out << YAML::Key << "psiTraining" << YAML::Value << _psiTraining;
	if (!_equipmentLayout.empty())
	{
		out << YAML::Key << "equipmentLayout" << YAML::Value;
		out << YAML::BeginSeq;
		for (std::vector<EquipmentLayoutItem*>::const_iterator j = _equipmentLayout.begin(); j != _equipmentLayout.end(); ++j)
			(*j)->save(out);
		out << YAML::EndSeq;
	}
	out << YAML::EndMap;
}

/**
 * Returns the soldier's full name.
 * @return Soldier name.
 */
std::wstring Soldier::getName() const
{
	return _name;
}

/**
 * Changes the soldier's full name.
 * @param name Soldier name.
 */
void Soldier::setName(const std::wstring &name)
{
	_name = name;
}

/**
 * Returns the craft the soldier is assigned to.
 * @return Pointer to craft.
 */
Craft *Soldier::getCraft() const
{
	return _craft;
}

/**
 * Assigns the soldier to a new craft.
 * @param craft Pointer to craft.
 */
void Soldier::setCraft(Craft *craft)
{
	_craft = craft;
}

/**
 * Returns the soldier's craft string, which
 * is either the soldier's wounded status,
 * the assigned craft name, or none.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::wstring Soldier::getCraftString(Language *lang) const
{
	std::wstring s;
	if (_recovery > 0)
	{
		s = lang->getString("STR_WOUNDED");
	}
	else if (_craft == 0)
	{
		s = lang->getString("STR_NONE_UC");
	}
	else
	{
		s = _craft->getName(lang);
	}
	return s;
}

/**
 * Returns a localizable-string representation of
 * the soldier's military rank.
 * @return String ID for rank.
 */
std::string Soldier::getRankString() const
{
	switch (_rank)
	{
	case RANK_ROOKIE:
		return "STR_ROOKIE";
		break;
	case RANK_SQUADDIE:
		return "STR_SQUADDIE";
		break;
	case RANK_SERGEANT:
		return "STR_SERGEANT";
		break;
	case RANK_CAPTAIN:
		return "STR_CAPTAIN";
		break;
	case RANK_COLONEL:
		return "STR_COLONEL";
		break;
	case RANK_COMMANDER:
		return "STR_COMMANDER";
		break;
	}
	return "";
}

/**
 * Returns a graphic representation of
 * the soldier's military rank.
 * @note THE MEANING OF LIFE
 * @return Sprite ID for rank.
 */
int Soldier::getRankSprite() const
{
	return 42 + _rank;
}


/**
 * Returns the soldier's military rank.
 * @return Rank enum.
 */
SoldierRank Soldier::getRank() const
{
	return _rank;
}

/**
 * Increase the soldier's military rank.
 */
void Soldier::promoteRank()
{
	_rank = (SoldierRank)((int)_rank + 1);
	if (_rank > RANK_SQUADDIE)
	{
		// only promotions above SQUADDIE are worth to be mentioned
		_recentlyPromoted = true;
	}
}

/**
 * Returns the soldier's amount of missions.
 * @return Missions.
 */
int Soldier::getMissions() const
{
	return _missions;
}

/**
 * Returns the soldier's amount of kills.
 * @return Kills.
 */
int Soldier::getKills() const
{
	return _kills;
}

/**
 * Returns the soldier's gender.
 * @return Gender.
 */
SoldierGender Soldier::getGender() const
{
	return _gender;
}

/**
 * Returns the soldier's look.
 * @return Look.
 */
SoldierLook Soldier::getLook() const
{
	return _look;
}

/**
 * Returns the soldier's rules.
 * @return rulesoldier
 */
RuleSoldier *Soldier::getRules() const
{
	return _rules;
}

/**
 * Returns the soldier's unique ID. Each soldier
 * can be identified by its ID. (not it's name)
 * @return Unique ID.
 */
int Soldier::getId() const
{
	return _id;
}

/**
 * Add a mission to the counter.
 */
void Soldier::addMissionCount()
{
	_missions++;
}

/**
 * Add a kill to the counter.
 */
void Soldier::addKillCount(int count)
{
	_kills += count;
}

/**
 * Get pointer to initial stats.
 */
UnitStats *Soldier::getInitStats()
{
	return &_initialStats;
}

/**
 * Get pointer to current stats.
 */
UnitStats *Soldier::getCurrentStats()
{
	return &_currentStats;
}

/**
 * Returns the unit's promotion status and resets it.
 * @return True if recently promoted, False otherwise.
 */
bool Soldier::isPromoted()
{
	bool promoted = _recentlyPromoted;
	_recentlyPromoted = false;
	return promoted;
}

/**
 * Returns the unit's current armor.
 * @return Pointer to armor data.
 */
Armor *Soldier::getArmor() const
{
	return _armor;
}

/**
 * Changes the unit's current armor.
 * @param armor Pointer to armor data.
 */
void Soldier::setArmor(Armor *armor)
{
	_armor = armor;
}

/**
 * Returns the amount of time until the soldier is healed.
 * @return Number of days.
 */
int Soldier::getWoundRecovery() const
{
	return _recovery;
}

/**
 * Changes the amount of time until the soldier is healed.
 * @param recovery Number of days.
 */
void Soldier::setWoundRecovery(int recovery)
{
	_recovery = recovery;

	// dismiss from craft
	if (_recovery > 0)
	{
		_craft = 0;
	}
}

/**
 * Heals soldier wounds.
 */
void Soldier::heal()
{
	_recovery--;
}

/**
 * Returns the list of EquipmentLayoutItems of a soldier.
 * @return Pointer to the EquipmentLayoutItem list.
 */
std::vector<EquipmentLayoutItem*> *Soldier::getEquipmentLayout()
{
	return &_equipmentLayout;
}

/**
 * Trains a soldier's Psychic abilities
 */
void Soldier::trainPsi()
{
	if (!_psiTraining)
	{
		_improvement = 0;
		return;
	}

	if (Options::getBool("anytimePsiTraining"))
	{
		if (_currentStats.psiSkill > 0)
		{
			if (_currentStats.psiSkill < 100 && 100 * 10/_currentStats.psiSkill >= RNG::generate(1, 100))
			{
				++_currentStats.psiSkill;
				++_improvement;
			}
		}
		else if (_currentStats.psiSkill < 0)
		{
			if (++_currentStats.psiSkill == 0)	// initial training is over
				_currentStats.psiSkill = RNG::generate(16, 24);
		}
		else if (_currentStats.psiSkill == 0)
		{
			_currentStats.psiSkill = -RNG::generate(30, 50);	// set initial training from 30 to 50 days
		}
		return;
	}

	_improvement = 0;
	if (_currentStats.psiSkill < 0)
		_currentStats.psiSkill = 0;
	else if(_currentStats.psiSkill <= 16)
		_improvement = RNG::generate(16, 24);
	else if(_currentStats.psiSkill <= 50)
		_improvement = RNG::generate(5, 12);
	else if(_currentStats.psiSkill < 100)
		_improvement = RNG::generate(1, 3);
	_currentStats.psiSkill += _improvement;
	if(_currentStats.psiSkill > 100)
		_currentStats.psiSkill = 100;
}

/**
 * returns whether or not the unit is in psi training
 */
bool Soldier::isInPsiTraining()
{
	return _psiTraining;
}

/**
 * changes whether or not the unit is in psi training
 */
void Soldier::setPsiTraining()
{
	_psiTraining = !_psiTraining;
}

/**
 * returns this soldier's psionic improvement score for this month.
 */
int Soldier::getImprovement()
{
	return _improvement;
}
}
