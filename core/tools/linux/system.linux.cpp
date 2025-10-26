#include "stdafx.h"
#include "../system.h"
#include <paths.h>

//use std::string instead of std::wstring for std::filesystem::path construction; linux only; https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95048

bool core::tools::system::is_dir_writable(const std::wstring &_dir_path_str)
{
    return true;
}

bool core::tools::system::delete_file(const std::wstring& _file_name)
{
    std::error_code error;
    std::filesystem::remove(from_utf16(_file_name), error);
    return !error;
}

bool core::tools::system::move_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    std::error_code error;
    std::filesystem::rename(from_utf16(_old_file), from_utf16(_new_file), error);

    if (!error)
    {
        return true;
    }
    else
    {
        // fallback. use copy and delete
        if (copy_file(_old_file, _new_file))
            return delete_file(_old_file);
        return false;
    }
}

bool core::tools::system::copy_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    std::error_code error;
    std::filesystem::copy(from_utf16(_old_file), from_utf16(_new_file), error);
    return !error;
}

bool core::tools::system::compare_dirs(const std::wstring& _dir1, const std::wstring& _dir2)
{
    if (_dir1.empty() || _dir2.empty())
        return false;

    std::error_code error;
    return std::filesystem::equivalent(from_utf16(_dir1), from_utf16(_dir2), error);
}

std::wstring core::tools::system::get_file_directory(const std::wstring& file)
{
    std::filesystem::path p(from_utf16(file));
    return p.parent_path().wstring();
}

std::wstring core::tools::system::get_file_name(const std::wstring& file)
{
    std::filesystem::path p(from_utf16(file));
    return p.filename().wstring();
}

std::wstring core::tools::system::get_temp_directory()
{
    std::string tmpdir;

    if (auto tmpdir_env = getenv("TMPDIR"))
        tmpdir = tmpdir_env;

    if (tmpdir.empty())
        tmpdir = _PATH_TMP;

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(tmpdir);

    return wide;
}

std::wstring core::tools::system::get_user_profile()
{
    std::string result = get_home_directory();

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(result);

    return wide;
}

unsigned long core::tools::system::get_current_thread_id()
{
    std::string threadId = boost::lexical_cast<std::string>(boost::this_thread::get_id());
    unsigned long threadNumber = 0;
    sscanf(threadId.c_str(), "%lx", &threadNumber);
    return threadNumber;
}

std::wstring core::tools::system::get_user_downloads_dir()
{
    std::string result = getenv("HOME");
    result.append("/Downloads");

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(result);

    return wide;
}

std::string core::tools::system::to_upper(std::string_view str)
{
    std::wstring upper = from_utf8(str);
    std::transform(upper.begin(), upper.end(), upper.begin(), ::towupper);
    return from_utf16(upper);
}

std::string core::tools::system::to_lower(std::string_view str)
{
    std::wstring lower = from_utf8(str);
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return from_utf16(lower);
}

std::string core::tools::system::get_os_version_string()
{
    // TODO : use actual value here
    return "Linux";
}

bool is_do_not_dirturb_on()
{
    return false;
}
