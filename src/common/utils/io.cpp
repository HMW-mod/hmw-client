#include "io.hpp"
#include "nt.hpp"
#include <fstream>
#include <curl/curl.h>
#include <iostream>

namespace utils::io
{
	bool remove_file(const std::string& file)
	{
		return DeleteFileA(file.data()) == TRUE;
	}

	bool move_file(const std::string& src, const std::string& target)
	{
		return MoveFileA(src.data(), target.data()) == TRUE;
	}

	bool file_exists(const std::string& file)
	{
		return std::ifstream(file).good();
	}

	bool write_file(const std::string& file, const std::string& data, const bool append)
	{
		const auto pos = file.find_last_of("/\\");
		if (pos != std::string::npos)
		{
			create_directory(file.substr(0, pos));
		}

		std::ofstream stream(
			file, std::ios::binary | std::ofstream::out | (append ? std::ofstream::app : 0));

		if (stream.is_open())
		{
			stream.write(data.data(), static_cast<std::streamsize>(data.size()));
			stream.close();
			return true;
		}

		return false;
	}

	std::string read_file(const std::string& file)
	{
		std::string data;
		read_file(file, &data);
		return data;
	}

	bool read_file(const std::string& file, std::string* data)
	{
		if (!data) return false;
		data->clear();

		if (file_exists(file))
		{
			std::ifstream stream(file, std::ios::binary);
			if (!stream.is_open()) return false;

			stream.seekg(0, std::ios::end);
			const std::streamsize size = stream.tellg();
			stream.seekg(0, std::ios::beg);

			if (size > -1)
			{
				data->resize(static_cast<uint32_t>(size));
				stream.read(const_cast<char*>(data->data()), size);
				stream.close();
				return true;
			}
		}

		return false;
	}

	size_t file_size(const std::string& file)
	{
		if (file_exists(file))
		{
			std::ifstream stream(file, std::ios::binary);

			if (stream.good())
			{
				stream.seekg(0, std::ios::end);
				return static_cast<size_t>(stream.tellg());
			}
		}

		return 0;
	}

	bool create_directory(const std::string& directory)
	{
		return std::filesystem::create_directories(directory);
	}

	bool directory_exists(const std::string& directory)
	{
		return std::filesystem::is_directory(directory);
	}

	bool directory_is_empty(const std::string& directory)
	{
		return std::filesystem::is_empty(directory);
	}

	bool remove_directory(const std::string& directory)
	{
		return std::filesystem::remove_all(directory);
	}

	std::vector<std::string> list_files(const std::string& directory)
	{
		std::vector<std::string> files;

		for (auto& file : std::filesystem::directory_iterator(directory))
		{
			files.push_back(file.path().generic_string());
		}

		return files;
	}

	std::vector<std::string> list_files_recursively(const std::string& directory)
	{
		std::vector<std::string> files;

		for (auto& file : std::filesystem::recursive_directory_iterator(directory))
		{
			files.push_back(file.path().generic_string());
		}

		return files;
	}

	void copy_folder(const std::filesystem::path& src, const std::filesystem::path& target)
	{
		std::filesystem::copy(src, target,
		                      std::filesystem::copy_options::overwrite_existing |
		                      std::filesystem::copy_options::recursive);
	}

	// Function to download a file using libcurl
	bool download_file(const std::string& url, const std::string& output_path) {
		CURL* curl = curl_easy_init();
		if (!curl) {
			std::cerr << "Failed to initialize libcurl" << std::endl;
			return false;
		}

		std::string buffer;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "Error during file download: " << curl_easy_strerror(res) << std::endl;
			curl_easy_cleanup(curl);
			return false;
		}

		curl_easy_cleanup(curl);

		// Use `write_file` from your utils::io to save the downloaded content
		if (!write_file(output_path, buffer, false)) {
			std::cerr << "Failed to write file: " << output_path << std::endl;
			return false;
		}

		return true;
	}

	bool download_file_with_auth(const std::string& url, const std::string& output_path, const std::string& bearer_token)
	{
		CURL* curl = curl_easy_init();
		if (!curl) {
			std::cerr << "Failed to initialize libcurl" << std::endl;
			return false;
		}

		std::string buffer;
		struct curl_slist* headers = nullptr;

		std::string auth_header = "Authorization: Bearer " + bearer_token;
		headers = curl_slist_append(headers, auth_header.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "Error during file download: " << curl_easy_strerror(res) << std::endl;
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			return false;
		}

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);

		if (!write_file(output_path, buffer, false)) {
			std::cerr << "Failed to write file: " << output_path << std::endl;
			return false;
		}

		return true;
	}

	size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
		size_t totalSize = size * nmemb;
		auto* buffer = static_cast<std::string*>(userp);
		buffer->append(static_cast<char*>(contents), totalSize);
		return totalSize;
	}
}
