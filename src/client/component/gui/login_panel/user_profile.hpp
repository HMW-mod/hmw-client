#pragma once
namespace user_profile {

	void set_username(std::string username);
	void set_prestige(int prestige);
	void set_rank(int rank);
	void set_rank_xp(int rank_xp);
	void set_avatar(std::string avatar);
	void login();
	void logout();

	std::string get_username();
	int get_prestige();
	int get_rank();
	int get_rank_xp();
	std::string get_avatar();
	std::string get_avatar_url();
	void reset();
	bool is_logged_in();
}
