#pragma once
#include "server_browser.hpp"

namespace server_info_panel {
	void show_info_panel();
	void refresh(std::string address);
	void open(std::vector<server_browser::ServerData> list, int server_index);
	void close();

	// Helper
	game::Material* get_map_material();
	game::Material* get_material(std::string asset_name);
	void draw_map_image(game::Material* asset);
	void update_server(const std::string& infoJson, const std::string& connect_address, server_browser::ServerData& server);
}

