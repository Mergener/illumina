#ifndef ILLUMINA_CLIAPPLICATION_H
#define ILLUMINA_CLIAPPLICATION_H

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

class CLIApplication;

struct BadCommandArgument : std::runtime_error {
    BadCommandArgument(std::string_view command_name,
                       std::string_view missing_arg,
                       std::string_view expected_arg_type_name);
};

class CommandContext {
    friend class CLIApplication;
public:
    bool        has_arg(std::string_view arg_name) const;
    std::string word_after(std::string_view arg_name,
                           std::optional<std::string> default_val = std::nullopt) const;
    std::string all_after(std::string_view arg_name,
                          std::optional<std::string> default_val = std::nullopt) const;
    i64         int_after(std::string_view arg_name,
                          std::optional<i64> default_val = std::nullopt) const;
    std::string path_after(std::string_view arg_name,
                           std::optional<std::string> default_val = std::nullopt) const;

private:
    std::string m_cmd_name;
    std::string m_arg;

    CommandContext(CLIApplication* server,
                   std::string_view command_name,
                   std::string_view arg);
};

using CommandHandler = std::function<void(CommandContext&)>;
using ErrorHandler   = std::function<void(CLIApplication&, const std::exception&)>;

class CLIApplication {
public:
    void handle(std::string_view command);
    void register_command(const std::string& command,
                          const CommandHandler& handler);
    void set_error_handler(const ErrorHandler& error_handler);
    void listen(std::istream& in = std::cin, std::ostream& out = std::cout);
    void stop_listening();

    CLIApplication() = default;
    ~CLIApplication() = default;
    CLIApplication(CLIApplication&& rhs) = default;
    CLIApplication& operator=(const CLIApplication& rhs) = default;

private:
    std::unordered_map<std::string, CommandHandler> m_cmd_handlers;
    ErrorHandler m_err_handler = nullptr;

    bool m_should_stop_serving = false;
};

} // illumina

#endif // ILLUMINA_CLIAPPLICATION_H
