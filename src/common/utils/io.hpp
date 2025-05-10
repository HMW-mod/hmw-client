#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <json.hpp>

namespace utils::io
{
	bool remove_file(const std::string& file);
	bool move_file(const std::string& src, const std::string& target);
	bool file_exists(const std::string& file);
	bool write_file(const std::string& file, const std::string& data, bool append = false);
	bool read_file(const std::string& file, std::string* data);
	std::string read_file(const std::string& file);
	size_t file_size(const std::string& file);
	bool create_directory(const std::string& directory);
	bool directory_exists(const std::string& directory);
	bool directory_is_empty(const std::string& directory);
	bool remove_directory(const std::string& directory);
	std::vector<std::string> list_files(const std::string& directory);
	std::vector<std::string> list_files_recursively(const std::string& directory);
	void copy_folder(const std::filesystem::path& src, const std::filesystem::path& target);
	bool download_file(const std::string& url, const std::string& output_path);
	bool download_file_with_auth(const std::string& url, const std::string& output_path, const std::string& bearer_token);
	size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
}
