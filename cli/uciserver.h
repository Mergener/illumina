#ifndef ILLUMINA_UCISERVER_H
#define ILLUMINA_UCISERVER_H

#include <string>
#include <string_view>
#include <functional>
#include <ostream>
#include <istream>
#include <memory>
#include <map>
#include <unordered_map>
#include <optional>

#include "parsehelper.h"
#include "types.h"

namespace illumina {

class UCIServer;

struct UCIBadCommandArgument : std::runtime_error {
    UCIBadCommandArgument(std::string_view command_name,
                          std::string_view missing_arg,
                          std::string_view expected_arg_type_name);
};

class UCICommandContext {
    friend class UCIServer;
public:
    bool        has_arg(std::string_view arg_name) const;
    std::string word_after(std::string_view arg_name,
                           std::optional<std::string> default_val = std::nullopt) const;
    std::string all_after(std::string_view arg_name,
                          std::optional<std::string> default_val = std::nullopt) const;
    i64         int_after(std::string_view arg_name,
                          std::optional<i64> default_val = std::nullopt) const;

private:
    std::string m_cmd_name;
    std::string m_arg;

    UCICommandContext(UCIServer* server,
                      std::string_view command_name,
                      std::string_view arg);
};

using UCICommandHandler      = std::function<void(UCICommandContext&)>;
using UCIErrorHandler        = std::function<void(UCIServer&, const std::exception&)>;

class UCIServer {
public:
    void handle(std::string_view command);
    void register_command(const std::string& command,
                          const UCICommandHandler& handler);
    void set_error_handler(const UCIErrorHandler& error_handler);
    void serve(std::istream& in = std::cin, std::ostream& out = std::cout);
    void stop_serving();

    UCIServer() = default;
    ~UCIServer() = default;
    UCIServer(UCIServer&& rhs) = default;
    UCIServer& operator=(const UCIServer& rhs) = default;

private:
    std::unordered_map<std::string, UCICommandHandler> m_cmd_handlers;
    UCIErrorHandler m_err_handler = nullptr;

    bool m_should_stop_serving = false;
};

} // illumina

#endif // ILLUMINA_UCISERVER_H
