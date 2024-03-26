#include "uciserver.h"

#include <sstream>

#include "utils.h"

namespace illumina {

//
// UCIMissingCommandArgument
//

static std::string generate_missing_arg_error_string(std::string_view command_name,
                                                     std::string_view missing_arg,
                                                     std::string_view expected_arg_type_name) {
    std::stringstream stream;

    if (!missing_arg.empty()) {
        stream << "Missing or invalid required "
               << expected_arg_type_name
               << " argument for command '"
               << command_name
               << "': '"
               << missing_arg << "'";
    }
    else {
        stream << "Missing or invalid required "
               << expected_arg_type_name
               << " positional argument for command '"
               << command_name << "'.";
    }
    return stream.str();
}

UCIBadCommandArgument::UCIBadCommandArgument(std::string_view command_name,
                                             std::string_view missing_arg,
                                             std::string_view expected_arg_type_name)
                                                     : std::runtime_error(generate_missing_arg_error_string(
                                                         command_name, missing_arg, expected_arg_type_name
                                                         )) {}

//
// UCICommandContext
//

static bool goto_arg(ParseHelper& parser, std::string_view arg_name) {
    if (arg_name.empty()) {
        return !parser.finished();
    }

    while (!parser.finished()) {
        std::string_view chunk = parser.read_chunk();
        if (chunk == arg_name) {
            return true;
        }
    }
    return false;
}

bool UCICommandContext::has_arg(std::string_view arg_name) const {
    ParseHelper parser(m_arg);
    return goto_arg(parser, arg_name);
}

std::string UCICommandContext::word_after(std::string_view arg_name,
                                          std::optional<std::string> default_val) const {
    ParseHelper parser(m_arg);
    if (!goto_arg(parser, arg_name)) {
        // Argument not found, try to use default value.
        if (default_val.has_value()) {
            return *default_val;
        }

        // No argument and no default value, throw.
        throw UCIBadCommandArgument(m_cmd_name, arg_name, "string");
    }
    // Argument found, return it.
    return std::string(parser.read_chunk());
}

i64 UCICommandContext::int_after(std::string_view arg_name,
                                 std::optional<i64> default_val) const {
    ParseHelper parser(m_arg);
    if (!goto_arg(parser, arg_name)) {
        // Argument not found, try to use default value.
        if (default_val.has_value()) {
            return *default_val;
        }

        // No argument and no default value, throw.
        throw UCIBadCommandArgument(m_cmd_name, arg_name, "integer");
    }
    // Argument found, try to parse it.
    std::string_view arg_word = parser.read_chunk();

    i64 value;
    if (try_parse_int(arg_word, value)) {
        // Value succesfully parsed.
        return value;
    }
    // Couldn't properly parse value.
    throw UCIBadCommandArgument(m_cmd_name, arg_name, "integer");
}

std::string UCICommandContext::all_after(std::string_view arg_name,
                                         std::optional<std::string> default_val) const {
    ParseHelper parser(m_arg);
    if (!goto_arg(parser, arg_name)) {
        // Argument not found, try to use default value.
        if (default_val.has_value()) {
            return *default_val;
        }

        // No argument and no default value, throw.
        throw UCIBadCommandArgument(m_cmd_name, arg_name, "string");
    }
    // Argument found, return it.
    return std::string(parser.remainder());
}

UCICommandContext::UCICommandContext(UCIServer* server,
                                     std::string_view command_name,
                                     std::string_view arg)
    : m_cmd_name(std::string(command_name)), m_arg(std::string(arg)) { }


//
// UCIServer
//

void UCIServer::handle(std::string_view command) {
    try {
        // Try to find command handler.
        ParseHelper parser(command);
        std::string command_name = std::string(parser.read_chunk());

        auto command_handler_it = m_cmd_handlers.find(command_name);
        if (command_handler_it == m_cmd_handlers.end()) {
            std::cerr << "Command not found: " << command_name << std::endl;
            return;
        }

        // Command handler found, try to handle the command.
        UCICommandHandler& handler = command_handler_it->second;
        UCICommandContext context(this, command_name, parser.remainder());
        handler(context);
    }
    catch (const UCIBadCommandArgument& bad_arg) {
        std::cerr << bad_arg.what() << std::endl;
    }
    catch (const std::exception& e) {
        if (!m_err_handler) {
            // Throw above, will maybe abort the program.
            throw e;
        }
        // We have an error handler, try to use it.
        m_err_handler(*this, e);
    }
}

void UCIServer::register_command(const std::string& command,
                                 const UCICommandHandler& handler) {
    m_cmd_handlers[command] = handler;
}

void UCIServer::set_error_handler(const UCIErrorHandler& error_handler) {
    m_err_handler = error_handler;
}

void UCIServer::serve(std::istream& in, std::ostream& out) {
    while (!m_should_stop_serving || in.eof()) {
        std::string line;
        std::getline(in, line);
        handle(line);
    }
}

void UCIServer::stop_serving() {
    m_should_stop_serving = true;
}

} // illumina