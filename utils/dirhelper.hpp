#pragma once

#define DIRHELPER_NO_STR

#include <vector>
#include <functional>

#include <filesystem> // c++17
namespace fs = std::filesystem;


#ifndef DIRHELPER_NO_STR

#include <string>

#endif // !DIRHELPER_NO_STR




namespace dirhelper
{
	using path_t = fs::path;

	using file_list_t = std::vector<path_t>;
	using file_func_t = std::function<void(path_t const&)>;

	file_list_t get_all_files(path_t const& src_dir);

	file_list_t get_files_of_type(path_t const& src_dir, const char* extension);

	file_list_t get_all_files(path_t const& src_dir, size_t max_size);

	file_list_t get_files_of_type(path_t const& src_dir, const char* extension, size_t max_size);

	path_t get_first_file_of_type(path_t const& src_dir, const char* extension);

	void process_files(file_list_t const& files, file_func_t const& func);


#ifndef DIRHELPER_NO_STR

	file_list_t get_all_files(std::string const& src_dir);

	file_list_t get_files_of_type(std::string const& src_dir, std::string const& extension);

	file_list_t get_files_of_type(const char* src_dir, std::string const& extension);

	file_list_t get_files_of_type(std::string const& src_dir, const char* extension);

	

		
	
	using file_str_list_t = std::vector<std::string>;
	using file_str_func_t = std::function<void(std::string const&)>;
	

	void process_files(file_str_list_t const& files, file_str_func_t const& func);


	


	// for testing/debugging
	std::string dbg_get_file_name(path_t const& file_path);

	std::string dbg_get_file_name(std::string const& file_path);




	// string overloads
	namespace str
	{
		using file_list_t = std::vector<std::string>;

		file_list_t get_all_files(const char* src_dir);

		file_list_t get_all_files(std::string const& src_dir);

		file_list_t get_files_of_type(std::string const& src_dir, std::string const& extension);

		file_list_t get_files_of_type(const char* src_dir, std::string const& extension);

		file_list_t get_files_of_type(std::string const& src_dir, const char* extension);

		file_list_t get_files_of_type(const char* src_dir, const char* extension);

		std::string get_first_file_of_type(const char* src_dir, const char* extension);

		std::string get_first_file_of_type(std::string const& src_dir, std::string const& extension);

		std::string get_first_file_of_type(const char* src_dir, std::string const& extension);

		std::string get_first_file_of_type(std::string const& src_dir, const char* extension);
	}

#endif // !DIRHELPER_NO_STR
}
