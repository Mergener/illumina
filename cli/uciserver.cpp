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
    stream << "Missing or invalid required " << expected_arg_type_name << " argument for command '" << command_name << "': '"
           << missing_arg << "'";
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

UCIServer& UCICommandContext::server() const {
    return *m_server;
}

UCICommandContext::UCICommandContext(UCIServer* server,
                                     std::string_view command_name,
                                     std::string_view arg)
    : m_server(server), m_cmd_name(std::string(command_name)), m_arg(std::string(arg)) { }

//
// UCIOption
//

void UCIOption::parse_and_set(std::string_view str) {
    try {
        parse_and_set_handler(str);
    }
    catch (const std::exception& e) {
        std::cerr << "Error when trying to parse value for option " << name() << ": " << e.what() << std::endl;
        return;
    }

    // Notify update handlers.
    for (const UCIOptionUpdateHandler& handler: m_update_handlers) {
        handler(*this);
    }
}

void UCIOption::add_update_handler(const UCIOptionUpdateHandler& handler) {
    m_update_handlers.push_back(handler);
}

bool UCIOption::has_value() const {
    return true;
}

bool UCIOption::has_min_value() const {
    return false;
}

bool UCIOption::has_max_value() const {
    return false;
}

std::string UCIOption::min_value_str() const {
    return "";
}

std::string UCIOption::max_value_str() const {
    return "";
}

const std::string& UCIOption::name() const {
    return m_name;
}

UCIOption::UCIOption(std::string name) : m_name(std::move(name)) { }

//
// UCIOptionString
//

const std::string& UCIOptionString::value() const {
    return m_val;
}

const std::string& UCIOptionString::default_value() const {
    return m_default_val;
}

void UCIOptionString::parse_and_set_handler(std::string_view str) {
    m_val = str;
}

std::string UCIOptionString::default_value_str() const {
    return m_default_val;
}

std::string UCIOptionString::current_value_str() const {
    return m_val;
}

std::string UCIOptionString::type_name() const {
    return "string";
}

UCIOptionString::UCIOptionString(std::string name, std::string default_val)
    : UCIOption(std::move(name)),
      m_val(std::move(default_val)),
      m_default_val(std::move(default_val)) { }

//
// UCIOptionSpin
//

i64 UCIOptionSpin::value() const {
    return m_val;
}

i64 UCIOptionSpin::default_value() const {
    return m_default_val;
}

i64 UCIOptionSpin::min_value() const {
    return m_min_val;
}

i64 UCIOptionSpin::max_value() const {
    return m_max_val;
}

void UCIOptionSpin::parse_and_set_handler(std::string_view str) {
    i64 parsed_val = parse_int(str);
    if (parsed_val < min_value() || parsed_val > max_value()) {
        throw std::invalid_argument(std::to_string(parsed_val) + " is out of expected bounds ("
                                  + std::to_string(min_value()) + ", "
                                  + std::to_string(max_value()) + ")");
    }
    m_val = parsed_val;
}

std::string UCIOptionSpin::default_value_str() const {
    return std::to_string(m_default_val);
}

std::string UCIOptionSpin::current_value_str() const {
    return std::to_string(m_val);
}

std::string UCIOptionSpin::type_name() const {
    return "spin";
}

bool UCIOptionSpin::has_min_value() const {
    return true;
}

bool UCIOptionSpin::has_max_value() const {
    return true;
}

std::string UCIOptionSpin::min_value_str() const {
    return std::to_string(min_value());
}

std::string UCIOptionSpin::max_value_str() const {
    return std::to_string(max_value());
}


UCIOptionSpin::UCIOptionSpin(std::string name,
                             i64 default_val,
                             i64 min_val,
                             i64 max_val)
    : UCIOption(std::move(name)),
      m_val(default_val),
      m_default_val(default_val),
      m_min_val(min_val),
      m_max_val(max_val) { }

//
// UCIOptionCheck
//

bool UCIOptionCheck::value() const {
    return m_val;
}

bool UCIOptionCheck::default_value() const {
    return m_default_val;
}

void UCIOptionCheck::parse_and_set_handler(std::string_view str) {
    if (str == "true") {
        m_val = true;
    }
    else if (str == "false") {
        m_val = false;
    }
    else {
        throw std::invalid_argument("Invalid boolean value '" + std::string(str) + "'. (expected 'true' or 'false')");
    }
}

std::string UCIOptionCheck::default_value_str() const {
    return std::to_string(m_default_val);
}

std::string UCIOptionCheck::current_value_str() const {
    return std::to_string(m_val);
}

std::string UCIOptionCheck::type_name() const {
    return "check";
}

UCIOptionCheck::UCIOptionCheck(std::string name, bool default_val)
    : UCIOption(std::move(name)),
      m_val(default_val),
      m_default_val(default_val) { }

//
// UCIOptionCombo
//

void UCIOptionCombo::parse_and_set_handler(std::string_view str) {
    bool ok = false;
    for (const std::string& opt: m_opts) {
        if (opt == str) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        throw std::invalid_argument("Unexpected option '" + std::string(str) + "' for " + name() + ".");
    }

    UCIOptionString::parse_and_set_handler(str);
}

std::string UCIOptionCombo::type_name() const {
    return "combo";
}

UCIOptionCombo::UCIOptionCombo(std::string name,
                               std::string default_val,
                               std::vector<std::string> options)
    : UCIOptionString(std::move(name), std::move(default_val)),
      m_opts(std::move(options)) { }

//
// UCIOptionButton
//

void UCIOptionButton::parse_and_set_handler(std::string_view str) {
}

std::string UCIOptionButton::default_value_str() const {
    return "";
}

std::string UCIOptionButton::current_value_str() const {
    return "";
}

std::string UCIOptionButton::type_name() const {
    return "button";
}

bool UCIOptionButton::has_value() const {
    return false;
}

UCIOptionButton::UCIOptionButton(std::string name)
    : UCIOption(std::move(name)) { }

//
// UCIServer
//

UCIOption& UCIServer::option(const std::string& name) {
    auto it = m_options.find(name);
    if (it == m_options.end()) {
        throw std::out_of_range("Option not found: " + name);
    }
    return *it->second;
}

const UCIOption& UCIServer::option(const std::string& name) const {
    auto it = m_options.find(name);
    if (it == m_options.end()) {
        throw std::out_of_range("Option not found: " + name);
    }
    return *it->second;
}

UCIServer::OptionList UCIServer::list_options() const {
    OptionList list;
    for (const auto& pair: m_options) {
        list.emplace_back(*pair.second);
    }
    return list;
}

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