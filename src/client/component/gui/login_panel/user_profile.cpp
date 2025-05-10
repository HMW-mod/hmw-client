#include <std_include.hpp>
#include "user_profile.hpp"

std::string username = "User";
int prestige = 0;
int rank = 0;
int rank_xp = 0;
std::string avatar = "";
bool logged_in = false;

void user_profile::set_username(std::string _username)
{
	username = _username;
}

void user_profile::set_prestige(int _prestige)
{
	prestige = _prestige;
}

void user_profile::set_rank(int _rank)
{
	rank = _rank;
}

void user_profile::set_rank_xp(int _rank_xp)
{
	rank_xp = _rank_xp;
}

void user_profile::set_avatar(std::string _avatar)
{
	avatar = _avatar;
}

void user_profile::login()
{
	if (logged_in) {
		return;
	}
	logged_in = true;
}

void user_profile::logout()
{
	if (!logged_in) {
		return;
	}
	logged_in = false;
}

std::string user_profile::get_username()
{
	return username;
}

int user_profile::get_prestige()
{
	return prestige;
}

int user_profile::get_rank()
{
	return rank;
}

int user_profile::get_rank_xp()
{
	return rank_xp;
}

std::string user_profile::get_avatar()
{
	return avatar;
}

std::string user_profile::get_avatar_url()
{
	return "https://backend.horizonmw.org/assets/" + avatar + "?key=system-medium-cover";
}

void user_profile::reset()
{
	username = "User";
	prestige = 0;
	rank_xp = 0;
	avatar = "";
	logout();
}

bool user_profile::is_logged_in()
{
	return logged_in;
}
