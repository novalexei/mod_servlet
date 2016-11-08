#include <fstream>
#include <experimental/filesystem>

#include <http_request.h>

#include "dispatcher.h"
#include "string.h"

namespace servlet
{

namespace fs = std::experimental::filesystem;

void default_servlet::init()
{
    const servlet_config &config = get_servlet_config();
    optional_ref<const std::string> param = config.get_init_parameter("useAcceptRanges");
    if (param) _use_accept_ranges = equal_ic(*param, "true");
}

void default_servlet::do_get(http_request &req, http_response &resp)
{
    if (resp.get_status() != OK) return;
    string_view file_path_str = req.get_path_translated();
    fs::path file_path{file_path_str.begin(), file_path_str.end()};
    std::error_code err;
    fs::file_status stat = fs::status(file_path, err);
    if (err)
    {
        if (_logger->is_loggable(logging::LEVEL::INFO))
            _logger->info() << "Failed to get status of file '" << file_path_str << "': " << err.message() << '\n';
        resp.set_status(http_response::SC_NOT_FOUND);
        return;
    }
    if (!fs::is_regular_file(stat))
    {
        if (_logger->is_loggable(logging::LEVEL::INFO))
            _logger->info() << "File '" << file_path_str << "' is not a regular file" << '\n';
        resp.set_status(http_response::SC_NOT_FOUND);
        return;
    }
    uintmax_t file_size = fs::file_size(file_path, err);
    if (err)
    {
        if (_logger->is_loggable(logging::LEVEL::INFO))
            _logger->info() << "Failed to obtain file size of file '" << file_path_str << "': " << err.message()
                            << '\n';
        resp.set_status(http_response::SC_NOT_FOUND);
        return;
    }
    fs::file_time_type last_modified = fs::last_write_time(file_path, err);
    if (err)
    {
        if (_logger->is_loggable(logging::LEVEL::INFO))
            _logger->info() << "Failed to obtain file modification time of file '"
                            << file_path_str << "': " << err.message() << '\n';
        resp.set_status(http_response::SC_NOT_FOUND);
        return;
    }
    std::ifstream in;
    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        in.open(file_path.generic_string(), std::ios_base::in | std::ios_base::binary);
        in.exceptions(std::ifstream::goodbit);
    }
    catch (const std::ios_base::failure &ex)
    {
        if (_logger->is_loggable(logging::LEVEL::INFO))
            _logger->info() << "Failed to open file '" << file_path_str << "': " << ex.what() << '\n';
        resp.set_status(http_response::SC_FORBIDDEN);
        return;
    }
    const servlet_context &context = get_servlet_config().get_servlet_context();
    optional_ref<const std::string> mime_type = context.get_mime_type(file_path_str);
    if (mime_type) resp.set_content_type(*mime_type);
    resp.set_date_header("Last-Modified", last_modified);
    if (file_size > 0)
    {
        auto lm = std::chrono::duration_cast<std::chrono::milliseconds>(last_modified.time_since_epoch()).count();
        if (lm > 0) resp.set_header("ETag", std::string{"W\""} + file_size + "-" + lm + "\"");
    }
    resp.set_header("Accept-Ranges", _use_accept_ranges ? "bytes" : "none");
    resp.set_content_length(file_size);

    resp.get_output_stream() << in.rdbuf();
    in.close();
}

} // end of servlet namespace
